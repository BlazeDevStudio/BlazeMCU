# Third-party notices

No third-party application source or firmware binary is vendored here.

The ESP-IDF SDK, CMake toolchain and underlying ESP32 software components are
provided externally by Espressif Systems and contributors under their own
licenses. See https://github.com/espressif/esp-idf/blob/v5.5.6/LICENSE and
the SDK's component-level license notices.

GitHub Actions uses actions/checkout and actions/upload-artifact, each subject
to its upstream license. No borrowed display library, rBoot code, LPB,
JuanFi, WiFi5soft or commercial firmware is included.

Before shipping a commercial kit: review current SDK dependency/license
reports, retain required third-party notices and determine a customer
distribution license.

ESP8266 builds fetch the open-source ESP8266 Arduino core through PlatformIO and the ArduinoJson 6 library from Benoit Blanchon (MIT). Their individual licenses and transitive dependency notices must be retained for commercial distribution. No dependency source has been copied into this repository.
