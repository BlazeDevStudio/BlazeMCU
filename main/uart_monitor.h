#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "uart_policy.h"
esp_err_t blaze_uart_init(void);
/* RX-only; no serial TX, no persistent configuration. */
esp_err_t blaze_uart_start(int rx_pin, int baud, bool acknowledged);
void blaze_uart_stop(void);
bool blaze_uart_active(void);
int blaze_uart_rx_pin(void);
int blaze_uart_baud(void);
uint32_t blaze_uart_expires_ms(void);
int blaze_uart_read(uint8_t *dst, size_t max);
