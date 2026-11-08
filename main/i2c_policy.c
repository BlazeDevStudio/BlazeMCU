#include "i2c_policy.h"
#include "board_caps.h"

/* Fail-closed, host-testable pin policy. Never writes hardware or storage. */
blaze_policy_t blaze_i2c_decide(int sda, int scl, bool acknowledged) {
  if (sda == scl) return BLAZE_DENY;
  const blaze_pin_t *d = blaze_pin_get(sda), *c = blaze_pin_get(scl);
  if (!d || !c || !d->digital_in || !c->digital_in) return BLAZE_DENY;
  const blaze_policy_t dp = blaze_pin_policy(d, true, acknowledged);
  const blaze_policy_t cp = blaze_pin_policy(c, true, acknowledged);
  if (dp == BLAZE_DENY || cp == BLAZE_DENY) return BLAZE_DENY;
  if (dp == BLAZE_ACK_REQUIRED || cp == BLAZE_ACK_REQUIRED) return BLAZE_ACK_REQUIRED;
  return BLAZE_ALLOW;
}
