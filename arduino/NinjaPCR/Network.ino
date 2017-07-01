

/* Init wifi */

// TODO dummy (Manage SSID and password with interactive serial interface and EEPROM)
/*
char ssid[] = "dummy_ssid";
char password[] = "dummy_pass";
*/
#include "wifi_conf.h"
ESP8266WebServer server(80);

/* HTTP request handlers */
/* Handle request to "/" */
void requestHandlerTop () {
    server.send(200, "text/plain", "requestHandlerTop");
}
/* Handle request to "/command" */
void requestHandlerCommand() {
    
    server.send(200, "text/plain", "requestHandlerCommand");

    Serial.println(server.uri());
    for (uint8_t i= 0; i<server.args(); i++) {
        Serial.print(server.argName(i));
        Serial.print("->");
        Serial.println(server.arg(i));
    }
    // TODO CommandParser::ParseCommand
}
/* Handle request to "/status" */
int statusCount = 0;
void requestHandlerStatus() {
    char response[50];
    sprintf(response, "updateStatus(%d);", statusCount++);
    server.send(200, "text/plain", response);
    // TODO SerialControl::SendStatus
}

void requestHandler404 () {
    server.send(404, "text/plain", "requestHandler404");
}

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

    Serial.println("network 1");
    server.on("/", requestHandlerTop);
    Serial.println("network 2");
    server.on("/command", requestHandlerCommand);
    Serial.println("network 3");
    server.on("/status", requestHandlerStatus);
    Serial.println("network 4");

    server.onNotFound(requestHandler404);
    Serial.println("network 5");
    server.begin();
    return true;
}

/* Handle HTTP request as a server */
void network_loop() {
    server.handleClient();
}
boolean network_is_connected () {
    return (WiFi.status() == WL_CONNECTED);
}
