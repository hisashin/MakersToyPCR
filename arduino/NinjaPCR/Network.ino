/* Init wifi */

// #include "wifi_conf.h"
#include "wifi_communicator.h"
#include "thermocycler.h"
#include <EEPROM.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>


bool isUpdateMode = false;
String DEFAULT_HOST_NAME = "NinjaPCR";
String hostName = DEFAULT_HOST_NAME;

/* Addresses of WiFi configuration */
/* Wfite 0xF0 when WiFi configuration is done. */
#define EEPROM_WIFI_CONF_DONE_ADDR 0
#define EEPROM_WIFI_CONF_DONE_VAL 0xF0

#define EEPROM_WIFI_SSID_ADDR (EEPROM_WIFI_CONF_DONE_ADDR+1)
#define EEPROM_WIFI_SSID_MAX_LENGTH 31

#define EEPROM_WIFI_PASSWORD_ADDR (EEPROM_WIFI_SSID_ADDR+EEPROM_WIFI_SSID_MAX_LENGTH+1)
#define EEPROM_WIFI_PASSWORD_MAX_LENGTH 31

#define EEPROM_WIFI_MDNS_HOST_ADDR (EEPROM_WIFI_PASSWORD_ADDR+EEPROM_WIFI_PASSWORD_MAX_LENGTH+1)
#define EEPROM_WIFI_MDNS_HOST_MAX_LENGTH 31

// flag 1byte + 4bytes
ESP8266WebServer server(80);

/* t_network_receive_interface */
void wifi_receive() {
    server.handleClient();
}
/* t_network_send_interface */
void wifi_send(char *response, char *funcName) {
    String str = "DeviceResponse.";
    str += String(funcName);
    str += "(";
    str += String(response);
    str += ");";
    char *buff = (char*)malloc(sizeof(char) * (str.length()+1));
    str.toCharArray(buff, str.length()+1);
    Serial.print("WiFi Sendss");
    Serial.println(buff);
    server.send(200, "text/plain", buff);
    free(buff);
    Serial.println("sent.");
}

/* HTTP request handlers */
/* Handle request to "/" */
void requestHandlerTop() {
    //server.send(200, "text/plain", "requestHandlerTop");
}
/* Handle request to "/command" */
void requestHandlerCommand() {
    Serial.print("rc.");
    Serial.println(server.uri());
    Serial.print("1");
    wifi->ResetCommand();
    Serial.print("2");
    wifi->SendCommand();
    Serial.print("3");
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
    boolean isRunning = false;
    if (gpThermocycler->GetProgramState() == Thermocycler::ProgramState::EStartup ||
      gpThermocycler->GetProgramState() == Thermocycler::ProgramState::ELidWait ||
      gpThermocycler->GetProgramState() == Thermocycler::ProgramState::ERunning ||
      gpThermocycler->GetProgramState() == Thermocycler::ProgramState::EComplete) {
      isRunning = true;
    }
    String response = "{connected:true,version:\"";
    response += String(OPENPCR_FIRMWARE_VERSION_STRING);
    if (isRunning) {
      response += "\",running:true}";
    } else {
      response += "\",running:false}";
    }
    char *chResponse = (char *) malloc(sizeof(char) * (response.length()+1));
    response.toCharArray(chResponse, response.length()+1);
    wifi_send(chResponse, "connect");
    free(chResponse);
}


void requestHandler404() {
    server.send(404, "text/plain", "requestHandler404");
}


/* AP scanning (to show AP list) */
// Find nearby SSIDs
int apCount = 0;
const int AP_MAX_NUMBER = 54;
String SSIDs[AP_MAX_NUMBER];
void scanNearbyAP() {
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    if (n == 0) {
        Serial.println("no networks found");
    }
    else {
        Serial.print(n);
        Serial.println(" found");
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
    Serial.println("1. Connect AP \"NinjaPCR\"");
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

void saveWiFiConnectionInfo(String ssid, String password, String host) {
    // Flags
    EEPROM.write(EEPROM_WIFI_CONF_DONE_ADDR, EEPROM_WIFI_CONF_DONE_VAL);
    clearOTAFlag();
    // Values
    saveStringToEEPROM(ssid, EEPROM_WIFI_SSID_ADDR,
            EEPROM_WIFI_SSID_MAX_LENGTH);
    saveStringToEEPROM(password, EEPROM_WIFI_PASSWORD_ADDR,
            EEPROM_WIFI_PASSWORD_MAX_LENGTH);
    saveStringToEEPROM(host, EEPROM_WIFI_MDNS_HOST_ADDR,
            EEPROM_WIFI_MDNS_HOST_MAX_LENGTH);
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
    s += "<div>Host name: <input name=\"h\" value=\"" + hostName + "\"/></div>";
    s += "<div><input type=\"submit\" value=\"Join\"/></div>";
    s += "</form>";
    s += "</body></html>\n";
    server.send(200, "text/html", s);
}
String BR_TAG = "<br/>";
bool isValidHostName (String host) {
    // Length (1 to EEPROM_WIFI_MDNS_HOST_MAX_LENGTH) a-zA-Z0-9, "-"
    if (host.length() < 4 || host.length() > EEPROM_WIFI_MDNS_HOST_MAX_LENGTH) {
        Serial.println("Invalid Length");
        return false;
    }
    for (int i=0; i<host.length(); i++) {
        char c = host.charAt(i);
        if (!( ('A'<=c&&c<='Z') || ('a'<=c&&c<='z') || ('0'<=c&&c<='9') || c=='-')) {
            Serial.print("Invalid char:");
            Serial.println(c);
            return false;
        }
    }
    // Characters
    return true;
}
void requestHandlerConfJoin() {
    Serial.println("requestHandlerConfJoin");
    String ssid = server.arg("s"); // Value of dropdown
    String password = server.arg("wp");
    String host = server.arg("h");
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
    if (!isValidHostName(host)) {
        emptyField = "Host name";
        isValid = false;
        
    }

    String s = getHTMLHeader();
    if (!isValid) {
        // Has error
        s += BR_TAG;
        s += "ERROR: " + emptyField + " is empty.";
    }
    else {
        s += "<div>SSID:" + ssid + BR_TAG;
        s += "Password: ******" + BR_TAG;
        s += "Please reset.";
    }
    s += "</body></html>\n";
    server.send(200, "text/html", s);

    if (isValid) {
        Serial.println("Valid input. Saving...");
        saveWiFiConnectionInfo(ssid, password, host);
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
    char *host = (char *) malloc(
                sizeof(char) * (EEPROM_WIFI_MDNS_HOST_MAX_LENGTH + 1));
    
    readStringFromEEPROM(ssid, EEPROM_WIFI_SSID_ADDR,
            EEPROM_WIFI_SSID_MAX_LENGTH);
    readStringFromEEPROM(password, EEPROM_WIFI_PASSWORD_ADDR,
            EEPROM_WIFI_PASSWORD_MAX_LENGTH);
    readStringFromEEPROM(host, EEPROM_WIFI_MDNS_HOST_ADDR,
            EEPROM_WIFI_MDNS_HOST_MAX_LENGTH);
    // Connect with saved SSID and password
    Serial.print("SSID:");
    Serial.println(ssid);
    Serial.print("Pass:");
    Serial.println(password);
    Serial.print("Host:");
    Serial.println(host);
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
    Serial.println("Connected.");
    //slack_send("NinjaPCR_test 1");
    beginMDNS(host);
    Serial.println("MDNS ok.");
    free(ssid);
    free(password);
    free(host);

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
int beginMDNS (const char *hostName) {
    MDNS.begin(hostName);
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS started.");
    Serial.println(hostName);
}
void startNormalOperationServer() {
    // Normal PCR operation mode
    /* Add handlers for paths */
    
    // Never remove it (for OTA configuration)
    server.on("/config", requestHandlerConfig);
    
    server.on("/", requestHandlerTop);
    server.on("/command", requestHandlerCommand);
    server.on("/status", requestHandlerStatus);
    server.on("/connect", requestHandlerConnect);
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
