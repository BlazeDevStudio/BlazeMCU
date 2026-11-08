#pragma once
#include "esp_err.h"
#include <stdbool.h>

esp_err_t blaze_security_init(void);
const char *blaze_ap_password(void);
bool blaze_token_matches(const char *candidate);
