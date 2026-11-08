# BlazeMCU — ESP Starter & Hardware Laboratory

**0.2.0 PREVIEW / UNVERIFIED — NOT READY FOR CUSTOMER RESALE OR PHYSICAL DEPLOYMENT**

A standalone dual-target ESP32-WROOM-32 / ESP8266 ESP-12E/F reference-board starter firmware, intentionally separate from Blaze MCU Studio, BlazePwifi, Phoenix, and the CoinOperatedEcosystem.

## What works in source

- ESP-IDF Wi-Fi SoftAP (192.168.4.1); SSID BlazeMCU-XXXX from device MAC.
- Per-device generated WPA2 AP password and independent admin token in NVS; shown on serial console.
- Mobile web dashboard and complete 40-entry classic ESP32 pin reference, including unsupported/reserved pins.
- Admin-token-gated **single-pin** digital HIGH/LOW or sampled input testing, plus PWM pulse/brightness control at 100–1000 Hz and 0–100% duty (backend enforced).
- Sensitive pin acknowledgment, input-only rejection of output driving, flash-pin hard deny.
- Session-only GPIO; 20-second timeout and explicit STOP release; no persisted GPIO choices.

**Not implemented on either target:** firmware upload, FLASH, PREBOOT, input-to-output mapping, debounce, ADC sampling, I²C/SPI diagnostics, displays, slideshow, UART monitor, and broader board-family compatibility certification. Disabled functions are visibly labeled. This source has no proven hardware results yet.

## Build

ESP32 requires **ESP-IDF v5.5.6** and a classic ESP32 target with **at least 4 MB flash**.

```sh
idf.py set-target esp32
idf.py build
# Only after validating hardware and power:
idf.py -p /dev/ttyUSB0 flash monitor
```

The primary app image is under `build/blazemcu_esp_starter.bin`; a standalone app image is not a merged bootloader + partition-table flash image. **Do not preflash resale units yet**: validate the compiled image, partition size, electrical characteristics, credential provisioning, recovery and physical resets first.

## ESP8266: NodeMCU / D1 mini

ESP8266 firmware is a **separate PlatformIO/Arduino-core build** under esp8266/, not an ESP-IDF cross-target compile. Both NodeMCU 1.0 (ESP-12E reference) and D1 mini use the same web dashboard and equivalent admin-protected session-only digital GPIO lab, with separate ESP8266 pin policy. Run locally:

    python -m pip install platformio==6.1.18
    pio run -d esp8266 -e nodemcuv2
    pio run -d esp8266 -e d1_mini

The board-specific application binaries are at esp8266/.pio/build/<env>/firmware.bin. **Build output must be reviewed per environment; no ESP8266 hardware validation yet.** GPIO0,2,15 are boot straps; GPIO6–11 remain selectable but electrically blocked; A0 is analog-only and not a GPIO17. ESP8266 bootloader/recovery layout is not compatible with ESP32 partitions. See docs/ESP8266.md.

## First boot and access

Power the board on via a trusted USB-to-serial connection (115200 baud); record the individual AP key and admin token displayed on its serial console. Join `BlazeMCU-XXXX` (final 4 hexadecimal MAC characters) and browse to `http://192.168.4.1`. Enter the admin token on Dashboard before starting GPIO tests. The token is kept only in that browser page's memory. Normal operation is offline.

Factory provisioning must ensure every board receives unique credentials; this preview exposes them in serial logs so it is **not yet hardened against physical access, leaked logs, or hostile RF environments**. Losing credentials requires authorized serial/NVS recovery; do not erase NVS casually.

## Safety and architecture

All GPIO0–GPIO39 appear in the reference, including unavailable, input-only, flash and strapping pins. UI acknowledgment never bypasses physical limits. Pin activity is bounded; no lab selections are stored. **WROOM is a reference profile, not a guarantee about a vendor's PCB**, LED, power budget, peripheral routing, or PSRAM.

The ESP32-only 4 MB partition plan reserves factory firmware, OTA metadata and two OTA slots. ESP8266 uses its framework's separate flash arrangement. OTA rollback is configured as preparation only: **neither FLASH nor PREBOOT is enabled**. See [recovery](docs/RECOVERY.md), [firmware compilation](docs/FIRMWARE.md) and [hardware rules](docs/HARDWARE.md).

## Quality gates

ESP8266's native pin policy is exercised separately:

    c++ -std=c++11 -Wall -Wextra -Werror tests/esp8266_policy_test.cpp -o /tmp/blaze8266-policy
    /tmp/blaze8266-policy
    cc -std=c11 -Wall -Wextra -Werror tests/pwm_policy_test.c -o /tmp/blaze-pwm-policy
    /tmp/blaze-pwm-policy

Its CI matrix builds NodeMCU 1.0 and D1 mini separately; no hardware reset or electrical proof is implied.


Run host-only policy checks (not actual ESP32 hardware tests):

```sh
cc -std=c11 -Wall -Wextra -Werror -Imain main/board_caps.c main/lab_policy.c tests/policy_test.c -o /tmp/blaze-policy
/tmp/blaze-policy
python3 -m unittest discover -s tests -v
```

CI additionally compiles the real ESP32 firmware and checks application-image size, but a green CI build cannot prove physical GPIO, OTA rollback, display wiring or boot behavior.

## Ownership and licensing

Copyright 2026 BlazeDevStudio. This public repository currently grants **no open-source license**. See [LICENSE](LICENSE) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Do not copy proprietary third-party firmware into the repository. The owner must explicitly choose an open-source license before granting redistribution rights to others.
