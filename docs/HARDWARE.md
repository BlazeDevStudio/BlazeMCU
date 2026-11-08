# Electrical and board profile rules

Target: generic ESP32-WROOM-32 reference DevKit, **not** ESP32-S2/S3/C3,
WROVER or ESP8266. Different dev boards may route, label or omit pins.

- GPIO is 3.3 V logic and not 5 V tolerant. Use level shifters or
  appropriate drivers for 5 V sensors, relays and external equipment.
- Do not connect traditional RS-232 voltage signals directly.
- Reference pin 6–11 connections are internal flash and never driven.
- GPIO34, 35, 36, 39 are input-only without internal pull resistors.
- GPIO37/38 are not exposed by the WROOM-32 reference module.
- Boot straps GPIO0/2/4/5/12/15 require extra caution even if a digital
  operation can be performed after successful boot.
- GPIO1/3 serve the UART0 serial console; GPIO16/17 may be PSRAM occupied
  on different modules. Do not run lab tests on such occupied resources.
- ADC2 can conflict with Wi-Fi; the current firmware implements no ADC test.
- GPIO output idle/LOW is **not** a universal electrically safe condition.
  Users must disconnect high-current loads and verify active-low wiring.
- Onboard LEDs are board dependent and no default LED has been assumed.

## Implementation limits

The active laboratory supports one pin at a time: digital input sampling at
approximately 20 ms resolution and manual steady output LOW/HIGH, with a hard
20-second lease. Counts can miss fast pulses; no interrupt capture, debounce,
PWM, mappings or display bus drivers exist. STOP releases a claimed GPIO using
the ESP-IDF driver, and a periodic task releases abandoned claims. All mappings
and counters are RAM-only and reset on device reboot.

Future display wiring must require explicit controller and pin selection,
I²C scan only confirms an ACK at an address, and SPI identification requires
manual selection. Never pretend a missing display was successfully initialized.
