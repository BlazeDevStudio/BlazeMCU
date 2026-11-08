#pragma once
#include <stdbool.h>
#include "lab_policy.h"
/* Host-testable, fail-closed UART input-only pin decision. */
blaze_policy_t blaze_uart_decide(int rx_pin, bool acknowledged);
