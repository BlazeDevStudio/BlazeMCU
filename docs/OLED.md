# BlazeMCU 0.5 — 128×64 OLED reference diagnostics

**Preview, code-compiled only. Not bench verified.**

BlazeMCU now provides limited **SSD1306 and SH1106 I²C OLED** diagnostics on both ESP32-WROOM-32 and ESP8266 NodeMCU/D1 mini. These controllers have different display RAM/addressing commands: do not substitute one for the other. Models are **manually selected** by the user, never inferred from an I²C address scan.

Only 128×64 reference glass is implemented; 128×32 panels, SPI panels and other OLED/TFT controllers are not supported. On common SH1106 128×64 breakouts, the 128 visible columns begin at RAM column 2 (132 RAM columns total). Other module mappings require hardware review.

## Operator procedure

1. Confirm a 3.3 V logic I²C display, power supply and pull-ups. Select SDA/SCL according to the board wiring and obtain permission for any boot-sensitive pins. Do not use known flash or input-only pins.
2. Use the **address-only scan** to locate a response (commonly 0x3C/0x3D). This does *not* identify the controller.
3. **Read the module's controller identification from its specification**, then explicitly select SSD1306 128×64 or SH1106 128×64 and address 0x3C or 0x3D.
4. Check the confirmation box acknowledging that test data overwrites display RAM. Use Checker or Border. A successful HTTP response only means I²C writes were acknowledged, **not** that pixels, current draw or visual compatibility were verified.
5. Select Blank and reconfirm to write zeros to the same display RAM. Images remain otherwise until refreshed by another host or after power changes.

The firmware uses one temporary 100 kHz I²C bus at a time and refuses to collide with an active GPIO experiment. Every data transaction carries 16 payload bytes plus one data-control byte (well under the ESP8266 Wire 32-byte limit). Each write is bounded and the total transfer uses an approximately 2-second deadline; no state is persisted in NVS/EEPROM or flash. A busy or stuck bus may still require external recovery. Nonmatching devices at 0x3C/0x3D may interpret these writes unpredictably, so **confirmation is mandatory**.

This is not a firmware updater, general-purpose I²C register editor, slideshow player, controller identifier, electrical certification, or a replacement for independent display power/current measurements.

References: Solomon Systech SSD1306 specification (I²C 0x00 command / 0x40 data control byte); SH1106 controller command reference (132-column RAM, page addressing). No external proprietary source is copied.
