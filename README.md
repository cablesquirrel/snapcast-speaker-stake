# Snapcast speaker stake

Hardware and firmware for the outdoor Snapcast speaker stakes: an ESP32-S3 (Seeed XIAO) client that streams from snapserver to a DAC and amplifier.

## Layout

```
hardware/
  v1/              KiCad project and Gerbers, original 3 W design (MAX98357A)
  v2.0.1/          KiCad project and Gerbers, 5 W design (GY-PCM5102A + CH05D)
firmware/          Firmware source, unmodified except as listed in CHANGES.md
  CHANGES.md       Every file and line changed from upstream, with rationale
  patches/         upstream-changes.patch: the same changes as a unified diff
releases/
  max98357_combo/  Prebuilt flash images for the 3 W flavor
  pcm5102a_ch05d/  Prebuilt flash images for the 5 W flavor
```

## Firmware flavors

| Flavor | Hardware | Config | Release |
|---|---|---|---|
| 3 W combo | XIAO + MAX98357A | `firmware/sdkconfig.max98357_combo` | `releases/max98357_combo/` |
| 5 W separate | XIAO + GY-PCM5102A + CH05D | `firmware/sdkconfig.pcm5102a_ch05d` | `releases/pcm5102a_ch05d/` |

Both flavors are built from the same source tree. They differ only in DAC configuration. See `firmware/CHANGES.md` for the full list of changes from upstream.

## Flash a prebuilt release

Each release folder contains the four images needed for a full flash:

```
python -m esptool --chip esp32s3 -p COMx write_flash \
  0x0 bootloader.bin 0x8000 partition-table.bin 0x1d000 ota_data_initial.bin 0x20000 snapclient.bin
```

Replace `COMx` with the board's port. Run this from inside the release folder.

## Build from source

Requires ESP-IDF 5.5.1. Build each flavor into its own directory:

```
cd firmware
idf.py -B build -D SDKCONFIG=sdkconfig.max98357_combo build
idf.py -B build.pcm5102a_ch05d -D SDKCONFIG=sdkconfig.pcm5102a_ch05d build
```

## OTA update

With the device on the network, push a new app image to its OTA listener on port 8032. Use a direct file reference so curl sends a Content-Length header:

```
curl <device-ip>:8032 --data-binary @releases/pcm5102a_ch05d/snapclient.bin
```

The connection is reset when the transfer completes. That is expected: the device reboots into the new image.

## WiFi

WiFi credentials are not compiled in. Provision them over USB-C with Improv WiFi (for example, web.esphome.io).

## Hardware notes

- Both PCB projects reference custom footprint and symbol libraries (XIAO ESP32-S3, GY-PCM5102A, CH05D) by absolute path under `C:/Users/Eric/Documents/KiCad/10.0/`. These libraries are not in this repo yet. The projects will not open cleanly without them.
- The V2.0.1 known issues are listed in `hardware/v2.0.1/KNOWN_ISSUES.md`. That board was sent to fab with them.

## Attribution and licensing

- `firmware/` is derived from [CarlosDerSeher/snapclient](https://github.com/CarlosDerSeher/snapclient). Its LICENSE and README are in `firmware/` unchanged.
- Hardware license: not yet chosen.
