/* Init wifi */

#include "wifi_conf.h"
#include "wifi_communicator.h"
#include "thermocycler.h"
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

/* OTA */
const char* OTA_HOST = "ninjapcrwifi";
const char* OTA_UPDATE_PATH = "/update";
const char* OTA_UPDATE_USER_NAME = "hoge";
const char* OTA_UPDATE_USER_PASSWORD = "fuga";

bool isUpdateMode = false;

/* Addresses of WiFi configuration */
/* Wfite 0xF0 when WiFi configuration is done. */
#define EEPROM_WIFI_CONF_DONE_ADDR 0//512
#define EEPROM_WIFI_CONF_DONE_VAL 0xF0
#define EEPROM_WIFI_SSID_ADDR (EEPROM_WIFI_CONF_DONE_ADDR+1)
#define EEPROM_WIFI_SSID_MAX_LENGTH 31
#define EEPROM_WIFI_PASSWORD_ADDR (EEPROM_WIFI_CONF_DONE_ADDR+EEPROM_WIFI_SSID_MAX_LENGTH+1)
#define EEPROM_WIFI_PASSWORD_MAX_LENGTH 31

// OTA boot type (0:normal mode, 1:local upload, 2:web download)
#define EEPROM_OTA_TYPE_ADDR  (EEPROM_WIFI_PASSWORD_ADDR+EEPROM_WIFI_PASSWORD_MAX_LENGTH+1)
#define EEPROM_OTA_DOWNLOAD_URL_ADDR (EEPROM_OTA_TYPE_ADDR+1)
#define EEPROM_OTA_DOWNLOAD_URL_MAXLENGTH 128

String PARAM_OTA_TYPE = "ot";
String PARAM_OTA_URL = "ou";
#define OTA_TYPE_NORMAL 0
#define OTA_TYPE_LOCAL_UPLOAD 1
#define OTA_TYPE_WEB_DOWNLOAD 2

/*
 #define EEPROM_WIFI_EMAIL_ADDR (EEPROM_WIFI_PASSWORD_ADDR+EEPROM_WIFI_PASSWORD_MAX_LENGTH+1)
 #define EEPROM_WIFI_EMAIL_MAX_LENGTH 254 // ref. RFC5321 4.5.3.1
 #define EEPROM_WIFI_EMAIL_PASS_ADDR (EEPROM_WIFI_EMAIL_ADDR+EEPROM_WIFI_EMAIL_MAX_LENGTH+1)
 #define EEPROM_WIFI_EMAIL_PASS_MAX_LENGTH 31

 #define EEPROM_WIFI_EMAIL_ERROR_FLAG_ADDR (EEPROM_WIFI_EMAIL_PASS_ADDR+EEPROM_WIFI_EMAIL_PASS_MAX_LENGTH+1)
 #define EEPROM_WIFI_EMAIL_ERROR_FLAG_VAL 0xF0
 #define EEPROM_WIFI_EMAIL_ERROR_MESSAGE_ADDR (EEPROM_WIFI_EMAIL_PASS_ADDR+EEPROM_WIFI_EMAIL_PASS_MAX_LENGTH+2)
 #define EEPROM_WIFI_EMAIL_ERROR_MESSAGE_MAX_LENGTH 128
 #define EEPROM_WIFI_LAST_IP_ADDR  (EEPROM_WIFI_EMAIL_ERROR_MESSAGE_ADDR+EEPROM_WIFI_EMAIL_ERROR_MESSAGE_MAX_LENGTH+1)
 #define EEPROM_WIFI_LAST_IP_EXISTS_VAL 0xF0

 */

// flag 1byte + 4bytes
ESP8266WebServer server(80);

// OTA mode
ESP8266HTTPUpdateServer httpUpdater;

/* t_network_receive_interface */
void wifi_receive() {
    server.handleClient();
}
/* t_network_send_interface */
void wifi_send(char *response, char *funcName) {
    Serial.println("wifi_send");
    char *buff = (char *) malloc(32 + strlen(response) + strlen(funcName));
    sprintf(buff, "DeviceResponse.%s(%s);", funcName, response);
    server.send(200, "text/plain", buff);
    free(buff);
}

/* HTTP request handlers */
/* Handle request to "/" */
void requestHandlerTop() {
    //server.send(200, "text/plain", "requestHandlerTop");
}
/* Handle request to "/command" */
void requestHandlerCommand() {
    Serial.println(server.uri());
    wifi->ResetCommand();
    wifi->SendCommand();
    char buff[256];
    for (uint8_t i = 0; i < server.args(); i++) {
        String sKey = server.argName(i);
        String sValue = server.arg(i);
        Serial.print(sKey);
        Serial.print("->");
        Serial.println(sValue);
        char *key = (char *) malloc(sKey.length() + 1);
        char *value = (char *) malloc(sValue.length() + 1);
        sKey.toCharArray(key, sKey.length() + 1);
        sValue.toCharArray(value, sValue.length() + 1);
        wifi->AddCommandParam(key, value, buff); //TODO
        free(key);
        free(value);
    }
    Serial.println("requestHandlerCommand done");
    Serial.println(buff);
}
/* Handle request to "/status" */
int statusCount = 0;

void requestHandlerStatus() {
    wifi->ResetCommand();
    wifi->RequestStatus();
}

/* Handle request to "/connect" */
void requestHandlerConnect() {
    wifi_send("{connected:true,version:\"1.0.5\"}", "connect");
}

void requestHandlerOTAError () {
    wifi_send("{error:true}", "onErrorOTAMode");

}

/* Handle request to "/config"  (OTA conf) */
void requestHandlerConfig() {
    String type = server.arg("ot"); // Value of dropdown
    String url = server.arg("ou"); // Value of dropdown
    Serial.print("type=");
    Serial.print(type);
    Serial.print(", url=");
    Serial.println(url);
    wifi_send("{accepted:true}", "onConf");
    

    saveStringToEEPROM(type, EEPROM_OTA_TYPE_ADDR, 1);
    saveStringToEEPROM(url, EEPROM_OTA_DOWNLOAD_URL_ADDR, EEPROM_OTA_DOWNLOAD_URL_MAXLENGTH);
    EEPROM.commit();
}

void requestHandlerOTATop () {
    String s = getHTMLHeader();
    s += "<h1>Firmware Update</h1>\n<ul>";
    s += "<li><a href=\"/update\">Update by local upload</a></li>";
    s += "<li><a href=\"/cancel\">Cancel (Back to normal mode)</a></li>";
    s += "</ul></body></html>\n";
    server.send(200, "text/html", s);
  
}
void requestHandlerOTACancel () {
    String s = getHTMLHeader();
    s += "<h1>Firmware Update</h1>\n<ul>";
    s += "<p>Firmware update is canceled. Please restart the device.</p></body></html>\n";
    server.send(200, "text/html", s);
    EEPROM.write(EEPROM_OTA_TYPE_ADDR, OTA_TYPE_NORMAL);
    EEPROM.commit();
  
}

void requestHandler404() {
    server.send(404, "text/plain", "requestHandler404");
}


/* Just load and print */
int otaType = 0;
String otaURL;
void loadOTAConfig () {

    char typeValueCh = EEPROM.read(EEPROM_OTA_TYPE_ADDR);
    Serial.print(typeValueCh);
    int otaType = typeValueCh - '0';
    
    Serial.print("OTA ch=");
    Serial.println(otaType);
    if (otaType==OTA_TYPE_LOCAL_UPLOAD) {
        isUpdateMode = true;
    } else if (otaType==OTA_TYPE_WEB_DOWNLOAD) {
        char *urlValue = (char *) malloc(sizeof(char) * (EEPROM_OTA_DOWNLOAD_URL_MAXLENGTH + 1));
        readStringFromEEPROM(urlValue, EEPROM_OTA_DOWNLOAD_URL_ADDR, EEPROM_OTA_DOWNLOAD_URL_MAXLENGTH);
        String str(urlValue);
        otaURL = str;
        free(urlValue);
        Serial.print("OTA URL=");
        Serial.println(otaURL);
        isUpdateMode = true;
    } else {
        otaType = 0;
        isUpdateMode = false;
    }
    
    Serial.print("OTA Type=");
    Serial.println(otaType);
}

/* AP scanning (to show AP list) */
// Find nearby SSIDs
int apCount = 0;
const int AP_MAX_NUMBER = 54;
String SSIDs[AP_MAX_NUMBER];
void scanNearbyAP() {
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
    }
    else {
        Serial.print(n);
        Serial.println(" networks found");
        if (n > AP_MAX_NUMBER) {
            n = AP_MAX_NUMBER;
        }
        apCount = 0;
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            String ssid = WiFi.SSID(i);
            for (int j = 0; j < apCount; j++) {
                if (ssid == SSIDs[apCount]) {
                    Serial.println("Duplicate");
                    break;
                }
            }
            Serial.print(apCount + 1);
            Serial.print(": ");
            Serial.print(ssid);
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")\n");
            SSIDs[apCount] = ssid;
            delay(1);
            apCount++;
            if (apCount >= AP_MAX_NUMBER) {
                break;
            }
        }
    }
    Serial.println("Scan done.");
}
void startScanMode() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(10);
}
const IPAddress apIP(192, 168, 1, 1); // http://192.168.1.1
void stopScanMode() {
}

// NinjaPCR works as an AP with this SSID
const char* apSSID = "NinjaPCR";

void startAPMode() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(apSSID);
}

/* Scan for nearby APs -> save APs -> start AP */
bool hasPrevWifiError = false;
String prevWifiError;

void startWiFiAPMode() {
    startScanMode();
    while (apCount == 0) {
        scanNearbyAP();
        delay(3000);
    }
    stopScanMode();
    startAPMode();
    server.begin();
    server.on("/", requestHandlerConfInit);
    server.on("/join", requestHandlerConfJoin);
    server.onNotFound(requestHandler404);
    Serial.println("OK");
    Serial.println("1. Connect API \"NinjaPCR\"");
    Serial.println("2. Access http://192.168.1.1");
}
boolean isWiFiConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

/* Check WiFi Connection in EEPROM */
bool isWifiConfDone() {
    return EEPROM.read(EEPROM_WIFI_CONF_DONE_ADDR) == EEPROM_WIFI_CONF_DONE_VAL;
}
int min(int a, int b) {
    return (a < b) ? a : b;
}
void saveStringToEEPROM(String str, int startAddress, int maxLength) {
    int len = min(str.length(), maxLength);
    Serial.print("to EEPROM ");
    Serial.print(len);
    Serial.print("bytes @");
    Serial.println(startAddress);
    for (int i = 0; i < len; i++) {
        EEPROM.write(startAddress + i, str.charAt(i));
    }
    EEPROM.write(startAddress + len, 0x00); // Write \0
}

void saveWiFiConnectionInfo(String ssid, String password) {
    EEPROM.write(EEPROM_WIFI_CONF_DONE_ADDR, EEPROM_WIFI_CONF_DONE_VAL);
    EEPROM.write(EEPROM_OTA_TYPE_ADDR, OTA_TYPE_NORMAL);
    saveStringToEEPROM(ssid, EEPROM_WIFI_SSID_ADDR,
            EEPROM_WIFI_SSID_MAX_LENGTH);
    saveStringToEEPROM(password, EEPROM_WIFI_PASSWORD_ADDR,
            EEPROM_WIFI_PASSWORD_MAX_LENGTH);
    EEPROM.commit();
}

/* Handle access to software AP */
#define PAGE_INIT 1
#define PAGE_JOIN 2
// Returned string ends with <body> tag.
String getHTMLHeader() {
    return "<!DOCTYPE HTML>\r\n<html><head><title>NinjaPCR</title></head><body>\r\n";
}
void requestHandlerConfInit() {
    // Send form
    String s = getHTMLHeader();
    if (hasPrevWifiError) {
        s += "<div style=\"color:red\">" + prevWifiError + "</div>";
    }
    s += "<form method=\"post\" action=\"join\">";
    s += "<div>SSID:<select name=\"s\"><option>----</option>";
    for (int i = 0; i < apCount; i++) {
        String ssid = SSIDs[i];
        s += "<option value=\"" + ssid + "\">" + ssid + "</option>";
    }
    s += "</select></div>";
    s += "<div>SSID (text): <input type=\"password\" name=\"st\"/></div>";
    s += "<div>Password: <input type=\"password\" name=\"wp\"/></div>";
    s += "<div><input type=\"submit\" value=\"Join\"/></div>";
    s += "</form>";
    s += "</body></html>\n";
    server.send(200, "text/html", s);
}
String BR_TAG = "<br/>";
void requestHandlerConfJoin() {
    Serial.println("requestHandlerConfJoin");
    String ssid = server.arg("s"); // Value of dropdown
    String password = server.arg("wp");
    String emptyField = "";
    bool isValid = true;
    if (ssid == "") {
        // Free text
        ssid = server.arg("st");
    }
    if (ssid == "") {
        emptyField = "WiFi SSID";
        isValid = false;
    }
    if (password == "") {
        emptyField = "WiFi Password";
        isValid = false;
    }

    String s = getHTMLHeader();
    if (!isValid) {
        // Has error
        s += BR_TAG;
        s += "ERROR: " + emptyField + " is empty.";
    }
    else {
        s += "Join.";
        s += "<div>SSID:" + ssid + BR_TAG;
        s += "Password:" + password + BR_TAG;
        s += "Please reset.";
    }
    s += "</body></html>\n";
    server.send(200, "text/html", s);

    if (isValid) {
        Serial.println("Valid input. Saving...");
        saveWiFiConnectionInfo(ssid, password);
        Serial.println("saved.");
    }
    else {
        Serial.println("Invalid input.");
    }
}

void readStringFromEEPROM(char *s, int startAddress, int maxLength) {
    int index = 0;
    while (index < maxLength) {
        char c = EEPROM.read(index + startAddress);
        s[index++] = c;
        if (c == 0x00)
            return;
    }
    s[maxLength] = 0x00;
}

/*
 * Check IP address 
 * -> Compare with previous IP address 
 * -> Send new IP address via Lambda function
 *    (http://d3n332182mb98i.cloudfront.net/?i=XXX&m=XXX&p=XXX)
 */
WiFiClientSecure httpsClient;
String byteToHexStr(char c) {
    char s[3];
    sprintf(s, "%02X", c);
    return String(s);
}

#define WIFI_TIMEOUT_SEC 10
/* Start network as a HTTP server */
boolean startWiFiHTTPServer() {
    // Check existence of WiFi Config
    if (!isWifiConfDone()) {
        Serial.println("WiFi config not found.");
        return false;
    }

    // Load connection info from EEPROM
    int ssidIndex = 0;
    char *ssid = (char *) malloc(
            sizeof(char) * (EEPROM_WIFI_SSID_MAX_LENGTH + 1));
    char *password = (char *) malloc(
            sizeof(char) * (EEPROM_WIFI_PASSWORD_MAX_LENGTH + 1));
    readStringFromEEPROM(ssid, EEPROM_WIFI_SSID_ADDR,
            EEPROM_WIFI_SSID_MAX_LENGTH);
    readStringFromEEPROM(password, EEPROM_WIFI_PASSWORD_ADDR,
            EEPROM_WIFI_PASSWORD_MAX_LENGTH);
    // Connect with saved SSID and password
    Serial.print("SSID:");
    Serial.println(ssid);
    Serial.print("Pass:");
    Serial.println(password);
    WiFi.begin(ssid, password);

    // Wait for connection establishment
    int wifiTime = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        wifiTime++;
        if (wifiTime > WIFI_TIMEOUT_SEC * 2) {
            // failure
            return false;
        }
        Serial.print(".");
    }
    free(ssid);
    free(password);

    Serial.print("\nConnected.IP=");
    Serial.println(WiFi.localIP());

    Serial.println("Load OTA conf.");
    loadOTAConfig();
    if (isUpdateMode) {
        startUpdaterServer();
    }
    else {
        startNormalOperationServer();
    }
    return true;
}
void startUpdaterServer() {
    // OTA mode
    Serial.println("Starting OTA mode (isUpdateMode=true)");
    MDNS.begin(OTA_HOST);
    httpUpdater.setup(&server, OTA_UPDATE_PATH, OTA_UPDATE_USER_NAME, OTA_UPDATE_USER_PASSWORD);
    server.on("/", requestHandlerOTATop);
    server.on("/cancel", requestHandlerOTACancel);
    server.on("/command", requestHandlerOTAError);
    server.on("/status", requestHandlerOTAError);
    server.on("/connect", requestHandlerOTAError);
    server.on("/config", requestHandlerOTAError);
    server.begin();
    MDNS.addService("http", "tcp", 80);
    Serial.printf(
            "HTTPUpdateServer ready! Open http://%s.local in your browser and login with username '%s' and password '%s'\n",
            OTA_HOST, OTA_UPDATE_USER_NAME,
            OTA_UPDATE_USER_PASSWORD);
}
void startNormalOperationServer() {
    // Normal PCR operation mode
    /* Add handlers for paths */
    server.on("/", requestHandlerTop);
    server.on("/command", requestHandlerCommand);
    server.on("/status", requestHandlerStatus);
    server.on("/connect", requestHandlerConnect);
    server.on("/config", requestHandlerConfig);
    server.onNotFound(requestHandler404);

    server.begin();
    Serial.println("Server started");
}

/* Handle HTTP request as a server */
void loopWiFiHTTPServer() {
    server.handleClient();
}
/* Handle HTTP request as AP */
void loopWiFiAP() {
    server.handleClient();
}
