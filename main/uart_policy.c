#include "uart_policy.h"
#include "board_caps.h"
blaze_policy_t blaze_uart_decide(int rx_pin, bool acknowledged) {
  /* Preserve UART0 boot/provisioning console. Input-only GPIOs may receive. */
  if (rx_pin == 1 || rx_pin == 3) return BLAZE_DENY;
  return blaze_pin_policy(blaze_pin_get(rx_pin), false, acknowledged);
}
