#include "../main/uart_policy.h"
#include "../common/uart_limits.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  for (int baud=0; baud<=12000; baud++) {
    bool allowed = baud==1200 || baud==2400 || baud==4800 || baud==9600;
    assert(blaze_uart_baud_supported(baud)==allowed);
  }
  assert(blaze_uart_decide(16,false)==BLAZE_ALLOW);
  assert(blaze_uart_decide(34,false)==BLAZE_ALLOW); /* RX-only supports input-only */
  assert(blaze_uart_decide(35,false)==BLAZE_ALLOW);
  assert(blaze_uart_decide(1,true)==BLAZE_DENY);
  assert(blaze_uart_decide(3,true)==BLAZE_DENY);
  assert(blaze_uart_decide(6,true)==BLAZE_DENY);
  assert(blaze_uart_decide(37,true)==BLAZE_DENY);
  assert(blaze_uart_decide(12,false)==BLAZE_ACK_REQUIRED);
  assert(blaze_uart_decide(12,true)==BLAZE_ALLOW);
  assert(blaze_uart_decide(-1,true)==BLAZE_DENY);
  puts("PASS: ESP32 RX-only UART pin/baud policy with console protection");
  return 0;
}
