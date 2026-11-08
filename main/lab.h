#pragma once
#include "esp_err.h"
#include "lab_policy.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int pin;          /* -1: idle */
  bool output;
  bool pwm;
  int level;        /* -1 for PWM: instantaneous level is not measured */
  uint32_t rising;
  uint32_t falling;
  uint32_t expires_in_ms;
  int pwm_frequency_hz;
  int pwm_duty_percent;
} blaze_lab_status_t;

esp_err_t blaze_lab_init(void);
blaze_policy_t blaze_lab_decide(int pin, bool output, bool acknowledged);
esp_err_t blaze_lab_start(int pin, bool output, int level, bool pullup, bool acknowledged);
esp_err_t blaze_lab_pwm_start(int pin, int frequency_hz, int duty_percent, bool acknowledged);
esp_err_t blaze_lab_stop(void);
blaze_lab_status_t blaze_lab_status(void);
