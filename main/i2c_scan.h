#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "i2c_policy.h"

/* Session-only I2C address probe. Does not read or write device registers.
 * Uses only one bus, at most 112 seven-bit probes, maximum 5 ms/probe.
 */
#define BLAZE_I2C_MAX_RESPONDERS 112

esp_err_t blaze_i2c_scan(int sda, int scl, bool acknowledged,
                         uint8_t *addresses, size_t capacity, size_t *found);
