#pragma once
/* Controller-specific reference OLED pattern generator. Pure C11/C++11, no I/O.
 * Supports opt-in 128x64 SSD1306 or SH1106 over 7-bit I2C 0x3C / 0x3D.
 * Do not infer controller from address ACK; this drives a chosen device.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BLAZE_OLED_WIDTH 128
#define BLAZE_OLED_PAGES 8
#define BLAZE_OLED_CHUNK 16

typedef enum {
  BLAZE_OLED_SSD1306 = 0,
  BLAZE_OLED_SH1106 = 1
} blaze_oled_controller_t;
typedef enum {
  BLAZE_OLED_BLANK = 0,
  BLAZE_OLED_CHECKER = 1,
  BLAZE_OLED_BORDER = 2
} blaze_oled_pattern_t;

static inline bool blaze_oled_allowed(int address, int controller, int pattern) {
  return (address == 0x3c || address == 0x3d) &&
         (controller == BLAZE_OLED_SSD1306 || controller == BLAZE_OLED_SH1106) &&
         pattern >= BLAZE_OLED_BLANK && pattern <= BLAZE_OLED_BORDER;
}
/* The command stream uses 0x00 as the I2C control byte. SH1106 uses an
 * explicit DC-DC command instead of the SSD1306 charge-pump command.
 * Models must be selected by the operator; no probing by write.
 */
static inline const uint8_t *blaze_oled_init_commands(int controller, size_t *length) {
  static const uint8_t ssd1306[] = {
    0x00, 0xae, 0xd5, 0x80, 0xa8, 0x3f, 0xd3, 0x00, 0x40,
    0x8d, 0x14, 0x20, 0x02, 0xa1, 0xc8, 0xda, 0x12,
    0x81, 0x7f, 0xd9, 0xf1, 0xdb, 0x40, 0xa4, 0xa6, 0xaf
  };
  static const uint8_t sh1106[] = {
    0x00, 0xae, 0xd5, 0x80, 0xa8, 0x3f, 0xd3, 0x00, 0x40,
    0xad, 0x8b, 0xa1, 0xc8, 0xda, 0x12, 0x81, 0x7f,
    0xd9, 0x22, 0xdb, 0x35, 0xa4, 0xa6, 0xaf
  };
  if (!length) return NULL;
  if (controller == BLAZE_OLED_SSD1306) {
    *length = sizeof(ssd1306);
    return ssd1306;
  }
  if (controller == BLAZE_OLED_SH1106) {
    *length = sizeof(sh1106);
    return sh1106;
  }
  *length = 0;
  return NULL;
}
/* Page-address start offset: SH1106 has 132 RAM columns; use +2 for
 * standard 128-visible-column breakout modules.
 */
static inline void blaze_oled_page_commands(int controller, unsigned page, uint8_t out[4]) {
  const unsigned offset = controller == BLAZE_OLED_SH1106 ? 2U : 0U;
  out[0] = 0x00;
  out[1] = (uint8_t)(0xb0 | (page & 7U));
  out[2] = (uint8_t)(0x00 | (offset & 15U));
  out[3] = (uint8_t)(0x10 | ((offset >> 4U) & 15U));
}
static inline uint8_t blaze_oled_pixel_byte(int pattern, unsigned page, unsigned column) {
  if (pattern == BLAZE_OLED_BLANK) return 0;
  if (pattern == BLAZE_OLED_CHECKER)
    return (uint8_t)((((column / 8U) + page) & 1U) ? 0xaa : 0x55);
  if (pattern == BLAZE_OLED_BORDER) {
    uint8_t value = 0;
    if (column == 0 || column == 127) value = 0xff;
    if (page == 0) value |= 0x01;
    if (page == 7) value |= 0x80;
    if (column == 63 || column == 64) value |= 0x18;
    return value;
  }
  return 0;
}
/* An I2C transaction has 0x40 data-control followed by exactly 16 bytes,
 * remaining under the ESP8266 Wire 32-byte buffer limit.
 */
static inline void blaze_oled_data_chunk(int pattern, unsigned page,
                                         unsigned start_column,
                                         uint8_t out[BLAZE_OLED_CHUNK + 1]) {
  out[0] = 0x40;
  for (unsigned i = 0; i < BLAZE_OLED_CHUNK; ++i)
    out[i+1] = blaze_oled_pixel_byte(pattern, page, start_column + i);
}
