# Changes from upstream

Upstream source: https://github.com/CarlosDerSeher/snapclient, pinned as the submodule `firmware/snapclient` at commit `5cda3a7` (GPL-3.0).

The four modified upstream files are in `patches/upstream-changes.patch`. New files are in `overlay/`. `scripts/setup-firmware.ps1` applies both. Each diff below is taken from that patch.

## Modified files

### `components/network_interface/CMakeLists.txt`

Adds the `status_led` component to the dependencies.

```diff
@@ -1,3 +1,3 @@
 idf_component_register(SRCS "network_interface.c" "eth_interface.c" "wifi_interface.c"
                        INCLUDE_DIRS "include"
-                       PRIV_REQUIRES driver esp_wifi esp_eth esp_netif esp_timer nvs_flash improv_wifi)
+                       PRIV_REQUIRES driver esp_wifi esp_eth esp_netif esp_timer nvs_flash improv_wifi status_led)
diff --git a/components/network_interface/wifi_interface.c b/components/network_interface/wifi_interface.c
index ae620b6..748b832 100644
```

### `components/network_interface/wifi_interface.c`

Drives the status LED from WiFi state. Also adds a **temporary debug log** on disconnect. Upstream logs this at `ESP_LOGV`, which is hidden by default, so the log is left in place.

```diff
@@ -21,6 +21,7 @@
 #include "network_interface.h"
 #include "nvs_flash.h"
 #include "sdkconfig.h"
+#include "status_led.h"
 
 #if ENABLE_WIFI_PROVISIONING
 #include "wifi_provisioning.h"
@@ -87,6 +88,9 @@ static void event_handler(void *arg, esp_event_base_t event_base, int event_id,
     esp_wifi_connect();
   } else if (event_base == WIFI_EVENT &&
              event_id == WIFI_EVENT_STA_DISCONNECTED) {
+    wifi_event_sta_disconnected_t *disconn = (wifi_event_sta_disconnected_t *)event_data;
+    ESP_LOGI(TAG, "DEBUG: WiFi disconnected, reason: %d", disconn->reason);
+    status_led_set_state(STATUS_LED_STATE_CONNECTING);
     if ((s_retry_num < WIFI_MAXIMUM_RETRY) || (WIFI_MAXIMUM_RETRY == 0)) {
       xSemaphoreTake(connIpSemaphoreHandle, portMAX_DELAY);
       connected = false;
@@ -140,6 +144,10 @@ static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
   ESP_LOGI(TAG, "~~~~~~~~~~~");
 
   s_retry_num = 0;
+
+  // Audio isn't flowing yet at this point -- main.c bumps this to READY
+  // once the first PCM chunk actually gets inserted into the player.
+  status_led_set_state(STATUS_LED_STATE_WAITING);
 }
 
 static void lost_ip_event_handler(void *arg, esp_event_base_t event_base,
@@ -160,6 +168,8 @@ static void lost_ip_event_handler(void *arg, esp_event_base_t event_base,
   xSemaphoreGive(connIpSemaphoreHandle);
 
   ESP_LOGI(TAG, "Wifi Lost IP Address");
+
+  status_led_set_state(STATUS_LED_STATE_CONNECTING);
 }
 
 /**
diff --git a/main/CMakeLists.txt b/main/CMakeLists.txt
index 9e89ca1..6857951 100644
```

### `main/CMakeLists.txt`

Adds the `status_led` component to the dependencies.

```diff
@@ -2,7 +2,7 @@ idf_component_register(SRCS "main.c" "connection_handler.c"
                        INCLUDE_DIRS "."
 
                        PRIV_REQUIRES esp_timer esp_wifi nvs_flash audio_board audio_hal audio_sal net_functions opus flac ota_server
-                       				 ui_http_server network_interface custom_board settings_manager tas5805m_settings udp_logger
+                       				 ui_http_server network_interface custom_board settings_manager tas5805m_settings udp_logger status_led
                        )
 
 set_source_files_properties(main.c PROPERTIES COMPILE_FLAGS -Wno-implicit-fallthrough)
diff --git a/main/main.c b/main/main.c
index 06b070d..145715b 100644
```

### `main/main.c`

Four changes:

- **Volume-slider fix.** `server_settings_msg_received()` applies mute and volume only when they change from the tracked state. That state is reset on every connection, so a first message matching the reset values was skipped. The first message of each connection is now always applied (`forceApply`).
- **Muted-at-boot fix.** `scSet` was uninitialized, so the first unmute could be skipped. It now starts as `{0}` with `muted = true`, matching the hardware.
- **Version string.** The hello message reports ESP-IDF's git-derived version and the build timestamp, so an OTA push can be confirmed from the Snapcast web UI. Upstream sends a static string.
- **OTA fix.** The OTA task's stack is 8 KB instead of 3584 bytes. The smaller stack overflowed during transfers (confirmed from a crash log). The OTA server source is unchanged from upstream.

Also adds LED state changes, a `status_led_init()` call, and the `receivedFirstSettings` flag threaded through `process_data()`.

```diff
@@ -34,6 +34,7 @@
 #include "nvs_flash.h"
 
 #include "esp32_udp_logger.h"
+#include "esp_app_desc.h"
 
 // Web socket server
 // #include "websocket_if.h"
@@ -58,6 +59,7 @@
 #include "settings_manager.h"
 #include "snapcast.h"
 #include "snapcast_protocol_parser.h"
+#include "status_led.h"
 #include "ui_http_server.h"
 #if CONFIG_DAC_TAS5805M
 #include "tas5805m_settings.h"
@@ -549,16 +551,30 @@ void audio_set_volume(int volume) {
  */
 void server_settings_msg_received(
     server_settings_message_t *server_settings_message,
-    snapcastSetting_t *scSet) {
+    snapcastSetting_t *scSet, bool *receivedFirstSettings) {
   // log mute state, buffer, latency
   ESP_LOGI(TAG, "Buffer length:  %ld", server_settings_message->buffer_ms);
   ESP_LOGI(TAG, "Latency:        %ld", server_settings_message->latency);
   ESP_LOGI(TAG, "Mute:           %d", server_settings_message->muted);
   ESP_LOGI(TAG, "Setting volume: %ld", server_settings_message->volume);
 
+  // On the first settings message of a new connection, force mute/volume to
+  // apply unconditionally, even if the reported values happen to match the
+  // freshly-reset scSet defaults (muted=true, volume=0). Without this, a
+  // real server-reported state that coincidentally matches those defaults
+  // (e.g. volume genuinely at 0, or an unmuted stream whose first message
+  // races in before a distinct value arrives) gets silently treated as "no
+  // change" by the comparisons below, and the DAC/DSP never actually gets
+  // told the real state for this connection -- same failure class as the
+  // muted-at-boot bug above, just triggered by connection state instead of
+  // stack garbage. This was observed as intermittent "volume slider does
+  // nothing" that only cleared after a few reconnects.
+  bool forceApply = !(*receivedFirstSettings);
+  *receivedFirstSettings = true;
+
   // Volume setting using ADF HAL
   // abstraction
-  if (scSet->muted != server_settings_message->muted) {
+  if (forceApply || (scSet->muted != server_settings_message->muted)) {
 #if SNAPCAST_USE_SOFT_VOL
     if (server_settings_message->muted) {
       dsp_processor_set_volome(0.0);
@@ -569,7 +585,7 @@ void server_settings_msg_received(
     audio_set_mute(server_settings_message->muted);
   }
 
-  if (scSet->volume != server_settings_message->volume) {
+  if (forceApply || (scSet->volume != server_settings_message->volume)) {
 #if SNAPCAST_USE_SOFT_VOL
     if (!server_settings_message->muted) {
       dsp_processor_set_volome((double)server_settings_message->volume / 100);
@@ -810,6 +826,7 @@ void handle_chunk_message(codec_type_t codec, snapcastSetting_t *scSet,
 #endif
 
         insert_pcm_chunk(new_pcmChunk);
+        status_led_set_state(STATUS_LED_STATE_READY);
       }
 
       if (player_send_snapcast_setting(scSet) != pdPASS) {
@@ -906,6 +923,7 @@ void handle_chunk_message(codec_type_t codec, snapcastSetting_t *scSet,
 #endif
 
         insert_pcm_chunk(new_pcmChunk);
+        status_led_set_state(STATUS_LED_STATE_READY);
 //        free_pcm_chunk(new_pcmChunk);
 //        new_pcmChunk = NULL;
       }
@@ -969,6 +987,7 @@ void handle_chunk_message(codec_type_t codec, snapcastSetting_t *scSet,
 #endif
       if (*pcmData) {
         insert_pcm_chunk(*pcmData);
+        status_led_set_state(STATUS_LED_STATE_READY);
       }
 
       *pcmData = NULL;
@@ -998,7 +1017,7 @@ void handle_chunk_message(codec_type_t codec, snapcastSetting_t *scSet,
 int process_data(snapcast_protocol_parser_t *parser,
                  time_sync_data_t *time_sync_data, bool *received_codec_header,
                  codec_type_t *codec, snapcastSetting_t *scSet,
-                 pcm_chunk_message_t **pcmData) {
+                 pcm_chunk_message_t **pcmData, bool *receivedFirstSettings) {
   base_message_t base_message_rx;
 
   if (parse_base_message(parser, &base_message_rx) != PARSER_OK) {
@@ -1051,7 +1070,8 @@ int process_data(snapcast_protocol_parser_t *parser,
       if (parse_sever_settings_message(parser, &base_message_rx, &server_settings_message) != PARSER_OK) {
         return -1;
       }
-      server_settings_msg_received(&server_settings_message, scSet);
+      server_settings_msg_received(&server_settings_message, scSet,
+                                   receivedFirstSettings);
       return 0;
     }
 
@@ -1110,13 +1130,34 @@ static void http_get_task(void *pvParameters) {
   hello_message_t hello_message;
   char *hello_message_serialized = NULL;
   static char device_hostname[64] = {0};  // Buffer for hostname
+  // Reported to snapserver in the hello message, and visible there as each
+  // client's "version" -- lets you confirm an OTA push actually landed
+  // without needing a serial connection. Combines ESP-IDF's own git-derived
+  // app version with the exact compile timestamp, since the git version
+  // alone doesn't change between two builds with no new commits (e.g. local
+  // edits between OTA pushes), but the timestamp always does.
+  static char version_string[80] = {0};
+  if (version_string[0] == '\0') {
+    const esp_app_desc_t *app_desc = esp_app_get_description();
+    snprintf(version_string, sizeof(version_string), "%s (built %s %s)",
+             app_desc->version, app_desc->date, app_desc->time);
+  }
   int result;
   time_sync_data_t time_sync_data;
   time_sync_data.lastTimeSync = 0;
   time_sync_data.lastTimeSyncSent = 0;
   bool received_codec_header = false;
+  bool receivedFirstSettings = false;
   codec_type_t codec = NONE;
-  snapcastSetting_t scSet;
+  // Zero-init, and explicitly track the real post-boot hardware state
+  // (max98357_init() drives the SD/mute GPIO LOW = muted at boot). Without
+  // this, scSet.muted starts as uninitialized stack garbage, and if it
+  // happens to already match the server's first "muted" value, the
+  // change-detection in server_settings_msg_received() silently skips the
+  // very first audio_set_mute() call, leaving the amp stuck in shutdown
+  // forever even though the client is happily streaming audio.
+  snapcastSetting_t scSet = {0};
+  scSet.muted = true;
   pcm_chunk_message_t *pcmData = NULL;
 
   // create a timer to send time sync messages every x µs
@@ -1135,6 +1176,7 @@ static void http_get_task(void *pvParameters) {
 //      esp_timer_stop(time_sync_data.timeSyncMessageTimer);
 
       received_codec_header = false;
+      receivedFirstSettings = false;
 
       xSemaphoreTake(idCounterSemaphoreHandle, portMAX_DELAY);
       id_counter = 0;
@@ -1215,7 +1257,7 @@ static void http_get_task(void *pvParameters) {
     }
     hello_message.hostname = device_hostname;
 
-    hello_message.version = (char *)VERSION_STRING;
+    hello_message.version = version_string;
     hello_message.client_name = "libsnapcast";
     hello_message.os = "esp32";
     hello_message.arch = "xtensa";
@@ -1296,7 +1338,7 @@ static void http_get_task(void *pvParameters) {
     while (1) {
       int result =
           process_data(&parser, &time_sync_data, &received_codec_header, &codec,
-                       &scSet, &pcmData);
+                       &scSet, &pcmData, &receivedFirstSettings);
       if (result != 0) {
         break;  // restart connection
       }
@@ -1370,6 +1412,8 @@ void app_main(void) {
   esp_log_level_set("UI_HTTP", ESP_LOG_WARN);
   esp_log_level_set("dspProc", ESP_LOG_DEBUG);
 
+  status_led_init();  // starts in STATUS_LED_STATE_CONNECTING (fast blink)
+
 #if CONFIG_SNAPCLIENT_USE_INTERNAL_ETHERNET || \
     CONFIG_SNAPCLIENT_USE_SPI_ETHERNET
   // clang-format off
@@ -1544,7 +1588,13 @@ void app_main(void) {
   dsp_settings_init();   // Then settings can restore params into the processor
 #endif
 
-  xTaskCreatePinnedToCore(&ota_server_task, "ota", 14 * 256, NULL,
+  // 14*256=3584 bytes was too small -- confirmed via a real crash log:
+  // "A stack overflow in task ota has been detected", triggered almost
+  // immediately after accepting a connection, with lwip_netconn_do_getaddr
+  // in the backtrace. The task does raw sockets, esp_ota_write() (which
+  // calls into the SPI flash driver), and apparently touches lwIP netconn
+  // internals -- 8KB gives real margin over the observed overflow.
+  xTaskCreatePinnedToCore(&ota_server_task, "ota", 8 * 1024, NULL,
                           OTA_TASK_PRIORITY, &t_ota_task, OTA_TASK_CORE_ID);
 
   xTaskCreatePinnedToCore(&http_get_task, "http", 15 * 1024, NULL,
```

## Unchanged from upstream

- `components/ota_server/ota_server.c` is identical to upstream.

## New files (`firmware/overlay/`)

Not in upstream. `scripts/setup-firmware.ps1` copies them into the submodule.

- `components/status_led/`: single-LED state indicator for the XIAO's onboard LED on GPIO21. States: connecting (fast blink), waiting (slow blink), ready (solid).
- `sdkconfig.max98357_combo`: build config for the XIAO + MAX98357A (3 W) flavor. `CONFIG_DAC_MAX98357=y`, `CONFIG_MAX98357_MUTE_PIN=7`, PSRAM octal, Improv WiFi provisioning, status LED on GPIO21.
- `sdkconfig.pcm5102a_ch05d`: build config for the XIAO + GY-PCM5102A + CH05D (5 W) flavor. Identical to the above except `CONFIG_DAC_PCM5102A=y` and `CONFIG_PCM5102A_MUTE_PIN=7`.
- `dependencies.lock`: pins the managed components (esp-dsp, mdns, and others) that the firmware was built and tested with. Upstream gitignores this file, so without it fresh clones resolve newer, unpinned versions and produce different binaries.

Build each flavor from `firmware/snapclient`, each into its own directory:

```
idf.py -B build -D SDKCONFIG=sdkconfig.max98357_combo build
idf.py -B build.pcm5102a_ch05d -D SDKCONFIG=sdkconfig.pcm5102a_ch05d build
```

## Submodule state

`firmware/snapclient` and its nested submodules (`flac`, `improv_wifi`, `opus`, `udp_logger`) are upstream at their pinned commits. Apart from the patch and overlay, they have no local changes.

After setup, `git status` inside `firmware/snapclient` shows the four modified files and the untracked overlay files. This is expected.

## Not included

- The local `sdkconfig` (generated and git-ignored). The two named variants above are the committed configs.
- WiFi credentials. These are provisioned at runtime via Improv WiFi.
