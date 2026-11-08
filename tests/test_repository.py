from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]

class RepoContractTests(unittest.TestCase):
    def test_partitions_do_not_overlap_or_exceed_4mb(self):
        lines = (ROOT / "partitions.csv").read_text().splitlines()
        entries = []
        for line in lines:
            if not line.strip() or line.strip().startswith("#"):
                continue
            name, kind, subtype, offset, size, *_ = [x.strip() for x in line.split(",")]
            off, sz = int(offset, 0), int(size, 0)
            self.assertGreater(sz, 0)
            if kind == "app":
                self.assertEqual(off % 0x10000, 0)
            entries.append((off, off + sz, name, kind, subtype))
        ordered = sorted(entries)
        for a, b in zip(ordered, ordered[1:]):
            self.assertLessEqual(a[1], b[0], (a, b))
        self.assertLessEqual(ordered[-1][1], 0x400000)
        self.assertEqual({x[2] for x in entries}, {"nvs", "otadata", "phy_init", "factory", "ota_0", "ota_1"})

    def test_user_interface_has_all_sections_and_safe_file_picker(self):
        html = (ROOT / "main/web/index.html").read_text()
        for section in ("dashboard", "loader", "gpio", "displays", "serial", "pins", "help"):
            self.assertIn('id="' + section + '"', html)
        self.assertIn('accept=".bin,application/octet-stream" disabled', html)
        self.assertIn("PREBOOT · unavailable", html)
        self.assertIn("FLASH · unavailable", html)

    def test_session_only_gpio_source_has_no_persistent_write(self):
        source = (ROOT / "main/lab.c").read_text()
        for forbidden in ("nvs_set_", "nvs_commit(", "esp_partition_write(", "esp_ota_begin("):
            self.assertNotIn(forbidden, source)
        self.assertIn("LEASE_MS 20000", source)
        self.assertIn("release_locked();", source)

    def test_gpio_pwm_is_real_and_bounded_on_both_targets(self):
        html = (ROOT / "main/web/index.html").read_text()
        self.assertIn('<option value="pwm">', html)
        self.assertIn('frequency_hz', html)
        self.assertIn('duty_percent', html)
        self.assertIn('min="100" max="1000"', html)
        esp32 = (ROOT / "main/lab.c").read_text()
        esp8266 = (ROOT / "esp8266/src/main.cpp").read_text()
        self.assertIn("ledc_channel_config", esp32)
        self.assertIn("ledc_stop(", esp32)
        self.assertIn("analogWriteFreq(", esp8266)
        api = (ROOT / "main/api.c").read_text()
        self.assertIn("blaze_lab_pwm_start(", api)
        self.assertIn('strcmp(m->valuestring, "pwm")', api)
        self.assertIn('strcmp(mode,"pwm")', esp8266)
        self.assertIn('doc["mode"] = activePwm ?', esp8266)
        self.assertIn("blaze_pwm_valid", api)
        self.assertIn("analogWrite(activePin, 0)", esp8266)
        self.assertIn("blaze_pwm_valid", esp32)
        self.assertIn("blaze_pwm_valid", esp8266)
        for source in (esp32, esp8266):
            self.assertNotIn("nvs_set_", source)
            self.assertNotIn("esp_ota_write(", source)

    def test_disabled_loader_never_writes_firmware(self):
        source = (ROOT / "main/api.c").read_text()
        self.assertIn('"/api/firmware/flash", HTTP_POST, loader_disabled', source)
        self.assertIn('"/api/firmware/preboot", HTTP_POST, loader_disabled', source)
        self.assertNotIn("esp_ota_write(", source)
        self.assertNotIn("esp_ota_set_boot_partition(", source)

if __name__ == "__main__":
    unittest.main()
