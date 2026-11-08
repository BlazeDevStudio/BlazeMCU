#include "lab.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define LEASE_MS 20000
static SemaphoreHandle_t lock;
static int active_pin = -1;
static bool active_output;
static int last_level;
static uint32_t rising_count, falling_count;
static int64_t deadline_us;

/* Reset only pins explicitly taken over by this laboratory. No NVS/filesystem writes. */
static void release_locked(void) {
  if (active_pin >= 0) {
    gpio_set_direction((gpio_num_t)active_pin, GPIO_MODE_DISABLE);
    gpio_reset_pin((gpio_num_t)active_pin);
  }
  active_pin = -1;
  active_output = false;
  last_level = 0;
  deadline_us = 0;
}

static void monitor_task(void *arg) {
  (void)arg;
  for (;;) {
    if (xSemaphoreTake(lock, pdMS_TO_TICKS(250)) == pdTRUE) {
      if (active_pin >= 0) {
        if (esp_timer_get_time() >= deadline_us) {
          release_locked();
        } else if (!active_output) {
          int now = gpio_get_level((gpio_num_t)active_pin);
          if (now != last_level) {
            if (now) rising_count++; else falling_count++;
            last_level = now;
          }
        }
      }
      xSemaphoreGive(lock);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

esp_err_t blaze_lab_init(void) {
  lock = xSemaphoreCreateMutex();
  if (!lock) return ESP_ERR_NO_MEM;
  BaseType_t ok = xTaskCreate(monitor_task, "lab_monitor", 3072, NULL, 4, NULL);
  return ok == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
blaze_policy_t blaze_lab_decide(int pin, bool output, bool acknowledged) {
  return blaze_pin_policy(blaze_pin_get(pin), output, acknowledged);
}
esp_err_t blaze_lab_start(int pin, bool output, int level, bool pullup, bool acknowledged) {
  if (blaze_lab_decide(pin, output, acknowledged) != BLAZE_ALLOW) return ESP_ERR_NOT_SUPPORTED;
  if (level != 0 && level != 1) return ESP_ERR_INVALID_ARG;
  if (!lock || xSemaphoreTake(lock, pdMS_TO_TICKS(250)) != pdTRUE) return ESP_ERR_TIMEOUT;
  release_locked();
  rising_count = falling_count = 0;
  gpio_config_t cfg = {0};
  cfg.pin_bit_mask = 1ULL << pin;
  cfg.mode = output ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
  cfg.pull_up_en = (!output && pullup && pin < 34) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  cfg.intr_type = GPIO_INTR_DISABLE;
  esp_err_t err = output ? gpio_set_level((gpio_num_t)pin, level) : ESP_OK;
  if (err == ESP_OK) err = gpio_config(&cfg);
  if (err == ESP_OK) {
    active_pin = pin;
    active_output = output;
    last_level = output ? level : gpio_get_level((gpio_num_t)pin);
    deadline_us = esp_timer_get_time() + LEASE_MS * 1000LL;
  } else {
    gpio_reset_pin((gpio_num_t)pin);
  }
  xSemaphoreGive(lock);
  return err;
}
esp_err_t blaze_lab_stop(void) {
  if (!lock || xSemaphoreTake(lock, pdMS_TO_TICKS(250)) != pdTRUE) return ESP_ERR_TIMEOUT;
  release_locked();
  xSemaphoreGive(lock);
  return ESP_OK;
}
blaze_lab_status_t blaze_lab_status(void) {
  blaze_lab_status_t st = {.pin = -1};
  if (lock && xSemaphoreTake(lock, pdMS_TO_TICKS(250)) == pdTRUE) {
    st.pin = active_pin;
    st.output = active_output;
    st.level = active_pin < 0 ? 0 : gpio_get_level((gpio_num_t)active_pin);
    st.rising = rising_count;
    st.falling = falling_count;
    if (active_pin >= 0) {
      int64_t delta = deadline_us - esp_timer_get_time();
      st.expires_in_ms = delta > 0 ? (uint32_t)(delta / 1000) : 0;
    }
    xSemaphoreGive(lock);
  }
  return st;
}
