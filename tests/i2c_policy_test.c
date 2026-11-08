#include "../main/i2c_policy.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  assert(blaze_i2c_decide(21,22,false) == BLAZE_ALLOW);
  assert(blaze_i2c_decide(22,21,false) == BLAZE_ALLOW);
  assert(blaze_i2c_decide(21,21,true) == BLAZE_DENY);
  assert(blaze_i2c_decide(6,22,true) == BLAZE_DENY);
  assert(blaze_i2c_decide(21,34,true) == BLAZE_DENY);
  assert(blaze_i2c_decide(21,37,true) == BLAZE_DENY);
  assert(blaze_i2c_decide(21,-1,true) == BLAZE_DENY);
  assert(blaze_i2c_decide(21,40,true) == BLAZE_DENY);
  assert(blaze_i2c_decide(0,22,false) == BLAZE_ACK_REQUIRED);
  assert(blaze_i2c_decide(0,22,true) == BLAZE_ALLOW);
  assert(blaze_i2c_decide(1,22,false) == BLAZE_ACK_REQUIRED);
  assert(blaze_i2c_decide(1,22,true) == BLAZE_ALLOW);
  puts("PASS: ESP32 I2C pair safety, restricted acknowledgment and flash denies");
  return 0;
}
