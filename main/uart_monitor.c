#include "uart_monitor.h"
#include "lab.h"
#include "../common/uart_limits.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

/* UART2 RX-only lab. Never serial.write/TX. Protect driver lifecycle against
 * concurrent HTTP GET/STOP and timeout worker; no NVS or boot writes.
 */
static SemaphoreHandle_t uart_mutex;
static int rx_pin = -1;
static int current_baud;
static int64_t deadline_us;

static void release_locked(void) {
  if (rx_pin < 0) return;
  const int pin = rx_pin;
  uart_driver_delete(UART_NUM_2);
  rx_pin = -1;
  current_baud = 0;
  deadline_us = 0;
  gpio_reset_pin((gpio_num_t)pin);
}
static void expire_locked(void) {
  if (rx_pin >= 0 && esp_timer_get_time() >= deadline_us) release_locked();
}
static bool take_uart(void) {
  return uart_mutex && xSemaphoreTake(uart_mutex,pdMS_TO_TICKS(200))==pdTRUE;
}
static void uart_watchdog(void *unused) {
  (void)unused;
  for (;;) {
    if (take_uart()) { expire_locked(); xSemaphoreGive(uart_mutex); }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
esp_err_t blaze_uart_init(void) {
  uart_mutex = xSemaphoreCreateMutex();
  if (!uart_mutex) return ESP_ERR_NO_MEM;
  BaseType_t created=xTaskCreate(uart_watchdog,"uart_watchdog",2048,NULL,3,NULL);
  return created==pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
bool blaze_uart_active(void) {
  if (!take_uart()) return true; /* Fail closed when lock cannot be acquired. */
  expire_locked();
  bool active = rx_pin >= 0;
  xSemaphoreGive(uart_mutex);
  return active;
}
int blaze_uart_rx_pin(void) {
  if (!take_uart()) return -1;
  expire_locked(); int n=rx_pin;
  xSemaphoreGive(uart_mutex); return n;
}
int blaze_uart_baud(void) {
  if (!take_uart()) return 0;
  expire_locked(); int n=current_baud;
  xSemaphoreGive(uart_mutex); return n;
}
uint32_t blaze_uart_expires_ms(void) {
  if (!take_uart()) return 0;
  expire_locked();
  int64_t rem = rx_pin < 0 ? 0 : deadline_us - esp_timer_get_time();
  xSemaphoreGive(uart_mutex);
  return rem > 0 ? (uint32_t)(rem / 1000) : 0;
}
void blaze_uart_stop(void) {
  if (!take_uart()) return;
  release_locked();
  xSemaphoreGive(uart_mutex);
}
esp_err_t blaze_uart_start(int pin, int baud, bool acknowledged) {
  if (blaze_uart_decide(pin, acknowledged) != BLAZE_ALLOW ||
      !blaze_uart_baud_supported(baud)) return ESP_ERR_INVALID_ARG;
  if (blaze_lab_status().pin >= 0) return ESP_ERR_INVALID_STATE;
  if (!take_uart()) return ESP_ERR_TIMEOUT;
  expire_locked();
  if (rx_pin >= 0) {
    xSemaphoreGive(uart_mutex);
    return ESP_ERR_INVALID_STATE;
  }
  uart_config_t config = {
    .baud_rate = baud,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT
  };
  esp_err_t err = uart_param_config(UART_NUM_2,&config);
  if (err == ESP_OK) err = uart_driver_install(UART_NUM_2,1024,0,0,NULL,0);
  if (err == ESP_OK) {
    err = uart_set_pin(UART_NUM_2,UART_PIN_NO_CHANGE,pin,
                       UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE);
    if (err != ESP_OK) uart_driver_delete(UART_NUM_2);
  }
  if (err == ESP_OK) {
    rx_pin = pin;
    current_baud = baud;
    deadline_us = esp_timer_get_time() + BLAZE_UART_LEASE_MS * 1000LL;
  } else {
    gpio_reset_pin((gpio_num_t)pin);
  }
  xSemaphoreGive(uart_mutex);
  return err;
}
int blaze_uart_read(uint8_t *dst, size_t max) {
  if (!dst || !max) return 0;
  if (max > BLAZE_UART_MAX_READ) max = BLAZE_UART_MAX_READ;
  if (!take_uart()) return 0;
  expire_locked();
  int count = rx_pin < 0 ? 0 : uart_read_bytes(UART_NUM_2,dst,(uint32_t)max,0);
  xSemaphoreGive(uart_mutex);
  return count > 0 ? count : 0;
}
