# Snapcast Speaker Stake

Hardware and firmware design to retrofit the [Hampton Bay B0029RA](https://www.homedepot.com/p/Hampton-Bay-16-9-in-Black-Outdoor-Landscape-Speaker-B0029RA-2/305754399) outdoor Bluetooth speaker into a [Snapcast](https://github.com/snapcast/snapcast) client.

The design uses an [ESP32-S3 (Seeed XIAO)](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html) along with a [I2S DAC](https://www.amazon.com/dp/B08YNJGSN4) and [5-watt amplifier](https://www.amazon.com/dp/B0F6YBYMWY).

![Cover Photo](./images/cover_photo.jpg)

## 📁 Repo Layout

```
hardware/
  LICENSE          CERN-OHL-S-2.0
  libraries/       Footprint and symbol libraries used by both boards
  v1/              KiCad project and Gerbers, original 3 W design (MAX98357A)
  v2.0.1/          KiCad project and Gerbers, 5 W design (GY-PCM5102A + CH05D)
firmware/
  snapclient/      Upstream firmware (submodule, pinned to 5cda3a7)
  patches/         upstream-changes.patch: our changes to four upstream files
  overlay/         New files we add (status_led component, speaker-stake sdkconfigs)
  CHANGES.md       Every file and line changed from upstream, with rationale
scripts/
  setup-firmware.ps1  Applies the patch and overlay to firmware/snapclient
releases/
  max98357_combo/  Prebuilt flash images for the 3 W flavor
  pcm5102a_ch05d/  Prebuilt flash images for the 5 W flavor
```

## 💿 Firmware flavors

| Flavor | Hardware | Config | Release |
|---|---|---|---|
| 3 W combo | XIAO + MAX98357A | `firmware/sdkconfig.max98357_combo` | `releases/max98357_combo/` |
| ![v1 image](./images/v1.png) | | | |
| 5 W separate | XIAO + GY-PCM5102A + CH05D | `firmware/sdkconfig.pcm5102a_ch05d` | `releases/pcm5102a_ch05d/` |
| ![v2 image](./images/v2.png) | | | |

Both flavors are built from the same source tree. They differ only in DAC configuration. See `firmware/CHANGES.md` for the full list of changes from upstream.

## 📦 Flash a prebuilt release

Each release folder contains the four images needed for a full flash:

```
python -m esptool --chip esp32s3 -p COMx write_flash \
  0x0 bootloader.bin 0x8000 partition-table.bin 0x1d000 ota_data_initial.bin 0x20000 snapclient.bin
```

Replace `COMx` with the board's port. Run this from inside the release folder.

## 🛠 Set up ESP-IDF (Windows)

The firmware builds with ESP-IDF **v5.5.1**. Other versions are untested.

1. **Install Git for Windows** from https://git-scm.com/download/win if you don't have it.

2. **Clone ESP-IDF v5.5.1 with submodules.** This takes a while. In PowerShell:
   ```powershell
   mkdir C:\Espressif
   cd C:\Espressif
   git clone -b v5.5.1 --recursive https://github.com/espressif/esp-idf.git
   ```

3. **Install the toolchain for the ESP32-S3:**
   ```powershell
   cd C:\Espressif\esp-idf
   .\install.ps1 esp32s3
   ```
   This downloads the Xtensa compiler, CMake, Ninja, and a Python virtual environment. By default they go under `%USERPROFILE%\.espressif`. To put them elsewhere, set `IDF_TOOLS_PATH` before running the script, for example `$env:IDF_TOOLS_PATH = "C:\Espressif\tools"`.

   If PowerShell blocks the script, run `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` once and try again.

4. **Activate the environment in every new terminal.** From the `esp-idf` folder:
   ```powershell
   .\export.ps1
   ```
   If you set `IDF_TOOLS_PATH` in step 3, set it again in the terminal first.

5. **Check the install:**
   ```powershell
   idf.py --version
   ```
   It should print `ESP-IDF v5.5.1`.

6. **Connect the board.** The XIAO ESP32-S3 uses its built-in USB-C port, so no USB-serial driver is needed. If no COM port appears, try a different cable. Some cables only charge and don't carry data.

When `idf.py --version` works, continue to **Build from source** below.

## 📚 Build from source

First, clone the repo with its submodules and prepare the firmware. This only needs to be done once per clone:

```powershell
git clone --recursive https://github.com/cablesquirrel/snapcast-speaker-stake
cd snapcast-speaker-stake
.\scripts\setup-firmware.ps1
```

The script checks out upstream at the pinned commit, applies `firmware/patches/upstream-changes.patch`, and copies in the overlay files. It is safe to run again.

Requires ESP-IDF 5.5.1 (see above). Build each flavor from `firmware/snapclient` into its own directory:

```
cd firmware\snapclient
idf.py -B build -D SDKCONFIG=sdkconfig.max98357_combo build
idf.py -B build.pcm5102a_ch05d -D SDKCONFIG=sdkconfig.pcm5102a_ch05d build
```

## 📶 OTA update

With the device on the network, push a new app image to its OTA listener on port 8032. Use a direct file reference so curl sends a Content-Length header:

```
curl <device-ip>:8032 --data-binary @releases/pcm5102a_ch05d/snapclient.bin
```

The connection is reset when the transfer completes. That is expected: the device reboots into the new image.

## 📱 WiFi

WiFi credentials are not compiled in. Provision them over USB-C with Improv WiFi (for example, web.esphome.io).

## 📝 Hardware notes

- Footprint and symbol libraries used by the boards are in `hardware/libraries/`. Each project's `fp-lib-table` and `sym-lib-table` reference them with `${KIPRJMOD}/../libraries/`, so the projects open from a fresh clone. Add the libraries to your global KiCad tables only if you need them outside these projects.
- `hardware/libraries/` contains the Seeed XIAO series, GY-PCM5102A, CH05D (provisional, see its KNOWN_ISSUES), and Adafruit MAX98357A libraries as copied from the local KiCad setup.
- The V2.0.1 known issues are listed in `hardware/v2.0.1/KNOWN_ISSUES.md`. That board was sent to fab with them.

## ⚖ Licensing

- **Hardware** (`hardware/`): [CERN-OHL-S-2.0](hardware/LICENSE), the strongly reciprocal open-source hardware license. The GY-PCM5102A footprint in this project derives from a CERN-OHL-S library, and that license requires derivatives to stay under the same terms.
- **Firmware** (`firmware/`): upstream [CarlosDerSeher/snapclient](https://github.com/CarlosDerSeher/snapclient) is pulled in as a submodule under GPL-3.0 (see `firmware/snapclient/LICENSE`). Our changes are listed in `firmware/CHANGES.md`. Because they modify GPL-3.0 code, they are also GPL-3.0. The firmware overlay files (`status_led` and the sdkconfigs) are included in that same GPL-3.0 grant.
- **Third-party libraries**: the Adafruit MAX98357A library (`adafruit-MAX98357.*`) comes from [besi/kicad-adafruit-MAX98357](https://github.com/besi/kicad-adafruit-MAX98357), and the Seeed XIAO library (`Seeed_Studio_XIAO_Series.*`) comes from Seeed Studio. The Seeed library is CC-BY-SA-4.0 (see `hardware/libraries/LICENSES/`). The Adafruit library has no declared license upstream and is included as-is, pending a takedown request if the owner objects. See `hardware/libraries/README.md`.
