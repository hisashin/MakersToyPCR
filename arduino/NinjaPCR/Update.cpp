#include <ESP8266HTTPUpdate.h>
#include "Update.h"

NinjaUpdate::NinjaUpdate() :
        ESP8266HTTPUpdate() {
}

HTTPUpdateResult NinjaUpdate::update(const String& url, const String& md5,
        const String& currentVersion) {
    DEBUG_HTTP_UPDATE("[NinjaPCR] NinjaUpdate::update\n");
    expectedMD5 = md5;
    return ESP8266HTTPUpdate::update(url, currentVersion);
}
/* Override */
HTTPUpdateResult NinjaUpdate::handleUpdate(HTTPClient& http,
        const String& currentVersion, bool spiffs) {
    DEBUG_HTTP_UPDATE("[NinjaPCR] handleUpdate start\n");
    ESP8266HTTPUpdate::handleUpdate(http, currentVersion, spiffs);
    DEBUG_HTTP_UPDATE("[NinjaPCR] handleUpdate end\n");
}

/* Override */
bool NinjaUpdate::runUpdate(Stream& in, uint32_t size, String md5,
        int command) {

    // If you get compilation error, "virtual" is removed from ESP8266HTTPUpdate::runUpdate() and ESP8266HTTPUpdate::handleUpdate()
    ESP8266HTTPUpdate::CheckNinjaPCR();
    DEBUG_HTTP_UPDATE("[NinjaPCR] runUpdate start\n");
    bool result = ESP8266HTTPUpdate::runUpdate(in, size, expectedMD5, command);
    DEBUG_HTTP_UPDATE("[NinjaPCR] runUpdate end\n");
    return result;

}

