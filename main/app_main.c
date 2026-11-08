#include "api.h"
#include "lab.h"
#include "security.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "blazemcu";
void app_main(void) {
  esp_err_t e = nvs_flash_init();
  if (e == ESP_ERR_NVS_NO_FREE_PAGES || e == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    /* Do not silently erase stored credentials or manufacturer metadata. */
    ESP_LOGE(TAG, "NVS requires explicit service recovery; refusing to erase");
    return;
  }
  ESP_ERROR_CHECK(e);
  ESP_ERROR_CHECK(blaze_security_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_ap();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  uint8_t mac[6];
  ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_AP, mac));
  wifi_config_t ap = {0};
  snprintf((char *)ap.ap.ssid, sizeof(ap.ap.ssid),
           "BlazeMCU-%02X%02X", mac[4], mac[5]);
  memcpy(ap.ap.password, blaze_ap_password(), strlen(blaze_ap_password()));
  ap.ap.ssid_len = strlen((char *)ap.ap.ssid);
  ap.ap.channel = 1;
  ap.ap.max_connection = 4;
  ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(blaze_lab_init());
  ESP_ERROR_CHECK(blaze_api_start());
  ESP_LOGI(TAG, "AP %s ready; open http://192.168.4.1", (char *)ap.ap.ssid);
  ESP_LOGW(TAG, "PREVIEW: no firmware upload/FLASH/PREBOOT, PWM, display or UART yet");
}
