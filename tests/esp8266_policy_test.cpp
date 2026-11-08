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
  std::puts("PASS: 18 ESP8266 pins, boot warnings, hard flash deny, A0 no digital");
}
