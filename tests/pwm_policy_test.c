#include "../common/pwm_limits.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  assert(!blaze_pwm_valid(0, 50));
  assert(!blaze_pwm_valid(99, 50));
  assert(!blaze_pwm_valid(1001, 50));
  assert(!blaze_pwm_valid(-1, 50));
  assert(!blaze_pwm_valid(500, -1));
  assert(!blaze_pwm_valid(500, 101));
  assert(blaze_pwm_valid(100, 0));
  assert(blaze_pwm_valid(500, 50));
  assert(blaze_pwm_valid(1000, 100));
  puts("PASS: PWM bounds 100..1000 Hz, 0..100% with invalid settings denied");
  return 0;
}
