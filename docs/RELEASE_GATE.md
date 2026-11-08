# BlazeMCU release/production acceptance — NOT YET APPROVED

Current firmware is a **software-compiled preview**, not a physically verified
commercial release. Passing CI, a browser mock, or a SHA-256 file digest does
**not** prove correct GPIO voltage, Wi-Fi interoperability, boot recovery,
SD/flash behavior, UART reception, OLED compatibility or field safety.

## Source and reproducibility gates

- [x] Standalone BlazeMCU repository; unrelated proprietary code not imported
- [x] ESP32-WROOM-32, ESP8266 NodeMCU and ESP8266 D1 mini toolchain builds
- [x] Native GPIO/I2C/OLED/UART policy unit tests
- [x] Desktop/mobile simulated API browser smoke checks
- [x] Bounded, authenticated control APIs and disabled FLASH/PREBOOT handlers
- [x] Per-board unsigned preview image inventory (SHA-256, byte size, fixed board ID)
- [ ] Independent security/threat-model review of Wi-Fi, HTTP and UART credential exposure
- [ ] Signed trusted-image chain, manufacture key/provisioning design and key-rotation story
- [ ] Third-party license and distributor obligations audited
- [ ] Source release freeze reviewed, changelog reconciled and reproducible release builds

## Required physical acceptance, for EACH real PCB revision

- [ ] Power/voltage/current measured under USB and standalone supply, including brownout
- [ ] Wi-Fi SoftAP association and captive/local portal from iOS, Android, Windows
- [ ] Unique AP/admin credentials persist across normal and unexpected power cycles
- [ ] Factory reset, lost-token handling and NVS/EEPROM-full behavior validated
- [ ] Boot strapping pins do not prevent startup; no accidental GPIO-driven load
- [ ] GPIO input/output and PWM verified with multimeter/oscilloscope and 3.3 V-safe loads
- [ ] GPIO 20-second watchdog releases claimed resources during Wi-Fi stalls
- [ ] UART RX 8N1 exact bytes under Wi-Fi traffic with 1200–9600 baud on representative modules
- [ ] UART0 boot/provisioning serial remains usable before and after UART monitor
- [ ] I²C scanner on physically identified bus, with absent/stuck devices, clock stretch and pull-ups
- [ ] Explicit SSD1306/SH1106 128x64 OLED model, address, power and patterns
- [ ] Recovery from wrong board, wrong image, partial flash, interrupted OTA and brownout
- [ ] Firmware authentication and rollback protection/failover validated independently
- [ ] One-shot PREBOOT restoration verified across resets/power loss, if ever implemented
- [ ] Customer installation, diagnostics, backup/recovery guide and RMA path rehearsed

## Firmware upload and one-shot PREBOOT — hard NO-GO

There is intentionally **no browser-based firmware writer** on either target.

A safe implementation must validate chip family, board profile, application
header, image length, target partition and cryptographic publisher signature
*before writing*. It must maintain a physically validated recovery channel and
correctly revert a one-shot candidate after successful boot and after watchdog
or power interruption. The ESP32's provisioned OTA slots are only groundwork;
the existing bootloader does not itself guarantee one-shot reverting.
ESP8266 Arduino eboot sketch-copy OTA does not imply an independent testable
one-shot slot. A superficially working upload/OTA flow is not enough.

The preview manifests under GitHub Actions include only **hashes**, not
signatures or secure boot proofs. Their booleans permanently state
hardware_tested=false, recovery_tested=false, production_approved=false until
separately qualified and signed under an approved release process.

**Final gate decision: NO MERGE, NO PUBLIC PRODUCTION RELEASE, NO CUSTOMER
FLASHING, NO CLAIM OF HARDWARE PASS.** The owner can later approve a bounded
preproduction physical-test session and the gate document can be updated from
real evidence. Do not interpret test-only GitHub artifacts as a signed release.
