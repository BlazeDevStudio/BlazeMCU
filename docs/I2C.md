# I²C discovery — 0.4 preview, both targets

This feature **only scans standard 7-bit addresses 0x08..0x77** and reports
which addresses acknowledge. It does not write device registers, initialize
a display controller, or claim that an address uniquely identifies the chip.

## Example wiring

| Board | SDA reference | SCL reference |
| --- | --- | --- |
| ESP32-WROOM-32 reference DevKit | GPIO21 | GPIO22 |
| ESP8266 NodeMCU 1.0 | GPIO4 / D2 | GPIO5 / D1 |
| ESP8266 D1 mini | GPIO4 / D2 | GPIO5 / D1 |

All GPIOs remain selectable in the UI, including those that the firmware must
refuse to drive. Both pins must be distinct, electrically output-capable, and
available. Boot-strapping and console pins require explicit warning acknowledgment.
ESP8266 GPIO16 is not supported for this I²C scanner; ESP8266 flash pins 6–11
and analog A0 are never driven. Scanning is refused while a GPIO test is active.

Use external pull-up resistors to **3.3 V** on SDA/SCL (typical 4.7 kΩ,
adjusted for bus capacitance/voltage). The ESP32 driver also enables weak
internal pull-ups, which are often insufficient. A sensor/display module may
already provide appropriate resistors: do not add unnecessary parallel loads.
Do not connect 5 V pull-ups, RS-232 lines, or unknown voltage buses to ESP pins.

The scanner checks each address with a 5 ms transaction timeout and has a
1.5 s overall bound. The ESP8266 Wire API supports bounded clock-stretching
but lacks a separate end() API in the pinned core; the service resets the
claimed pins to input at the end. A wedged bus can still require a power cycle.
Physical behavior, scan timing and recovery have **not** been verified with
real hardware. The browser marks an empty scan as no response, not proof that
no I²C device is physically attached.

## Safety / persistence

The scan is authenticated, does not alter flash/boot metadata, and does not
persist pin selections or discovered addresses. The scanner releases pins
after the request; the browser stores results only until the page closes.
Web HTTP processing is briefly serialized during the scan. Never trigger a
scan while important equipment depends on those pins.

Display diagnostics, RGB patterns, OLED initialization, SPI probing, and any
automatic controller identification remain unavailable pending dedicated
drivers and electrical tests.

## OLED diagnostics are a separate explicit operation (0.5 preview)

The I²C scanner remains **address-only**; a response at 0x3C/0x3D is NOT proof that a controller is SSD1306 or SH1106. The operator must separately choose the exact 128×64 controller and acknowledge wiring before any OLED command is issued. See docs/OLED.md. No generic sensor register writes have been added.
