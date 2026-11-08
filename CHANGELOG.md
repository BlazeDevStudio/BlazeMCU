# Changelog

## 0.6.0-preview (UART monitoring + release gate, no release)

- RX-only UART readout on ESP32 UART2 and ESP8266 SoftwareSerial.
- 8N1 1200/2400/4800/9600, 20-second expiry, bounded 128-byte polling, ASCII/hex output.
- GPIO1/3 debug console never retargeted; hard denies flash/absent/ESP8266 GPIO16.
- GPIO/I²C/OLED commands cannot overlap with UART capture. ESP32 uses a watchdog and mutex to expire sessions even without browser polling.
- Host pin/baud regression, end-to-end mock browser checks, documentation.
- Hardware confirmation, reboot/flash safety, slideshow and manufacturing readiness remain outstanding.
- Per-board unsigned SHA-256/size inventory added to firmware artifacts, with explicit NO-GO flags for hardware, recovery and production.

## 0.5.0-preview (OLED diagnostic driver, no release)

- Added narrow SSD1306/SH1106 128×64 I²C initializers and checker/border/blank test patterns for ESP32 and ESP8266.
- Shared host-testable command framing, 16-byte data chunks and SH1106 visible column offset; no copied third-party firmware.
- Hard-restricts explicit user-selected address to 0x3C or 0x3D, requires admin token and confirmation, and refuses active GPIO test collisions.
- Browser UI adds controller, address and test pattern selection with confirmation reset on changes; desktop/mobile mock-API regressions added.
- ESP-IDF and ESP8266 Wire backends have bounded transaction deadlines. No hardware display pattern success has been verified.
- Automatic controller discovery, HD44780, SPI TFT, slideshow, UART and FLASH/PREBOOT remain unfinished.

## 0.4.0-preview (I²C hardware lab preview — no release)

- Both targets implement bounded I²C address-only discovery with configurable SDA/SCL.
- ESP32 uses the ESP-IDF v5.5 I²C master bus driver; ESP8266 uses the pinned Arduino Wire implementation.
- Reject identical, flash, input-only and unsupported bus pins; strapping/UART pins require acknowledgment.
- Refuse scanning while GPIO experiments are active, use 3.3 V wiring warnings, and restore pins after scan.
- UI shows responding hexadecimal addresses, explicitly not inferred controller models.
- Added native permission regressions and deterministic simulated browser UI smoke tests.
- No physical I²C/display testing performed; full display drivers/slideshow remain unavailable.

## 0.3.0-preview (structure and UI/UX cleanup — no release)

- Split the shared dashboard into semantic HTML, CSS and JavaScript with one source of truth.
- Removed inline script/style execution policy; static resources embedded and served independently on ESP32 and ESP8266.
- Replaced raw JSON readouts with compact device facts, guarded admin-token verification, live GPIO measurements and searchable pin reference.
- Improved keyboard/focus and narrow-screen navigation, warning acknowledgement and disabled-feature labeling.
- Added exact static asset/router/CSP/auth contracts and JavaScript syntax checks to both CI matrices.
- Hardware flashing, one-time PREBOOT, electrical and reset verification remain unproven.

## 0.2.0-preview (dual-target GPIO PWM development)

- Shared 100–1000 Hz, 0–100% PWM validation across ESP32 / ESP8266, with hardware LEDC on ESP32 and nonblocking ESP8266 software PWM.
- Browser frequency/duty sliders and estimated period/on-time for a single pin.
- Automatic PWM stop/expiry within the existing 20-second session lease.
- Added native PWM boundary checks and cross-target CI enforcement.
- No electrical PWM waveform or reboot/timeout has been verified on physical boards.

## 0.1.1-preview (dual-target development, no release)

- Added dedicated ESP8266 PlatformIO/Arduino-core NodeMCU and D1 mini targets.
- Shares the ESP32 dashboard assets; uses ESP8266-native secure AP, credentials, GPIO policy, 20-second RAM-only input/output tests and hard denies.
- Added ESP8266 pin-policy host tests and board-specific firmware build matrix.
- FLASH, PREBOOT, PWM, displays and serial monitoring remain disabled/unimplemented.

## 0.1.0-preview (development branch, no release)

- Reconstructed independent ESP32 starter from owner's requirements because no
  historical Blaze ESP Starter v0.1 source was recoverable.
- ESP-IDF AP, per-device credentials, admin-token-protected GPIO commands.
- Complete classic ESP32 reference GPIO set with hard electrical policy.
- Bounded digital input/output laboratory and explicit STOP, no persisted lab state.
- Seven-section responsive offline dashboard; disabled features visibly named.
- Factory + two OTA partition layout, host policy checks, firmware build workflow.
- No firmware loader, temporary boot, display or UART implementation.
