#!/usr/bin/env python3
"""Build provenance manifest; NOT a security signature or hardware acceptance.

Uses only stdlib. Intentionally rejects target/board mismatches and oversized,
empty or incorrect-magic ESP images; does not guess flash offsets or perform OTA.
"""
import argparse
import hashlib
import json
from pathlib import Path

KNOWN = {
    "esp32": {"esp32-wroom-32-devkit": 0x120000},
    "esp8266": {"nodemcuv2": 0x100000, "d1_mini": 0x100000},
}

def inspect_image(chip: str, board: str, image: Path) -> dict:
    if chip not in KNOWN or board not in KNOWN[chip]:
        raise ValueError("Unsupported chip/board combination; no automatic target inference")
    if image.suffix.lower() != ".bin" or not image.is_file():
        raise ValueError("Expected an existing compiled .bin application")
    data_size = image.stat().st_size
    limit = KNOWN[chip][board]
    if data_size < 4096 or data_size > limit:
        raise ValueError(f"Application size {data_size} outside board size limit {limit}")
    with image.open("rb") as fh:
        magic = fh.read(1)
        if magic != b"\xe9":
            raise ValueError("Not a typical ESP image header (expected 0xE9)")
        sha256 = hashlib.sha256()
        sha256.update(magic)
        for part in iter(lambda: fh.read(131072), b""):
            sha256.update(part)
    return {
        "manifest_schema": "blazemcu.preview.v1",
        "project": "BlazeMCU",
        "chip_family": chip,
        "board": board,
        "image": image.name,
        "image_bytes": data_size,
        "board_reference_max_bytes": limit,
        "sha256": sha256.hexdigest(),
        "checks": {
            "compiled_file_present": True,
            "size_within_reference_limit": True,
            "basic_header_magic": True,
            "signature_verified": False,
            "chip_header_compatibility_verified": False,
            "hardware_tested": False,
            "recovery_tested": False,
            "production_approved": False,
        },
        "firmware_loader": "disabled",
        "preboot": "disabled",
        "note": "Unsigned preview inventory only; SHA-256 is not publisher authentication.",
    }

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--chip", required=True, choices=sorted(KNOWN))
    parser.add_argument("--board", required=True)
    parser.add_argument("--image", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    result = inspect_image(args.chip, args.board, args.image)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"{args.chip}/{args.board}: {result['image_bytes']} bytes, sha256 {result['sha256']}")
    print("PREVIEW / HARDWARE UNVERIFIED / NOT AUTHENTICATED")

if __name__ == "__main__":
    main()
