#include "tarea_led.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "led.h"
#include "pines.h"

#define PERIODO_LED_MS CONFIG_SEYTR_PERIODO_LED_MS
#define ENCENDIDO_MS ((PERIODO_LED_MS * CONFIG_SEYTR_DUTY_CYCLE_LED) / 100)
#define APAGADO_MS (PERIODO_LED_MS - ENCENDIDO_MS)

static led_t s_led;
static const char *TAG = "tarea-led";

esp_err_t tarea_led_iniciar(void)
{
    ESP_LOGI(TAG, "SEyTR - Taller 1. Tick a %d Hz", configTICK_RATE_HZ);
    if (configTICK_RATE_HZ != 1000)
    {
        ESP_LOGW(TAG, "El tick no esta configurado a 1000 Hz.");
    }

    esp_err_t err = led_iniciar(&s_led, PIN_LED, LED_NIVEL_ENCENDIDO);
    if (err != ESP_OK)
    {
        return err;
    }

    if (xTaskCreate(tarea_led, "parpadeo", 2048, NULL, 5, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "No hay memoria suficiente para crear la tarea del LED");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

void tarea_led(void *argumento)
{
    (void)argumento;

    TickType_t referencia = xTaskGetTickCount();

    for (;;)
    {
        led_encender(&s_led);
        xTaskDelayUntil(&referencia, pdMS_TO_TICKS(ENCENDIDO_MS));

        led_apagar(&s_led);
        xTaskDelayUntil(&referencia, pdMS_TO_TICKS(APAGADO_MS));
    }
}
