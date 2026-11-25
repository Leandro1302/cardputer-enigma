#include "WifiBroadcast.h"
#include <esp_event.h>
#include <esp_wifi.h>
#include <cstring>

bool WifiBroadcast::start(const std::string& ssid) {
    if (started_ || ssid.empty() || ssid.size() > MaxSsidBytes ||
        ssid.find('\0') != std::string::npos) return false;

    const auto eventLoop = esp_event_loop_create_default();
    if (eventLoop != ESP_OK && eventLoop != ESP_ERR_INVALID_STATE) return false;
    if (!initialized_) {
        wifi_init_config_t driver = WIFI_INIT_CONFIG_DEFAULT();
        driver.nvs_enable = 0;
        if (esp_wifi_init(&driver) != ESP_OK) return false;
        initialized_ = true;
    }

    wifi_config_t config{};
    std::memcpy(config.ap.ssid, ssid.data(), ssid.size());
    config.ap.ssid_len = ssid.size();
    config.ap.channel = 1;
    config.ap.authmode = WIFI_AUTH_OPEN;
    config.ap.max_connection = 1;
    config.ap.beacon_interval = 100;
    // Configure the exact SSID before starting the radio. Keep it in RAM.
    started_ = esp_wifi_set_storage(WIFI_STORAGE_RAM) == ESP_OK &&
               esp_wifi_set_mode(WIFI_MODE_AP) == ESP_OK &&
               esp_wifi_set_config(WIFI_IF_AP, &config) == ESP_OK &&
               esp_wifi_start() == ESP_OK;
    if (!started_) stop();
    return started_;
}

bool WifiBroadcast::stop() {
    if (started_) {
        if (esp_wifi_stop() != ESP_OK) return false;
        started_ = false;
    }
    if (initialized_ && esp_wifi_deinit() == ESP_OK) initialized_ = false;
    return true;
}

app::BroadcastStatus WifiBroadcast::status() const {
    if (!started_) return {};
    wifi_config_t config{};
    if (esp_wifi_get_config(WIFI_IF_AP, &config) != ESP_OK ||
        config.ap.ssid_len > MaxSsidBytes) return {true, {}, false};
    return {true, std::string(reinterpret_cast<const char*>(config.ap.ssid), config.ap.ssid_len),
            config.ap.authmode == WIFI_AUTH_OPEN};
}
