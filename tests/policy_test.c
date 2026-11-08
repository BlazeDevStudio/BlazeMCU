#include "board_caps.h"
#include "lab_policy.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  assert(strcmp(blaze_board_id(), "esp32-wroom-32-devkit-reference") == 0);
  for (int i = 0; i < BLAZE_PIN_COUNT; ++i) {
    const blaze_pin_t *p = blaze_pin_get(i);
    assert(p && p->number == i && p->category && p->warning);
    if (!p->exposed) {
      assert(blaze_pin_policy(p, true, true) == BLAZE_DENY);
      assert(blaze_pin_policy(p, false, true) == BLAZE_DENY);
    }
  }
  assert(blaze_pin_get(-1) == NULL);
  assert(blaze_pin_get(40) == NULL);
  assert(blaze_pin_policy(blaze_pin_get(34), true, true) == BLAZE_DENY);
  assert(blaze_pin_policy(blaze_pin_get(34), false, false) == BLAZE_ALLOW);
  assert(blaze_pin_policy(blaze_pin_get(6), true, true) == BLAZE_DENY);
  assert(blaze_pin_policy(blaze_pin_get(11), false, true) == BLAZE_DENY);
  assert(blaze_pin_policy(blaze_pin_get(0), true, false) == BLAZE_ACK_REQUIRED);
  assert(blaze_pin_policy(blaze_pin_get(0), true, true) == BLAZE_ALLOW);
  assert(blaze_pin_policy(blaze_pin_get(3), true, false) == BLAZE_ACK_REQUIRED);
  assert(blaze_pin_policy(blaze_pin_get(3), true, true) == BLAZE_ALLOW);
  assert(blaze_pin_policy(blaze_pin_get(32), true, false) == BLAZE_ALLOW);
  assert(blaze_pin_policy(blaze_pin_get(33), false, false) == BLAZE_ALLOW);
  puts("PASS: 40 pin profiles, restricted visibility and fail-closed policy");
}
