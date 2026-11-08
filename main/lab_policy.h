#pragma once
#include "board_caps.h"
#include <stdbool.h>

typedef enum {
  BLAZE_DENY = 0,
  BLAZE_ACK_REQUIRED = 1,
  BLAZE_ALLOW = 2
} blaze_policy_t;

/* Acknowledgment only unlocks electrically supported operations.
 * It NEVER allows writing flash/PSRAM-connected or absent pins,
 * and NEVER makes an input-only pin output-capable.
 */
blaze_policy_t blaze_pin_policy(const blaze_pin_t *pin, bool output, bool acknowledged);
