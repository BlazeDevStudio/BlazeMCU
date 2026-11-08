# Firmware binary preparation

No browser-side .ino compilation is provided. The loader is **disabled** in this
preview. Do not upload a binary from an unrelated ESP variant.

## Build an application image

- Arduino IDE: Tools → Board / Flash Size / Partition Scheme, then
  Sketch → Export Compiled Binary. Locate the application binary in the
  export output. Different IDE/core versions may emit additional bootloader,
  partition or merged images; identify them by name and build logs.
- Arduino CLI: install the Espressif ESP32 core, then run
  `arduino-cli compile --fqbn esp32:esp32:esp32 --export-binaries sketch/`.
  Pass board options appropriate to the exact chip and flash configuration.
- PlatformIO: configure `platform = espressif32`, a correct `board = ...` and
  matching partition scheme, then run `pio run`. Application binaries are
  commonly in `.pio/build/<environment>/firmware.bin`.
- ESP-IDF v5.5.6: `idf.py set-target esp32 && idf.py build`.
  Use the app binary, not bootloader.bin or partition-table.bin, for an OTA slot.

## Image policy before any future FLASH/PREBOOT

Target classic ESP32, 4 MB minimum, compatible chip revision, flash mode and
encryption/security configuration. A valid `0xE9` image header alone does not
prove bootability. Check the app header, chip ID, segment bounds, checksums,
signature/secure-boot requirements, partition fit, anti-rollback and
application startup behavior. This repository's OTA slot size is **0x120000**
bytes, not the entire 4 MB flash.

A full-flash/merged image includes offsets or bootloader/partition metadata and
**cannot** be treated as an OTA application image. Arbitrary third-party
applications may not integrate ESP-IDF rollback confirmation or any BlazeMCU
boot protocol.

## PREBOOT gate

ESP-IDF's `ESP_OTA_IMG_NEW` to `PENDING_VERIFY` rollback mechanism can allow
one first boot before automatic rollback on subsequent reset without a
validity mark. That alone is **not a proven PREBOOT contract**:
- A trial app can mark itself valid and become persistent.
- A power loss during metadata changes can alter outcomes.
- Existing OTA slot contents or rollback states can change selection.
- Serial/physical reset behavior has not been tested here.

The actual PREBOOT route returns HTTP 501 and makes no flash writes.
A true one-time mechanism needs a proven bootloader/handshake or selection
protocol that cannot be overridden by arbitrary trial apps, plus physical
power-cycle verification. ESP8266 rBoot is research-only for this branch.
