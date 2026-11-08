#pragma once
#include <stdbool.h>
#include <stdint.h>

#define BLAZE_PIN_COUNT 40

typedef struct {
  uint8_t number;
  bool exposed;       /* Available on the reference WROOM-32 dev board */
  bool digital_in;
  bool digital_out;
  bool adc;           /* Capability reference, analog testing not implemented */
  bool pwm;           /* Capability; bounded single-pin PWM is implemented */
  bool strap;
  bool reserved;      /* Active system/peripheral caution; not a permission bypass */
  const char *category;
  const char *warning;
} blaze_pin_t;

const blaze_pin_t *blaze_pin_get(int pin);
const char *blaze_board_id(void);
