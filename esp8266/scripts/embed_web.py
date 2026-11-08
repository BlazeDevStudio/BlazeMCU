# Generate a ROM-backed HTML header from the same source as ESP32.
Import("env")
from pathlib import Path
project = Path(env.subst("$PROJECT_DIR")).resolve()
source = project.parent / "main" / "web" / "index.html"
target = project / "include" / "generated_web.h"
content = source.read_text(encoding="utf-8")
delimiter = "BLAZE_WEB"
assert ")" + delimiter + '"' not in content, "Web asset conflicts with delimiter"
header = ('#pragma once\n#include <Arduino.h>\n'
          'static const char BLAZE_INDEX_HTML[] PROGMEM = R"' + delimiter + '('
          + content + ')' + delimiter + '";\n')
if not target.exists() or target.read_text(encoding="utf-8") != header:
    target.write_text(header, encoding="utf-8")
print("BlazeMCU: shared web UI embedded as ESP8266 PROGMEM")
