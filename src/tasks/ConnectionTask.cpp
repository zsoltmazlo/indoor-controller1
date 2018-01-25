#include "Configuration.h"
#include "tasks.h"

void tasks::Connection::operator()(void* args) {
    printf("CONN | Task started.\n");

    // Wifi persistancy needs to be disabled as restarting device can cause a problem
    // with reconnection
    WiFi.persistent(false);
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_STA);

    if (Configuration::show_mac_address) {
        uint8_t mac[6];
        WiFi.macAddress(mac);

        // must wait some time to get mac address correctly
        delay(50ms);

        // Connecting to a WiFi
        printf("\nCONN | MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    printf("CONN | Connecting to %s", Configuration::ssid);

    // restart chip after 15sec of trying
    uint8_t restart_after = 30;
    Serial.setDebugOutput(false);
    WiFi.begin(Configuration::ssid, Configuration::pwd);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500ms);
        --restart_after;
        if (restart_after == 0) {
            printf("\nCONN | Wifi connection timed out after 30sec, restarting chip.");
            ESP.restart();
        }
    }
    Serial.println('\n');
    Serial.setDebugOutput(true);

    printf("\nCONN | Wifi connected.\nCONN | IP address: %s\n", WiFi.localIP().toString().c_str());
    ::Connection::instance->connect();

    for (;;) {
        if (!::Connection::instance->isConnected()) {
            printf("CONN | Connection interrupted, reconnecting...\n");
            ::Connection::instance->reconnect();
            printf("CONN | Reconnected.");
        }
        vTaskDelay(10s);
    }
}