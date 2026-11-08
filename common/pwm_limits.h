#pragma once
/* Same bounded PWM contract for ESP32 / ESP8266; no runtime dependencies.
 * 100..1000 Hz limits ESP8266 software-PWM load during Wi-Fi operation.
 * 0% and 100% are valid steady brightness endpoints, not pulse trains.
 */
#ifndef BLAZE_PWM_LIMITS_H
#define BLAZE_PWM_LIMITS_H
#define BLAZE_PWM_FREQ_MIN 100
#define BLAZE_PWM_FREQ_MAX 1000
static inline int blaze_pwm_valid(int hz, int duty_percent) {
  return hz >= BLAZE_PWM_FREQ_MIN && hz <= BLAZE_PWM_FREQ_MAX &&
         duty_percent >= 0 && duty_percent <= 100;
}
#endif
