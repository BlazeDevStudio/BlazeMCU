from pathlib import Path
import re
import unittest
from html.parser import HTMLParser

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
        js = (ROOT / "main/web/app.js").read_text()
        self.assertIn("frequency_hz", js)
        self.assertIn("duty_percent", js)
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

    def test_web_separated_and_authenticated_on_both_chips(self):
        html = (ROOT / "main/web/index.html").read_text()
        js = (ROOT / "main/web/app.js").read_text()
        css = (ROOT / "main/web/style.css").read_text()
        esp32 = (ROOT / "main/api.c").read_text()
        esp8266 = (ROOT / "esp8266/src/main.cpp").read_text()
        cmake = (ROOT / "main/CMakeLists.txt").read_text()
        generator = (ROOT / "esp8266/scripts/embed_web.py").read_text()
        self.assertIn('href="/style.css"', html)
        self.assertIn('src="/app.js"', html)
        self.assertNotIn("<style>", html)
        self.assertNotIn("<script>", html)
        self.assertNotIn("unsafe-inline", esp32)
        self.assertNotIn("unsafe-inline", esp8266)
        self.assertIn("style-src 'self'", esp32)
        self.assertIn("script-src 'self'", esp8266)
        for path in ("/style.css", "/app.js", "/api/auth/check"):
            self.assertIn('"' + path + '"', esp32)
            self.assertIn('"' + path + '"', esp8266)
        for asset in ("web/index.html", "web/style.css", "web/app.js"):
            self.assertIn(asset, cmake)
            self.assertIn(asset, generator)
        self.assertIn('auth_check_get(', esp32)
        self.assertIn('authCheckGet()', esp8266)
        self.assertIn("await request('/api/auth/check'", js)
        self.assertNotIn("localStorage", js)
        self.assertNotIn("sessionStorage", js)
        self.assertIn('[hidden]', css)
        self.assertLess(len(js.encode("utf-8")), 24000)

    def test_web_document_id_contract(self):
        html = (ROOT / "main/web/index.html").read_text()
        js = (ROOT / "main/web/app.js").read_text()
        class Ids(HTMLParser):
            def __init__(self):
                super().__init__()
                self.ids = set()
            def handle_starttag(self, tag, attrs):
                attributes = dict(attrs)
                if attributes.get("id"):
                    assert attributes["id"] not in self.ids, attributes["id"]
                    self.ids.add(attributes["id"])
        document = Ids()
        document.feed(html)
        names = set(re.findall(r"\$\('([a-zA-Z0-9]+)'\)", js))
        self.assertFalse(names - document.ids, names - document.ids)
        for section in ("dashboard", "gpio", "pins", "loader", "displays", "serial", "help"):
            self.assertIn(section, document.ids)
            self.assertIn("tab-" + section, document.ids)

    def test_docs_describe_both_actual_pwm_targets(self):
        readme = (ROOT / "README.md").read_text()
        hardware = (ROOT / "docs/HARDWARE.md").read_text()
        esp8266 = (ROOT / "docs/ESP8266.md").read_text()
        self.assertIn("NodeMCU / D1 mini", readme)
        self.assertIn("PWM uses ESP32 LEDC or ESP8266", hardware)
        self.assertNotIn("No PWM, reaction mappings", esp8266)
        self.assertIn("ESP8266 supports bounded software PWM", esp8266)

    def test_browser_smoke_suite_is_hardware_mock_only(self):
        test_source = (ROOT / "tests/ui/console.spec.cjs").read_text()
        config_source = (ROOT / "tests/ui/playwright.config.cjs").read_text()
        self.assertIn("page.route('**/api/**'", test_source)
        self.assertIn("mobile-chromium", config_source)
        self.assertIn("desktop-chromium", config_source)
        self.assertIn("0x3C", test_source)
        self.assertIn("simulated API, no hardware", (ROOT / ".github/workflows/esp32-preview.yml").read_text().lower())

    def test_disabled_loader_never_writes_firmware(self):
        source = (ROOT / "main/api.c").read_text()
        self.assertIn('"/api/firmware/flash", HTTP_POST, loader_disabled', source)
        self.assertIn('"/api/firmware/preboot", HTTP_POST, loader_disabled', source)
        self.assertNotIn("esp_ota_write(", source)
        self.assertNotIn("esp_ota_set_boot_partition(", source)

if __name__ == "__main__":
    unittest.main()
