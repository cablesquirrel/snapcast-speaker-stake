# Changes from upstream

Upstream: https://github.com/CarlosDerSeher/snapclient at commit `5cda3a7` (branch `develop`, merge commit "Merge branch 'develop'").

The upstream source is pulled in as the submodule `firmware/snapclient`, pinned to `5cda3a7`. The changes below are in `patches/upstream-changes.patch`, which modifies only four existing upstream files. New files are kept separately in `overlay/` and copied in by `scripts/setup-firmware.ps1`.

Setup is covered in the root README. In short:

```
git clone --recursive https://github.com/cablesquirrel/snapcast-speaker-stake
cd snapcast-speaker-stake
.\scripts\setup-firmware.ps1
```

Line references are to the **upstream** file. "after L N" means the new lines are inserted after upstream line N. "L N-M" means upstream lines N through M are replaced or changed.

## Modified files

### `components/network_interface/CMakeLists.txt`
- L3: added `status_led` to `PRIV_REQUIRES`.

### `components/network_interface/wifi_interface.c`
- after L23: `#include "status_led.h"` (1 line).
- after L89, `event_handler()`, `WIFI_EVENT_STA_DISCONNECTED` (3 lines):
  - `ESP_LOGI(TAG, "DEBUG: WiFi disconnected, reason: %d", ...)`. **Temporary debug log, left in place.** Upstream logs this at `ESP_LOGV`, which is hidden at the default log level.
  - `status_led_set_state(STATUS_LED_STATE_CONNECTING)`.
- after L142, `got_ip_event_handler()` (4 lines): `status_led_set_state(STATUS_LED_STATE_WAITING)`, with a comment.
- after L162, `lost_ip_event_handler()` (2 lines): `status_led_set_state(STATUS_LED_STATE_CONNECTING)`.

### `main/CMakeLists.txt`
- L5: added `status_led` to `PRIV_REQUIRES`.

### `main/main.c`
- after L36: `#include "esp_app_desc.h"` (1 line).
- after L60: `#include "status_led.h"` (1 line).
- L552 (1 line changed) and after L558 (14 lines), in `server_settings_msg_received()`:
  - new parameter `bool *receivedFirstSettings` (L552 signature).
  - `forceApply` logic: on the first settings message of each connection, mute and volume are applied unconditionally. Otherwise they are applied only on change, as upstream does.
  - **Fixes** an intermittent bug where the volume slider appeared to do nothing after reconnects.
- L561 and L572 (changed): mute and volume conditions now include `forceApply ||`.
- after L812, after L908, after L971: `status_led_set_state(STATUS_LED_STATE_READY)` after each successful `insert_pcm_chunk()` (OPUS, FLAC, and PCM paths).
- L1001 (changed): `process_data()` signature gains `bool *receivedFirstSettings`.
- L1054 (changed, +1 line): `server_settings_msg_received()` call passes `receivedFirstSettings`.
- L1110 (13 lines added), in `http_get_task()`, before the declarations:
  - `static char version_string[80]`, filled once from `esp_app_get_description()` (ESP-IDF's git-derived version plus the compile date and time).
  - `bool receivedFirstSettings = false;`
- L1119 (changed to 9 lines): upstream declares `snapcastSetting_t scSet;` with no initializer. Now `snapcastSetting_t scSet = {0}; scSet.muted = true;`, so the client starts in the muted state the hardware is actually in. This fixes the amp staying muted when the first server message reports `muted=false`.
- after L1135 (1 line), in the connection housekeeping block at the top of the loop: `receivedFirstSettings = false;`, so each new connection gets its own first-message apply.
- L1218 (changed): `hello_message.version = version_string;` replaces `(char *)VERSION_STRING`. Upstream sends a static string.
- L1299 (changed): `process_data(...)` call passes `&receivedFirstSettings`.
- after L1372 (2 lines), in `app_main()`: `status_led_init();` and a comment.
- L1547 (changed, +7 lines), in `app_main()`, OTA task creation: stack size changed from `14 * 256` (3584 bytes) to `8 * 1024` (8192 bytes), with a comment.
  - **Fixes** OTA. The receive task overflowed its stack during transfers (confirmed from a crash log: "stack overflow in task ota").

## Unchanged from upstream

- `components/ota_server/ota_server.c`: identical to upstream. The OTA fix is the stack size in `main/main.c` above, not a change to this file.

## New files (`firmware/overlay/`)

These files do not exist upstream. `scripts/setup-firmware.ps1` copies them into the submodule.

- `overlay/components/status_led/` (CMakeLists.txt, Kconfig.projbuild, include/status_led.h, status_led.c): single-LED state indicator for the XIAO's onboard LED on GPIO21. States: connecting (fast blink), waiting (slow blink), ready (solid).
- `overlay/sdkconfig.max98357_combo`: full build config for the XIAO + MAX98357A (3 W) flavor. Uses `CONFIG_DAC_MAX98357=y`, `CONFIG_MAX98357_MUTE_PIN=7`, PSRAM octal, Improv WiFi provisioning, and the status LED on GPIO21.
- `overlay/sdkconfig.pcm5102a_ch05d`: full build config for the XIAO + GY-PCM5102A + CH05D (5 W) flavor. Identical to the above except `CONFIG_DAC_PCM5102A=y` and `CONFIG_PCM5102A_MUTE_PIN=7`.
- `overlay/dependencies.lock`: pins the managed component versions (esp-dsp, mdns, ...) the firmware was built and tested with. Upstream gitignores this file, so without it fresh clones resolve newer, unpinned versions and produce different binaries.

Build each flavor into its own directory (from `firmware/snapclient`):
```
idf.py -B build -D SDKCONFIG=sdkconfig.max98357_combo build
idf.py -B build.pcm5102a_ch05d -D SDKCONFIG=sdkconfig.pcm5102a_ch05d build
```

## Submodule state

`firmware/snapclient` and its nested submodules (`flac`, `improv_wifi`, `opus`, `udp_logger`) are upstream at their pinned commits, with no local changes beyond the patch and overlay above. The flac build makefiles that were deleted in an earlier working copy are back, because they are part of upstream's own flac submodule.

After the patch is applied, `git status` inside `firmware/snapclient` shows the four modified files and the untracked overlay files. This is expected.

## Not included

- The local `sdkconfig` (generated and git-ignored). The two named variants above are the committed configs.
- WiFi credentials. These are provisioned at runtime via Improv WiFi.
