#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  STATUS_LED_STATE_CONNECTING,  // no WiFi yet (or WiFi lost) -- fast blink
  STATUS_LED_STATE_WAITING,     // WiFi up, not yet streaming audio -- slow blink
  STATUS_LED_STATE_READY,       // audio actively flowing -- solid on
} status_led_state_t;

// No-op if CONFIG_STATUS_LED_ENABLE is not set.
void status_led_init(void);

// No-op if CONFIG_STATUS_LED_ENABLE is not set.
void status_led_set_state(status_led_state_t state);

#ifdef __cplusplus
}
#endif
