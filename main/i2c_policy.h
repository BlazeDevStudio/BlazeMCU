#pragma once
#include "lab_policy.h"
#include <stdbool.h>

/* Host-testable I2C permission logic; no ESP-IDF dependency. */
blaze_policy_t blaze_i2c_decide(int sda, int scl, bool acknowledged);
