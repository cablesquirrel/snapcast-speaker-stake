#include "status_led.h"

#include "sdkconfig.h"

#if CONFIG_STATUS_LED_ENABLE

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "STATUS_LED";

static volatile status_led_state_t s_state = STATUS_LED_STATE_CONNECTING;

#if CONFIG_STATUS_LED_ACTIVE_LOW
#define LED_ON_LEVEL 0
#define LED_OFF_LEVEL 1
#else
#define LED_ON_LEVEL 1
#define LED_OFF_LEVEL 0
#endif

static void status_led_task(void *pv) {
  (void)pv;
  bool on = false;

  while (1) {
    switch (s_state) {
      case STATUS_LED_STATE_READY:
        gpio_set_level(CONFIG_STATUS_LED_GPIO, LED_ON_LEVEL);
        vTaskDelay(pdMS_TO_TICKS(200));
        break;

      case STATUS_LED_STATE_WAITING:
        on = !on;
        gpio_set_level(CONFIG_STATUS_LED_GPIO, on ? LED_ON_LEVEL : LED_OFF_LEVEL);
        vTaskDelay(pdMS_TO_TICKS(600));
        break;

      case STATUS_LED_STATE_CONNECTING:
      default:
        on = !on;
        gpio_set_level(CONFIG_STATUS_LED_GPIO, on ? LED_ON_LEVEL : LED_OFF_LEVEL);
        vTaskDelay(pdMS_TO_TICKS(150));
        break;
    }
  }
}

void status_led_init(void) {
  gpio_reset_pin(CONFIG_STATUS_LED_GPIO);
  gpio_set_direction(CONFIG_STATUS_LED_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_level(CONFIG_STATUS_LED_GPIO, LED_OFF_LEVEL);

  BaseType_t r = xTaskCreate(status_led_task, "status_led", 2048, NULL,
                             tskIDLE_PRIORITY + 1, NULL);
  if (r != pdPASS) {
    ESP_LOGW(TAG, "failed to create status_led task");
  }
}

void status_led_set_state(status_led_state_t state) { s_state = state; }

#else  // !CONFIG_STATUS_LED_ENABLE

void status_led_init(void) {}
void status_led_set_state(status_led_state_t state) { (void)state; }

#endif
