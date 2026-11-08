#include "../esp8266/include/blaze_caps.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main() {
  assert(BLAZE8266_PIN_COUNT == 18);
  for (int n = 0; n < 18; ++n) {
    const Blaze8266Pin *p = blaze8266Pin(n);
    assert(p && p->number == n && p->label && p->warning && p->category);
    if (!p->exposed || !p->input) assert(blaze8266Policy(n, false, true) == BLAZE8266_DENY);
    if (!p->exposed || !p->output) assert(blaze8266Policy(n, true, true) == BLAZE8266_DENY);
  }
  assert(blaze8266Pin(-1) == nullptr && blaze8266Pin(18) == nullptr);
  assert(blaze8266Policy(6,true,true) == BLAZE8266_DENY);
  assert(blaze8266Policy(9,false,true) == BLAZE8266_DENY);
  assert(blaze8266Policy(10,true,true) == BLAZE8266_DENY);
  assert(blaze8266Policy(17,false,true) == BLAZE8266_DENY);
  assert(blaze8266Policy(17,true,true) == BLAZE8266_DENY);
  for (int n : {0,2,15}) {
    assert(blaze8266Policy(n,true,false) == BLAZE8266_ACK_REQUIRED);
    assert(blaze8266Policy(n,true,true) == BLAZE8266_ALLOW);
  }
  assert(blaze8266Policy(1,true,false) == BLAZE8266_ACK_REQUIRED);
  assert(blaze8266Policy(3,false,true) == BLAZE8266_ALLOW);
  assert(blaze8266Policy(4,true,false) == BLAZE8266_ALLOW);
  assert(blaze8266Policy(5,false,false) == BLAZE8266_ALLOW);
  assert(blaze8266Policy(16,true,false) == BLAZE8266_ALLOW);
  assert(!blaze8266Pin(16)->pwm && blaze8266Pin(17)->adc);
  assert(blaze8266I2cPolicy(4,5,false) == BLAZE8266_ALLOW);
  assert(blaze8266I2cPolicy(4,4,true) == BLAZE8266_DENY);
  assert(blaze8266I2cPolicy(4,16,true) == BLAZE8266_DENY);
  assert(blaze8266I2cPolicy(6,5,true) == BLAZE8266_DENY);
  assert(blaze8266I2cPolicy(17,5,true) == BLAZE8266_DENY);
  assert(blaze8266I2cPolicy(0,5,false) == BLAZE8266_ACK_REQUIRED);
  assert(blaze8266I2cPolicy(0,5,true) == BLAZE8266_ALLOW);
  assert(blaze8266I2cPolicy(1,5,false) == BLAZE8266_ACK_REQUIRED);
  assert(blaze8266I2cPolicy(1,5,true) == BLAZE8266_ALLOW);
  assert(blaze8266UartPolicy(12,false)==BLAZE8266_ALLOW);
  assert(blaze8266UartPolicy(0,false)==BLAZE8266_ACK_REQUIRED);
  assert(blaze8266UartPolicy(0,true)==BLAZE8266_ALLOW);
  for (int p : {1,3,6,7,8,9,10,11,16,17})
    assert(blaze8266UartPolicy(p,true)==BLAZE8266_DENY);
  std::puts("PASS: ESP8266 GPIO/I2C/UART fail-closed policy");
}
