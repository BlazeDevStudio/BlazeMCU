# Changelog

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
