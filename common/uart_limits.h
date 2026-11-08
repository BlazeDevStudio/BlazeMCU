#pragma once
/* Shared baud/lease/capture contract, separate from board-specific RX policy. */
#include <stdbool.h>
#define BLAZE_UART_LEASE_MS 20000
#define BLAZE_UART_MAX_READ 128
static inline bool blaze_uart_baud_supported(int b) {
  return b == 1200 || b == 2400 || b == 4800 || b == 9600;
}
