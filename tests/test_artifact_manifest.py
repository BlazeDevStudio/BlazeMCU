import hashlib
import tempfile
import unittest
from pathlib import Path
from scripts.artifact_manifest import inspect_image

class ManifestTests(unittest.TestCase):
    def test_valid_reference_targets_and_sha256(self):
        payload = b"\xe9" + b"\0" * 8191
        with tempfile.TemporaryDirectory() as tmp:
            file = Path(tmp) / "firmware.bin"
            file.write_bytes(payload)
            for chip, board in (("esp32","esp32-wroom-32-devkit"),
                                ("esp8266","nodemcuv2"),("esp8266","d1_mini")):
                obj = inspect_image(chip,board,file)
                self.assertEqual(obj["sha256"],hashlib.sha256(payload).hexdigest())
                self.assertEqual(obj["image_bytes"],8192)
                self.assertFalse(obj["checks"]["hardware_tested"])
                self.assertFalse(obj["checks"]["signature_verified"])
                self.assertFalse(obj["checks"]["production_approved"])
                self.assertEqual(obj["firmware_loader"],"disabled")

    def test_wrong_chip_board_and_invalid_images_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            file = Path(tmp) / "firmware.bin"
            file.write_bytes(b"\xe9" + b"\0"*8191)
            for chip,board in (("esp32","nodemcuv2"),("esp8266","esp32-wroom-32-devkit"),
                               ("esp32","unknown"),("bad","d1_mini")):
                with self.assertRaises(ValueError):
                    inspect_image(chip,board,file)
            file.write_bytes(b"\0"*8192)
            with self.assertRaisesRegex(ValueError, "header"):
                inspect_image("esp8266","d1_mini",file)
            file.write_bytes(b"\xe9"+b"\0"*100)
            with self.assertRaisesRegex(ValueError, "size"):
                inspect_image("esp32","esp32-wroom-32-devkit",file)
            file.write_bytes(b"\xe9"+b"\0"*0x120000)
            with self.assertRaisesRegex(ValueError, "size"):
                inspect_image("esp32","esp32-wroom-32-devkit",file)
            nonbin = Path(tmp)/"firmware.zip"
            nonbin.write_bytes(b"\xe9"+b"\0"*8191)
            with self.assertRaises(ValueError):
                inspect_image("esp8266","nodemcuv2",nonbin)
