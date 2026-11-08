#pragma once
#include "esp_err.h"
#include "lab_policy.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int pin;          /* -1: idle */
  bool output;
  int level;
  uint32_t rising;
  uint32_t falling;
  uint32_t expires_in_ms;
} blaze_lab_status_t;

esp_err_t blaze_lab_init(void);
blaze_policy_t blaze_lab_decide(int pin, bool output, bool acknowledged);
esp_err_t blaze_lab_start(int pin, bool output, int level, bool pullup, bool acknowledged);
esp_err_t blaze_lab_stop(void);
blaze_lab_status_t blaze_lab_status(void);
