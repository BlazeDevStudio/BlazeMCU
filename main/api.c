#include "api.h"
#include "board_caps.h"
#include "lab.h"
#include "security.h"
#include "cJSON.h"
#include "esp_chip_info.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_flash.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const unsigned char _binary_web_index_html_start[] asm("_binary_web_index_html_start");
extern const unsigned char _binary_web_index_html_end[] asm("_binary_web_index_html_end");
static httpd_handle_t server;

static esp_err_t json_send(httpd_req_t *req, cJSON *obj, const char *status) {
  char *encoded = cJSON_PrintUnformatted(obj);
  cJSON_Delete(obj);
  if (!encoded) return ESP_ERR_NO_MEM;
  if (status) httpd_resp_set_status(req, status);
  httpd_resp_set_type(req, "application/json; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
  esp_err_t err = httpd_resp_send(req, encoded, HTTPD_RESP_USE_STRLEN);
  free(encoded);
  return err;
}
static esp_err_t error_response(httpd_req_t *req, const char *status, const char *message) {
  cJSON *obj = cJSON_CreateObject();
  if (!obj) return ESP_ERR_NO_MEM;
  cJSON_AddStringToObject(obj, "status", "error");
  cJSON_AddStringToObject(obj, "message", message);
  return json_send(req, obj, status);
}
static bool authorized(httpd_req_t *req) {
  char token[80] = {0};
  if (httpd_req_get_hdr_value_str(req, "X-Blaze-Token", token, sizeof(token)) != ESP_OK) return false;
  bool ok = blaze_token_matches(token);
  memset(token, 0, sizeof(token));
  return ok;
}
static esp_err_t require_token(httpd_req_t *req) {
  return error_response(req, "401 Unauthorized", "Admin token required. Read it from the device serial console.");
}
static esp_err_t index_get(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
  httpd_resp_set_hdr(req, "Content-Security-Policy",
                     "default-src 'self'; style-src 'unsafe-inline' 'self'; script-src 'unsafe-inline' 'self'; connect-src 'self'; object-src 'none'; base-uri 'none'");
  return httpd_resp_send(req, (const char *)_binary_web_index_html_start,
                         _binary_web_index_html_end - _binary_web_index_html_start - 1);
}
static esp_err_t status_get(httpd_req_t *req) {
  esp_chip_info_t info;
  esp_chip_info(&info);
  uint32_t flash = 0;
  esp_flash_get_size(NULL, &flash);
  cJSON *obj = cJSON_CreateObject();
  cJSON_AddStringToObject(obj, "product", "BlazeMCU ESP Starter");
  cJSON_AddStringToObject(obj, "version", "0.1.0-preview");
  cJSON_AddStringToObject(obj, "board", blaze_board_id());
  cJSON_AddNumberToObject(obj, "cores", info.cores);
  cJSON_AddNumberToObject(obj, "heap_free", esp_get_free_heap_size());
  cJSON_AddNumberToObject(obj, "flash_bytes", flash);
  cJSON_AddStringToObject(obj, "portal", "192.168.4.1");
  cJSON_AddStringToObject(obj, "loader", "disabled_pending_recovery_validation");
  cJSON_AddStringToObject(obj, "gpio_mode", "session_only_20s");
  return json_send(req, obj, NULL);
}
static esp_err_t pins_get(httpd_req_t *req) {
  cJSON *arr = cJSON_CreateArray();
  if (!arr) return ESP_ERR_NO_MEM;
  for (int i = 0; i < BLAZE_PIN_COUNT; i++) {
    const blaze_pin_t *p = blaze_pin_get(i);
    cJSON *pin = cJSON_CreateObject();
    if (!pin) { cJSON_Delete(arr); return ESP_ERR_NO_MEM; }
    cJSON_AddNumberToObject(pin, "number", p->number);
    cJSON_AddBoolToObject(pin, "exposed", p->exposed);
    cJSON_AddBoolToObject(pin, "input", p->digital_in);
    cJSON_AddBoolToObject(pin, "output", p->digital_out);
    cJSON_AddBoolToObject(pin, "adc", p->adc);
    cJSON_AddBoolToObject(pin, "pwm", p->pwm);
    cJSON_AddBoolToObject(pin, "strap", p->strap);
    cJSON_AddBoolToObject(pin, "reserved", p->reserved);
    cJSON_AddStringToObject(pin, "category", p->category);
    cJSON_AddStringToObject(pin, "warning", p->warning);
    cJSON_AddItemToArray(arr, pin);
  }
  cJSON *obj = cJSON_CreateObject();
  cJSON_AddStringToObject(obj, "board", blaze_board_id());
  cJSON_AddItemToObject(obj, "pins", arr);
  return json_send(req, obj, NULL);
}
static esp_err_t gpio_get(httpd_req_t *req) {
  blaze_lab_status_t st = blaze_lab_status();
  cJSON *obj = cJSON_CreateObject();
  cJSON_AddStringToObject(obj, "status", st.pin < 0 ? "idle" : "running");
  cJSON_AddNumberToObject(obj, "pin", st.pin);
  cJSON_AddStringToObject(obj, "mode", st.output ? "output" : "input");
  cJSON_AddNumberToObject(obj, "level", st.level);
  cJSON_AddNumberToObject(obj, "rising_edges", st.rising);
  cJSON_AddNumberToObject(obj, "falling_edges", st.falling);
  cJSON_AddNumberToObject(obj, "expires_in_ms", st.expires_in_ms);
  return json_send(req, obj, NULL);
}
static esp_err_t receive_small_json(httpd_req_t *req, cJSON **obj) {
  if (req->content_len <= 0 || req->content_len > 256) return ESP_ERR_INVALID_SIZE;
  char data[257];
  int count = 0;
  while (count < req->content_len) {
    int r = httpd_req_recv(req, data + count, req->content_len - count);
    if (r <= 0) return ESP_FAIL;
    count += r;
  }
  data[count] = 0;
  *obj = cJSON_ParseWithLength(data, (size_t)count);
  memset(data, 0, sizeof(data));
  return *obj ? ESP_OK : ESP_ERR_INVALID_ARG;
}
static esp_err_t gpio_post(httpd_req_t *req) {
  if (!authorized(req)) return require_token(req);
  cJSON *obj = NULL;
  if (receive_small_json(req, &obj) != ESP_OK)
    return error_response(req, "400 Bad Request", "Expected JSON up to 256 bytes");
  cJSON *p = cJSON_GetObjectItemCaseSensitive(obj, "pin");
  cJSON *m = cJSON_GetObjectItemCaseSensitive(obj, "mode");
  cJSON *l = cJSON_GetObjectItemCaseSensitive(obj, "level");
  cJSON *a = cJSON_GetObjectItemCaseSensitive(obj, "ack");
  cJSON *u = cJSON_GetObjectItemCaseSensitive(obj, "pullup");
  if (!cJSON_IsNumber(p) || !cJSON_IsString(m) ||
      (strcmp(m->valuestring, "input") && strcmp(m->valuestring, "output")) ||
      (l && !cJSON_IsNumber(l)) || (a && !cJSON_IsBool(a)) || (u && !cJSON_IsBool(u))) {
    cJSON_Delete(obj);
    return error_response(req, "400 Bad Request", "Invalid pin, mode or option type");
  }
  int pin = p->valueint;
  bool output = strcmp(m->valuestring, "output") == 0;
  int level = l ? l->valueint : 0;
  bool ack = cJSON_IsTrue(a), pullup = cJSON_IsTrue(u);
  cJSON_Delete(obj);
  blaze_policy_t decision = blaze_lab_decide(pin, output, ack);
  if (decision == BLAZE_ACK_REQUIRED)
    return error_response(req, "409 Conflict", "Sensitive pin: acknowledge warning and proceed anyway");
  if (decision == BLAZE_DENY)
    return error_response(req, "422 Unprocessable Entity", "Unsupported operation: pin absent, flash-reserved, or input-only");
  esp_err_t e = blaze_lab_start(pin, output, level, pullup, ack);
  if (e != ESP_OK) return error_response(req, "503 Service Unavailable", "GPIO driver failed; test not started");
  cJSON *out = cJSON_CreateObject();
  cJSON_AddStringToObject(out, "status", "running");
  cJSON_AddStringToObject(out, "message", "Test active for up to 20 seconds; STOP to release the pin");
  return json_send(req, out, NULL);
}
static esp_err_t gpio_stop(httpd_req_t *req) {
  if (!authorized(req)) return require_token(req);
  if (blaze_lab_stop() != ESP_OK) return error_response(req, "503 Service Unavailable", "Failed to stop test");
  cJSON *out = cJSON_CreateObject();
  cJSON_AddStringToObject(out, "status", "idle");
  return json_send(req, out, NULL);
}
static esp_err_t loader_disabled(httpd_req_t *req) {
  if (!authorized(req)) return require_token(req);
  return error_response(req, "501 Not Implemented",
    "FLASH and PREBOOT disabled: image/partition/rollback and physical recovery not yet validated");
}
static esp_err_t register_route(const char *uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *)) {
  httpd_uri_t route = {.uri = uri, .method = method, .handler = handler, .user_ctx = NULL};
  return httpd_register_uri_handler(server, &route);
}
esp_err_t blaze_api_start(void) {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.max_uri_handlers = 9;
  cfg.stack_size = 6144;
  esp_err_t e = httpd_start(&server, &cfg);
  if (e != ESP_OK) return e;
  if ((e = register_route("/", HTTP_GET, index_get)) != ESP_OK ||
      (e = register_route("/api/status", HTTP_GET, status_get)) != ESP_OK ||
      (e = register_route("/api/pins", HTTP_GET, pins_get)) != ESP_OK ||
      (e = register_route("/api/gpio", HTTP_GET, gpio_get)) != ESP_OK ||
      (e = register_route("/api/gpio", HTTP_POST, gpio_post)) != ESP_OK ||
      (e = register_route("/api/gpio/stop", HTTP_POST, gpio_stop)) != ESP_OK ||
      (e = register_route("/api/firmware/flash", HTTP_POST, loader_disabled)) != ESP_OK ||
      (e = register_route("/api/firmware/preboot", HTTP_POST, loader_disabled)) != ESP_OK) {
    httpd_stop(server);
    server = NULL;
    return e;
  }
  return ESP_OK;
}
