#include "lab_policy.h"

blaze_policy_t blaze_pin_policy(const blaze_pin_t *p, bool output, bool ack) {
  if (!p || !p->exposed) return BLAZE_DENY;
  if (output ? !p->digital_out : !p->digital_in) return BLAZE_DENY;
  if ((p->strap || p->reserved) && !ack) return BLAZE_ACK_REQUIRED;
  return BLAZE_ALLOW;
}
