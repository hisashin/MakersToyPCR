/* Init wifi */

#include "wifi_conf.h"
#include "wifi_communicator.h"
#include "thermocycler.h"
#include <EEPROM.h>



ESP8266WebServer server(80);

/* t_network_receive_interface */
void wifi_receive () {
  server.handleClient();
}
/* t_network_send_interface */
void wifi_send (char *response, char *funcName) {
  Serial.println("wifi_send");
  char *buff = (char *) malloc(32 + strlen(response) + strlen(funcName));
  sprintf(buff, "DeviceResponse.%s(\"%s\");", funcName, response);
  server.send(200, "text/plain", buff);
  free(buff);
}

/* HTTP request handlers */
/* Handle request to "/" */
void requestHandlerTop () {
    //server.send(200, "text/plain", "requestHandlerTop");
}
/* Handle request to "/command" */
void requestHandlerCommand() {
    Serial.println(server.uri());
    wifi->ResetCommand();
    wifi->SendCommand();
    char buff[256];
    for (uint8_t i= 0; i<server.args(); i++) {
        String sKey = server.argName(i);
        String sValue = server.arg(i);
        Serial.print(sKey);
        Serial.print("->");
        Serial.println(sValue);
        char *key = (char *) malloc(sKey.length()+1);
        char *value = (char *) malloc(sValue.length()+1);
        sKey.toCharArray(key, sKey.length()+1);
        sValue.toCharArray(value, sValue.length()+1);
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
  wifi_send("{connected:true,version:\"1.0.5\"}","connect");
}

void requestHandler404 () {
    server.send(404, "text/plain", "requestHandler404");
}


/* AP scanning (to show AP list) */
// Find nearby SSIDs
int apCount = 0;
const int AP_MAX_NUMBER = 16;
String SSIDs[AP_MAX_NUMBER];
void scanNearbyAP() {
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    apCount = n;
    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
    }
    else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println(
                    (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            SSIDs[i] = WiFi.SSID(i);
            delay(10);
        }
    }
    Serial.println("Scan done.");
}
void startScanMode() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(10);
}
void stopScanMode() {
}

// NinjaPCR works as an AP with this SSID
const char* apSSID = "NinjaPCR WiFi";

void startAPMode() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(apSSID);
}

/* Scan for nearby APs -> save APs -> start AP */
void startWiFiAPMode () {
    startScanMode();
    while (apCount == 0) {
        scanNearbyAP();
        delay(3000);
    }
    stopScanMode();
    startAPMode();
    server.begin();
    Serial.println("Server Started.");
}
boolean isWiFiConnected () {
    return (WiFi.status() == WL_CONNECTED);
}

/* Addresses of WiFi configuration */
/* Wfite 0xF0 when WiFi configuration is done. */
#define EEPROM_WIFI_CONF_DONE_ADDR 512
#define EEPROM_WIFI_CONF_DONE_VAL 0xF0
#define EEPROM_WIFI_SSID_ADDR 512+1
#define EEPROM_WIFI_SSID_MAX_LENGTH 31
#define EEPROM_WIFI_PASSWORD_ADDR (512+32)
#define EEPROM_WIFI_PASSWORD_MAX_LENGTH 31
#define EEPROM_WIFI_EMAIL_ADDR (512+32+32)
#define EEPROM_WIFI_EMAIL_MAX_LENGTH 254 // ref. RFC5321 4.5.3.1

#define EEPROM_WIFI_LAST_IP_ADDR  (512+32+32+255)
#define EEPROM_WIFI_LAST_IP_EXISTS_VAL 0xF0
// flag 1byte + 4bytes

/* Check WiFi Connection in EEPROM */
bool isWifiConfDone () {
    return EEPROM.read(EEPROM_WIFI_CONF_DONE_ADDR) == EEPROM_WIFI_CONF_DONE_VAL;
}

void saveStringToEEPROM (String str, int startAddress, int maxLength) {
    int length = min(str.length(), maxLength);
    for (int i = 0, l = str.length(); i < l; i++) {
        EEPROM.write(startAddress+i, str.charAt(i));
    }
    EEPROM.write(startAddress+length, 0x00); // Write \0
}

void saveWiFiConnectionInfo(String ssid, String password, String email) {
    EEPROM.write(EEPROM_WIFI_CONF_DONE_ADDR, EEPROM_WIFI_CONF_DONE_VAL);
    
    saveStringToEEPROM(ssid, EEPROM_WIFI_SSID_ADDR, EEPROM_WIFI_SSID_MAX_LENGTH);
    saveStringToEEPROM(password, EEPROM_WIFI_PASSWORD_ADDR, EEPROM_WIFI_PASSWORD_MAX_LENGTH);
    saveStringToEEPROM(email, EEPROM_WIFI_EMAIL_ADDR, EEPROM_WIFI_EMAIL_MAX_LENGTH);
}

/* Handle access to software AP */
void loopWiFiAP () {
    WiFiClient client = server.available();
    if (!client) {
        return;
    }

    String ssid = "";
    String password = "";
    String email = "";
    // Read the first line of the request
    String req = client.readStringUntil('\r');
    while (client.available()) {
        String str = client.readStringUntil('\r');
        // ssid=au_Wi-Fi&password=aaa
        if (str.indexOf("ssid=") != -1 && str.indexOf("password=") != -1) {
            ssid = getParamFromString(str, "ssid");
            password = getParamFromString(str, "password");
            email = getParamFromString(str, "email");
        }
    }
    Serial.println(req);
    client.flush();
    
    int page = 0;
    if (req.indexOf("/init") != -1) {
        page = PAGE_INIT;
    } else if (req.indexOf("/join") != -1) {
        page = PAGE_JOIN;
    }

    client.flush();

    // Prepare the response. Start with the common header:
    String s = "HTTP/1.1 200 OK\r\n";
    s += "Content-Type: text/html\r\n\r\n";
    s += "<!DOCTYPE HTML>\r\n<html><head>NinjaPCR</head><body>\r\n";

    if (page == PAGE_INIT) {
        s += "<form method=\"post\" action=\"join\">";
        s += "<div>SSID:<select name=\"ssid\"><option>----</option>";
        for (int i = 0; i < apCount; i++) {
            String ssid = SSIDs[i];
            s += "<option value=\"" + ssid + "\">" + ssid + "</option>";
        }
        s += "</select></div>";
        s += "<div>Password: <input type=\"text\" name=\"password\"/></div>";
        s += "<div>Mail address: <input type=\"text\" name=\"email\"/></div>";
        s += "<div><input type=\"submit\" value=\"Join\"/></div>";
        s += "</form>";
    }
    else if (page == PAGE_JOIN) {
        s += "Join.";
        s += "<div>SSID:" + ssid + "</div>";
        s += "<div>Password:" + password + "</div>";
        s += "<div>Email:" + email + "</div>";
    }
    s += "</body></html>\n";

    // Send the response to the client
    client.print(s);
    delay(1);
    Serial.println("Client disonnected");
    Serial.println(s);

    if (page == PAGE_JOIN && ssid.length()>0 && password.length()>0 && email.length()>0) {
        saveWiFiConnectionInfo(ssid, password, email);
    }
}
void readStringFromEEPROM (char *s, int startAddress, int maxLength) {
    int index = 0;
    while (index < maxLength) {
        char c = EEPROM.read(index+startAddress);
        s[index++] = c;
        if (c==0x00) return;
    }
    s[maxLength] = 0x00;
}
/* Start network as a HTTP server */
boolean startWiFiHTTPServer() {
    // Check existence of WiFi Config
    if (!isWifiConfDone ()) {
        Serial.println("Network can't start. WiFi config not found.");
        return;
    }
    // Load connection info from EEPROM
    int ssidIndex = 0;
    char *ssid = malloc(sizeof(char) * (EEPROM_WIFI_SSID_MAX_LENGTH+1));
    char *password = malloc(sizeof(char) * (EEPROM_WIFI_PASSWORD_MAX_LENGTH+1));
    readStringFromEEPROM(ssid, EEPROM_WIFI_SSID_ADDR, EEPROM_WIFI_SSID_MAX_LENGTH);
    readStringFromEEPROM(password, EEPROM_WIFI_PASSWORD_ADDR, EEPROM_WIFI_PASSWORD_MAX_LENGTH);
    // Connect with saved SSID and password
    Serial.print("SSID:"); Serial.println(ssid);
    Serial.print("Pass:"); Serial.println(password);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    free (ssid, password);
    Serial.print("\nConnected.IP address: ");
    Serial.println(WiFi.localIP());
    
    char *email = malloc(sizeof(char) * (EEPROM_WIFI_EMAIL_MAX_LENGTH+1));
    Serial.print("Email:"); Serial.println(email);
    readStringFromEEPROM(email, EEPROM_WIFI_EMAIL_ADDR, EEPROM_WIFI_EMAIL_MAX_LENGTH);
    char prevIP = malloc(sizeof(char) * 4);
    
    bool ipAddressChanged = false;
    if (EEPROM.read(EEPROM_WIFI_LAST_IP_ADDR)==EEPROM_WIFI_LAST_IP_EXISTS_VAL) {
        for (int i=0; i<4; i++) {
            if (EEPROM.read(EEPROM_WIFI_LAST_IP_ADDR+1+i)!=WiFi.localIP()[i]) {
                ipAddressChanged = true;
            }
        }
    } else {
        ipAddressChanged = true;
    }
    if (ipAddressChanged) {
        // Send IP with lambda function
        // TODO call lambda function
        // TODO check last IP
    }
    free(email);
    free(prevIP);

    /* Add handlers for paths */
    server.on("/", requestHandlerTop);
    server.on("/command", requestHandlerCommand);
    server.on("/status", requestHandlerStatus);
    server.on("/connect", requestHandlerConnect);
    server.onNotFound(requestHandler404);
    
    server.begin();
    Serial.println("Server started");
    return true;
}

/* Handle HTTP request as a server */
void loopWiFiHTTPServer () {
    //server.handleClient();
}