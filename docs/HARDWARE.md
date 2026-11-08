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
mappings or display bus drivers exist. Bounded PWM is now implemented as a separate
single-pin mode but has not been verified electrically. STOP releases a claimed GPIO using
the ESP-IDF driver, and a periodic task releases abandoned claims. All mappings
and counters are RAM-only and reset on device reboot.

Future display wiring must require explicit controller and pin selection,
I²C scan only confirms an ACK at an address, and SPI identification requires
manual selection. Never pretend a missing display was successfully initialized.

## PWM preview

PWM uses ESP32 LEDC or ESP8266 software PWM at 100–1000 Hz (inclusive) with 0–100% duty, one pin at a time, stopped by explicit STOP or session expiry; no physical oscilloscope or power-load validation performed. A displayed period or duty cycle is an estimate, not a measurement. Output drivers must not be directly connected to motors, coils or high-current loads.

## Browser administration (0.3 preview)

The dashboard must confirm the per-device admin token with /api/auth/check before allowing GPIO mutation. CSS and JavaScript are served from device flash with CSP that disallows inline scripts and styles; no external assets are loaded. This is an offline lab UI, not an HTTPS or customer-account system. Physical serial credential exposure and AP-side attack resistance still require testing and manufacturing hardening.

## I²C address discovery

Address-only bus scanning is available on both ESP32 and ESP8266, but **not hardware verified**. Avoid 5 V pull-ups. The driver checks 0x08–0x77 at bounded timeout and releases pins after the scan, refusing to interrupt an active GPIO lease. It never identifies, initializes, or drives a display controller. See docs/I2C.md.

## Limited I²C OLED output

The 0.5 preview offers opt-in SSD1306/SH1106 128×64 checker, border and blank patterns only. No display model is auto-detected. It rewrites target display RAM and may persist visually until blank/power off; laboratory settings are never stored. The driver refuses scans while a GPIO test is active. Physical output and recovery remain unverified.

## RX-only UART (0.6 preview)

No serial transmit API exists. UART0 console pins GPIO1/3 and flash pins are denied; ESP8266 GPIO16 cannot run software RX. External 3.3 V TTL signals only, with shared ground. Never connect RS-232 or 5 V TTL without an approved converter. Sessions expire after 20 seconds and cannot overlap GPIO/I²C/OLED activity. Physical timing and input reliability are unverified.
