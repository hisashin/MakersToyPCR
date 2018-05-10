/**
 * OTA
 */

// OTA boot type (0:normal mode, 1:local upload, 2:web download)
#define EEPROM_OTA_TYPE_ADDR  (EEPROM_WIFI_MDNS_HOST_ADDR+EEPROM_WIFI_MDNS_HOST_MAX_LENGTH+1)
#define EEPROM_OTA_DOWNLOAD_URL_ADDR (EEPROM_OTA_TYPE_ADDR+1)
#define EEPROM_OTA_DOWNLOAD_URL_MAXLENGTH 128

String PARAM_OTA_TYPE = "ot";
String PARAM_OTA_URL = "ou";
#define OTA_TYPE_NO_UPDATE 0 /* No update (boot with normal PCR mode) */
#define OTA_TYPE_LOCAL_UPLOAD 1
#define OTA_TYPE_WEB_DOWNLOAD 2

const char* OTA_HOST = "ninjapcrwifi";
const char* OTA_UPDATE_PATH = "/update";


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
        isUpdateMode = true;
    } else if (otaType==OTA_TYPE_WEB_DOWNLOAD) {
        // This functionality is disabled now.
        char *urlValue = (char *) malloc(sizeof(char) * (EEPROM_OTA_DOWNLOAD_URL_MAXLENGTH + 1));
        readStringFromEEPROM(urlValue, EEPROM_OTA_DOWNLOAD_URL_ADDR, EEPROM_OTA_DOWNLOAD_URL_MAXLENGTH);
        Serial.println("OTA_TYPE_WEB_DOWNLOAD");
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

void requestHandlerOTATop () {
    String s = getHTMLHeader();
    s += "<h1>Device Update</h1>\n<ul>";
    s += "<li><a href=\"/update\">Update by local upload</a></li>";
    s += "<li><a href=\"/cancel\">Cancel (Back to normal mode)</a></li>";
    s += "</ul></body></html>\n";
    server.send(200, "text/html", s);
  
}
void requestHandlerOTACancel () {
    String s = getHTMLHeader();
    s += "<h1>Device Update</h1>\n<ul>";
    s += "<p>Device update is canceled. Please restart the device.</p></body></html>\n";
    server.send(200, "text/html", s);
    EEPROM.write(EEPROM_OTA_TYPE_ADDR, OTA_TYPE_NO_UPDATE);
    EEPROM.commit();
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
    server.begin();
    Serial.printf("HTTPUpdateServer ready!");
}

/* Handle request to "/config"  (OTA conf) */
// Possible value is only ot=
void requestHandlerConfig() {
    String type = server.arg("ot"); // Value of dropdown
    String url = server.arg("ou"); // Value of dropdown
    Serial.print("type=");
    Serial.print(type);
    Serial.print(", url=");
    Serial.println(url);
    saveStringToEEPROM(type, EEPROM_OTA_TYPE_ADDR, 1);
    saveStringToEEPROM(url, EEPROM_OTA_DOWNLOAD_URL_ADDR, EEPROM_OTA_DOWNLOAD_URL_MAXLENGTH);
    wifi_send("{accepted:true}", "onConf");
    
    EEPROM.commit();
}
