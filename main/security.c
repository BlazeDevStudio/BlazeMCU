#include "security.h"
#include "esp_random.h"
#include "bootloader_random.h"
#include "esp_log.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "blaze_security";
static char ap_pass[21], admin_token[33];

/* Persist device identity only. No GPIO laboratory state is persisted. */
static void random_token(char *out, size_t length) {
  static const char symbols[] = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
  uint8_t bytes[32];
  esp_fill_random(bytes, length);
  for (size_t i = 0; i < length; i++) out[i] = symbols[bytes[i] % (sizeof(symbols) - 1)];
  out[length] = 0;
  memset(bytes, 0, sizeof(bytes));
}
esp_err_t blaze_security_init(void) {
  /* Enable true entropy before Wi-Fi; disable before RF/ADC starts. */
  bootloader_random_enable();
  nvs_handle_t nvs;
  esp_err_t e = nvs_open("blaze_auth", NVS_READWRITE, &nvs);
  if (e != ESP_OK) { bootloader_random_disable(); return e; }
  size_t len = sizeof(ap_pass);
  bool first_pass = nvs_get_str(nvs, "ap_pass", ap_pass, &len) != ESP_OK;
  if (first_pass) {
    random_token(ap_pass, 20);
    e = nvs_set_str(nvs, "ap_pass", ap_pass);
  }
  len = sizeof(admin_token);
  bool first_admin = nvs_get_str(nvs, "admin_token", admin_token, &len) != ESP_OK;
  if (e == ESP_OK && first_admin) {
    random_token(admin_token, 32);
    e = nvs_set_str(nvs, "admin_token", admin_token);
  }
  if (e == ESP_OK) e = nvs_commit(nvs);
  nvs_close(nvs);
  bootloader_random_disable();
  if (e != ESP_OK) return e;
  /* Serial provisioning is physical-access only. Keep credentials off HTTP endpoints. */
  ESP_LOGW(TAG, "Device AP key (serial only): %s", ap_pass);
  ESP_LOGW(TAG, "Admin token (serial only): %s", admin_token);
  ESP_LOGW(TAG, "Provision/relabel credentials for resale; serial logs contain secrets.");
  return ESP_OK;
}
const char *blaze_ap_password(void) { return ap_pass; }
bool blaze_token_matches(const char *candidate) {
  if (!candidate) return false;
  size_t len = strlen(candidate);
  unsigned diff = (unsigned)(len ^ 32U);
  for (size_t i = 0; i < 32; i++) {
    unsigned c = i < len ? (unsigned)(unsigned char)candidate[i] : 0U;
    diff |= c ^ (unsigned)(unsigned char)admin_token[i];
  }
  return diff == 0;
}
