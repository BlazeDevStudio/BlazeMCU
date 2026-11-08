#include "../common/oled_pattern.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  assert(blaze_oled_allowed(0x3c, BLAZE_OLED_SSD1306, BLAZE_OLED_CHECKER));
  assert(blaze_oled_allowed(0x3d, BLAZE_OLED_SH1106, BLAZE_OLED_BLANK));
  assert(!blaze_oled_allowed(0x68, BLAZE_OLED_SSD1306, BLAZE_OLED_BLANK));
  assert(!blaze_oled_allowed(0x3c, 2, BLAZE_OLED_BLANK));
  assert(!blaze_oled_allowed(0x3c, 0, -1));
  assert(!blaze_oled_allowed(0x3c, 0, 3));
  size_t len = 0;
  const uint8_t *commands = blaze_oled_init_commands(BLAZE_OLED_SSD1306,&len);
  assert(commands && len > 20 && len < 32 && commands[0] == 0x00);
  commands = blaze_oled_init_commands(BLAZE_OLED_SH1106,&len);
  assert(commands && len > 20 && len < 32 && commands[0] == 0x00);
  assert(blaze_oled_init_commands(7,&len) == NULL && len == 0);
  uint8_t pageCmds[4] = {0};
  blaze_oled_page_commands(BLAZE_OLED_SSD1306, 7, pageCmds);
  assert(pageCmds[1] == 0xb7 && pageCmds[2] == 0x00);
  blaze_oled_page_commands(BLAZE_OLED_SH1106, 7, pageCmds);
  assert(pageCmds[1] == 0xb7 && pageCmds[2] == 0x02);
  assert(blaze_oled_pixel_byte(BLAZE_OLED_BLANK,0,0) == 0);
  assert(blaze_oled_pixel_byte(BLAZE_OLED_CHECKER,0,0) == 0x55);
  assert(blaze_oled_pixel_byte(BLAZE_OLED_CHECKER,0,8) == 0xaa);
  assert(blaze_oled_pixel_byte(BLAZE_OLED_BORDER,0,0) == 0xff);
  assert(blaze_oled_pixel_byte(BLAZE_OLED_BORDER,0,15) == 0x01);
  assert(blaze_oled_pixel_byte(BLAZE_OLED_BORDER,7,15) == 0x80);
  uint8_t packet[17]={0};
  blaze_oled_data_chunk(BLAZE_OLED_CHECKER,0,0,packet);
  assert(packet[0] == 0x40 && packet[1] == 0x55 && packet[9] == 0xaa);
  blaze_oled_data_chunk(BLAZE_OLED_BLANK,0,0,packet);
  for (int i = 1; i < 17; i++) assert(packet[i] == 0);
  puts("PASS: OLED controller/address limits, init framing, SH1106 offset, patterns, 16-byte chunks");
  return 0;
}
