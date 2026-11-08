#pragma once
#include <stdbool.h>
#include "esp_err.h"
/* Write-only opt-in OLED reference diagnostics. Must verify physical wiring.
 * No persistent pin/controller preferences or firmware writes.
 */
esp_err_t blaze_oled_draw(int sda, int scl, int address, int controller,
                          int pattern, bool acknowledged);
