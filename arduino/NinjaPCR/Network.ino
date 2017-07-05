/* Init wifi */

#include "wifi_conf.h"
#include "wifi_communicator.h"
#include "thermocycler.h"

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
    
    //server.send(200, "text/plain", "requestHandlerCommand");

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
  /*
    char response[50];
    sprintf(response, "updateStatus(%d);", statusCount++);
    server.send(200, "text/plain", response);
    // TODO SerialControl::SendStatus
    */
    wifi->ResetCommand();
    wifi->RequestStatus();
}
/* Handle request to "/connect" */
void requestHandlerConnect() {
  wifi_send("true","connect");
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
