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

/* Start network as an access point */
boolean network_ap_start() {
    
    
}
int apCount = 0;
const int AP_MAX_NUMBER = 16;
String SSIDs[AP_MAX_NUMBER];

/* Find nearby SSIDs to show in config view */
void scan() {
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
    delay(100);
}
const char* apSSID = "NinjaPCR WiFi";

void startAPMode() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(apSSID);
}

void startServer() {
    server.begin();
    Serial.println("Server Started.");
}
void setup_ap_mode () {
    startScanMode();
    while (apCount == 0) {
        scan();
        delay(3000);
    }
    stopScanMode();
    startAPMode();
    startServer();
}
/* Start network as a HTTP server */
boolean network_start() {
    WiFi.begin(ssid, password);
    Serial.print("SSID:");
    Serial.println(ssid);
    Serial.print("Pass:");
    Serial.println(password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.print("\nConnected.IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/", requestHandlerTop);
    server.on("/command", requestHandlerCommand);
    server.on("/status", requestHandlerStatus);
    server.on("/connect", requestHandlerConnect);

    server.onNotFound(requestHandler404);
    Serial.println("Server started");
    server.begin();
    return true;
}

/* Handle HTTP request as a server */
void network_loop() {
    //server.handleClient();
}
boolean network_is_connected () {
    return (WiFi.status() == WL_CONNECTED);
}

#define EEPROM_WIFI_SSID_ADDR 512
#define EEPROM_WIFI_SSID_MAX_LENGTH 31
#define EEPROM_WIFI_PASSWORD_ADDR 512+32
#define EEPROM_WIFI_PASSWORD_MAX_LENGTH 31

void saveWiFiConnectionInfo(String ssid, String password) {
    for (int i = 0, l = ssid.length(); i < l; i++) {
        EEPROM.write(EEPROM_WIFI_SSID_ADDR+i, ssid.charAt(i));
    }
    EEPROM.write(EEPROM_WIFI_SSID_ADDR+ssid.length(), 0x00);
    for (int i = 0, l = password.length(); i < l; i++) {
        EEPROM.write(EEPROM_WIFI_PASSWORD_ADDR+i, password.charAt(i));
    }
}

/* Handle access to software AP */
void loop_ap_mode () {
    WiFiClient client = server.available();
    if (!client) {
        return;
    }

    String ssid = "";
    String password = "";

    // Read the first line of the request
    String req = client.readStringUntil('\r');
    while (client.available()) {
        String str = client.readStringUntil('\r');
        // ssid=au_Wi-Fi&password=aaa
        if (str.indexOf("ssid=") != -1 && str.indexOf("password=") != -1) {
            ssid = getParamFromString(str, "ssid");
            password = getParamFromString(str, "password");
        }
    }
    Serial.println(req);
    client.flush();

    // Match the request
    int val = -1; // We'll use 'val' to keep track of both the
    // request type (read/set) and value if set.

    int page = 0;
    if (req.indexOf("/init") != -1) {
        page = PAGE_INIT;
    }
    else if (req.indexOf("/join") != -1) {
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
        s += "<div><input type=\"submit\" value=\"Join\"/></div>";
        s += "</form>";
    }
    else if (page == PAGE_JOIN) {
        s += "Join.";
        s += "<div>SSID:" + ssid + "</div>";
        s += "<div>Password:" + password + "</div>";
    }
    s += "</body></html>\n";

    // Send the response to the client
    client.print(s);
    delay(1);
    Serial.println("Client disonnected");
    Serial.println(s);

    if (page == PAGE_JOIN) {
        saveWiFiConnectionInfo(ssid, password);
    }
}