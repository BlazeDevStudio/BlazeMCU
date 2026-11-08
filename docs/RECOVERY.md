# Recovery and partition safety

Minimum verified plan (not physically demonstrated): ESP32-WROOM-32 with 4 MB flash.

| Region | Offset | Size | Purpose |
|---|---:|---:|---|
| NVS | 0x9000 | 0x6000 | Per-device credentials, **not** lab settings |
| otadata | 0xF000 | 0x2000 | Protected boot selection metadata |
| phy_init | 0x11000 | 0x1000 | PHY init |
| factory | 0x20000 | 0x180000 | Factory starter application |
| ota_0 | 0x1A0000 | 0x120000 | Future upgrade slot |
| ota_1 | 0x2C0000 | 0x120000 | Future upgrade slot |

The existing application cannot electrically guarantee every attached device
is safe after reset. Tests release GPIO direction and terminate after 20
seconds; external loads, relays and boot-strapping circuits may still energize.
Provide physical protection and safe external pull circuitry.

The factory partition is *not* automatically selected after arbitrary resets.
Do not advertise a recovery button or "guaranteed fallback" until a verified
boot-selection implementation and on-device acceptance test exist.

For damaged firmware, use the specific board's USB serial bootloader in ROM
download mode (usually hold BOOT/GPIO0 during reset), with an authorized
factory image, offsets, and esptool. Reflash the complete validated layout
under qualified supervision. Erasing otadata or NVS can change boot choice,
credentials, calibration or customer data; **back them up first**. Avoid
\`erase_flash\` unless a full recovery image and re-provisioning procedure
are available. Do not short flash pins.

Resale acceptance requires physical tests of power interruption, wrong image
rejection, spontaneous reset, serial recovery, failed updater and a post-reset
safe-idle check on every supported PCB revision.
