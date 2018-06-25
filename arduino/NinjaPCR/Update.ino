/**
 * OTA
 */
#include "Update.h"
#include "pcr_includes.h"
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiClientSecure.h>

// OTA mode
ESP8266HTTPUpdateServer httpUpdater;

// OTA boot type (0:normal mode, 1:local upload, 2:web download)
#define EEPROM_OTA_TYPE_ADDR  (EEPROM_WIFI_MDNS_HOST_ADDR+EEPROM_WIFI_MDNS_HOST_MAX_LENGTH+1)
#define EEPROM_OTA_CURRENT_VERSION_ADDR (EEPROM_OTA_TYPE_ADDR+1)
#define EEPROM_OTA_CURRENT_VERSION_MAXLENGTH 128

String PARAM_OTA_TYPE = "ot";
String PARAM_OTA_URL = "ou";
#define OTA_TYPE_NO_UPDATE 0 /* No update (boot with normal PCR mode) */
#define OTA_TYPE_LOCAL_UPLOAD 1
#define OTA_TYPE_WEB_DOWNLOAD 2

const char* OTA_HOST = "ninjapcrwifi";
const char* OTA_UPDATE_PATH = "/update";

char *firmwareHost = "ninjapcr.tori.st";
int port = 443;

/* Fetch MD5 hash of latest firmware in secure way */
String getMD5(String path) {
    WiFiClientSecure *httpsClient = new WiFiClientSecure();
    Serial.println("getMD5");
    Serial.println(firmwareHost);
    if (!httpsClient->connect(firmwareHost, port)) {
        Serial.println("Failed to connect.");
        return "";
    }
    Serial.println("Connected.");

    String url = path;

    httpsClient->print(
            String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + firmwareHost + "\r\n"
                    + "User-Agent: NinjaPCR\r\n" + "Connection: close\r\n\r\n");

    Serial.println("request sent");
    while (httpsClient->connected()) {
        String line = httpsClient->readStringUntil('\n');
        Serial.println(line);
        if (line == "\r") {
            break;
        }
    }
    String line = httpsClient->readStringUntil('\n');
    Serial.println(line);
    delete httpsClient;
    return line;
}

int otaType = 0;
String otaURL;

void clearOTAFlag() {
    EEPROM.write(EEPROM_OTA_TYPE_ADDR, OTA_TYPE_NO_UPDATE);
}

void loadOTAConfig() {
    char typeValueCh = EEPROM.read(EEPROM_OTA_TYPE_ADDR);
    Serial.print(typeValueCh);
    int otaType = typeValueCh - '0';

    if (otaType == OTA_TYPE_LOCAL_UPLOAD) {
        Serial.println("OTA_TYPE_LOCAL_UPLOAD");
        char *cOriginalVersion = (char *) malloc(
                sizeof(char) * (EEPROM_OTA_CURRENT_VERSION_MAXLENGTH + 1));
        readStringFromEEPROM(cOriginalVersion, EEPROM_OTA_CURRENT_VERSION_ADDR,
        EEPROM_OTA_CURRENT_VERSION_MAXLENGTH);
        String originalVersion(cOriginalVersion);
        String currentVersion(OPENPCR_FIRMWARE_VERSION_STRING);
        free(cOriginalVersion);
        Serial.println("OriginalVersion=" + originalVersion);

        if (currentVersion.equals(originalVersion)) {
            Serial.println("Same version.");
            isUpdateMode = true;
        }
        else {
            Serial.println("Version Changed.");
            clearOTAFlag();
            EEPROM.commit();
            isUpdateMode = false;
        }
    }
    else if (otaType == OTA_TYPE_WEB_DOWNLOAD) {
        //TODO download
        clearOTAFlag();
        EEPROM.commit();
        isUpdateMode = true;
    }
    else {
        otaType = 0;
        isUpdateMode = false;
    }

    Serial.print("OTA Type=");
    Serial.println(otaType);
}

void requestHandlerOTATop() {
    String s = getHTMLHeader();
    s += "<h1>Device Update</h1>\n<ul>";
    s += "<li><a href=\"/update\">Upload new firmware</a></li>";
    s += "<li><a href=\"/cancel\">Cancel (Back to normal mode)</a></li>";
    s += "</ul></body></html>\n";
    server.send(200, "text/html", s);
}

void requestHandlerOTACancel() {
    clearOTAFlag();
    EEPROM.commit();
    String s = getHTMLHeader();
    s += "<h1>Device Update</h1>\n";
    s += "<p>Device update is canceled.</p>";
    s += getRestartLink();
    s += "</body></html>\n";
    server.send(200, "text/html", s);
    ESP.restart();
}

String getRestartLink() {
    return "<a href=\"/restart\">Restart</a>";
}
void requestHandlerRestart() {
    String s = getHTMLHeader();
    s += "<h1>Device is restarting</h1>\n";
    server.send(200, "text/html", s);
    ESP.restart();
}

void requestHandlerOTAError() {
    wifi_send("{error:true}", "onErrorOTAMode");
}

void startUpdaterServer() {
    // OTA mode
    Serial.println("Starting OTA mode (isUpdateMode=true)");
    httpUpdater.setup(&server, OTA_UPDATE_PATH);
    server.on("/", requestHandlerOTATop);
    server.on("/cancel", requestHandlerOTACancel);
    server.on("/command", requestHandlerOTAError);
    server.on("/status", requestHandlerOTAError);
    server.on("/connect", requestHandlerOTAError);
    server.on("/config", requestHandlerOTAError);
    server.on("/restart", requestHandlerRestart);
    server.begin();
    Serial.printf("HTTPUpdateServer ready!");
}

/* Handle request to "/config"  (OTA conf) */
// Restart device in binary upload mode
void requestHandlerConfig() {
    String type = server.arg("ot");
    Serial.print("type=");
    Serial.print(type);
    saveStringToEEPROM(type, EEPROM_OTA_TYPE_ADDR, 1);
    saveStringToEEPROM(OPENPCR_FIRMWARE_VERSION_STRING,
    EEPROM_OTA_CURRENT_VERSION_ADDR,
    EEPROM_OTA_CURRENT_VERSION_MAXLENGTH);
    wifi_send("{accepted:true}", "onConf");

    EEPROM.commit();
    ESP.restart();
}

// Download the latest binary by HTTPS
void requestHandlerFirmwareUpdate() {
    String type = server.arg("ot");
    Serial.print("type=");
    Serial.print(type);
    saveStringToEEPROM("2", EEPROM_OTA_TYPE_ADDR, 1);
    saveStringToEEPROM(OPENPCR_FIRMWARE_VERSION_STRING,
    EEPROM_OTA_CURRENT_VERSION_ADDR,
    EEPROM_OTA_CURRENT_VERSION_MAXLENGTH);
    wifi_send("{accepted:true}", "onConf");

    EEPROM.commit();
    ESP.restart();
}
NinjaUpdate ninjaUpdate;

/*
 * Online update
 * */
void execUpdate() {
    Serial.println("Updating...");
    // Fetch MD5 of latest firmware by SSL
    String md5 = getMD5("/secure/NinjaPCR.md5");
    Serial.print("Expected MD5=" + md5);

    t_httpUpdate_return ret = HTTP_UPDATE_FAILED;
    ret = ninjaUpdate.update("http://ninjapcr.tori.st/bin/NinjaPCR.bin", md5);

    switch (ret) {
    case HTTP_UPDATE_FAILED:
        //Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", NinjaUpdate.getLastError(), NinjaUpdate.getLastErrorString().c_str());
        Serial.printf("HTTP_UPDATE_FAILD Error");
        break;

    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

    case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        ESP.restart();
        break;
    }
}
