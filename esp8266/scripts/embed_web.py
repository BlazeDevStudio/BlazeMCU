# Produce three ROM-resident web assets from a single cross-target source.
# No LittleFS/SPIFFS partition or second, stale copy of the interface.
Import("env")
from pathlib import Path

project = Path(env.subst("$PROJECT_DIR")).resolve()
root = project.parent
target = project / "include" / "generated_web.h"
assets = (
    ("main/web/index.html", "BLAZE_INDEX_HTML", "BLAZE_HTML"),
    ("main/web/style.css", "BLAZE_STYLESHEET", "BLAZE_CSS"),
    ("main/web/app.js", "BLAZE_APP_JS", "BLAZE_JS"),
)
parts = ["#pragma once", "#include <Arduino.h>"]
for relative_path, symbol, delimiter in assets:
    content = (root / relative_path).read_text(encoding="utf-8")
    assert ")" + delimiter + '"' not in content, "Embedded asset delimiter conflict"
    parts.append('static const char ' + symbol +
                 '[] PROGMEM = R"' + delimiter + '(' +
                 content + ')' + delimiter + '";')
header = "\n".join(parts) + "\n"
if not target.exists() or target.read_text(encoding="utf-8") != header:
    target.write_text(header, encoding="utf-8")
print("BlazeMCU: embedded HTML/CSS/JS from main/web for ESP8266")
