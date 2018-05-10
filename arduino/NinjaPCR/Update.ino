/**
 * OTA
 */
#include <ESP8266HTTPUpdateServer.h>
#include "pcr_includes.h"


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

//OPENPCR_FIRMWARE_VERSION_STRING
int otaType = 0;
String otaURL;

void clearOTAFlag() {
    EEPROM.write(EEPROM_OTA_TYPE_ADDR, OTA_TYPE_NO_UPDATE);
}

void loadOTAConfig () {
    char typeValueCh = EEPROM.read(EEPROM_OTA_TYPE_ADDR);
    Serial.print(typeValueCh);
    int otaType = typeValueCh - '0';
    
    if (otaType==OTA_TYPE_LOCAL_UPLOAD) {
        Serial.println("OTA_TYPE_LOCAL_UPLOAD");
        char *cOriginalVersion = (char *) malloc(sizeof(char) * (EEPROM_OTA_CURRENT_VERSION_MAXLENGTH + 1));
        readStringFromEEPROM(cOriginalVersion, EEPROM_OTA_CURRENT_VERSION_ADDR, EEPROM_OTA_CURRENT_VERSION_MAXLENGTH);
        String originalVersion(cOriginalVersion);
        String currentVersion(OPENPCR_FIRMWARE_VERSION_STRING);
        free(cOriginalVersion);
        Serial.println("OriginalVersion=" + originalVersion);
        
        if (currentVersion.equals(originalVersion)) {
            Serial.println("Same version.");
            isUpdateMode = true;
        } else {
            Serial.println("Version Changed.");
            clearOTAFlag();
            EEPROM.commit();
            isUpdateMode = false;
        }
    } else {
        otaType = 0;
        isUpdateMode = false;
    }
    
    Serial.print("OTA Type=");
    Serial.println(otaType);
}

void requestHandlerOTATop () {
    String s = getHTMLHeader();
    s += "<h1>Device Update</h1>\n<ul>";
    s += "<li><a href=\"/update\">Upload new firmware</a></li>";
    s += "<li><a href=\"/cancel\">Cancel (Back to normal mode)</a></li>";
    s += "</ul></body></html>\n";
    server.send(200, "text/html", s);
  
}

void requestHandlerOTACancel () {
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

String getRestartLink () {
  return "<a href=\"/restart\">Restart</a>";
}
void requestHandlerRestart () {
  String s = getHTMLHeader();
  s += "<h1>Device is restarting</h1>\n";
  server.send(200, "text/html", s);
  ESP.restart();
}

void requestHandlerOTAError () {
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
// Possible value is only ot=
void requestHandlerConfig() {
    String type = server.arg("ot");
    Serial.print("type=");
    Serial.print(type);
    saveStringToEEPROM(type, EEPROM_OTA_TYPE_ADDR, 1);
    saveStringToEEPROM(OPENPCR_FIRMWARE_VERSION_STRING, EEPROM_OTA_CURRENT_VERSION_ADDR, EEPROM_OTA_CURRENT_VERSION_MAXLENGTH);
    wifi_send("{accepted:true}", "onConf");
    
    EEPROM.commit();
    ESP.restart();
}
