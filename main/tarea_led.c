#include "tarea_led.h"

#include <inttypes.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "led.h"
#include "pines.h"

#define PERIODO_LED_MS CONFIG_SEYTR_PERIODO_LED_MS
#define ENCENDIDO_MS ((PERIODO_LED_MS * CONFIG_SEYTR_DUTY_CYCLE_LED) / 100)
#define APAGADO_MS (PERIODO_LED_MS - ENCENDIDO_MS)
#define LONGITUD_COLA_COMANDOS_LED 4

static led_t s_led;
static const char *TAG = "traza-led";
static QueueHandle_t s_cola_comandos;
static portMUX_TYPE s_mux_estado = portMUX_INITIALIZER_UNLOCKED;
static bool s_estado_led;

static void escribir_estado(bool encendido)
{
    if (encendido)
    {
        led_encender(&s_led);
    }
    else
    {
        led_apagar(&s_led);
    }

    portENTER_CRITICAL(&s_mux_estado);
    s_estado_led = encendido;
    portEXIT_CRITICAL(&s_mux_estado);
}

static void aplicar_accion(tarea_led_accion_t accion, bool *automatico)
{
    switch (accion)
    {
    case TAREA_LED_ENCENDER:
        *automatico = false;
        escribir_estado(true);
        break;
    case TAREA_LED_APAGAR:
        *automatico = false;
        escribir_estado(false);
        break;
    case TAREA_LED_CONMUTAR:
        *automatico = false;
        escribir_estado(!tarea_led_encendido());
        break;
    case TAREA_LED_AUTOMATICO:
        *automatico = true;
        break;
    default:
        break;
    }
}

esp_err_t tarea_led_iniciar(void)
{
    s_cola_comandos = xQueueCreate(LONGITUD_COLA_COMANDOS_LED,
                                   sizeof(tarea_led_accion_t));
    if (s_cola_comandos == NULL)
    {
        ESP_LOGE(TAG, "No hay memoria suficiente para la cola de comandos del LED");
        return ESP_ERR_NO_MEM;
    }

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

esp_err_t tarea_led_controlar(tarea_led_accion_t accion)
{
    if (s_cola_comandos == NULL ||
        (unsigned)accion > (unsigned)TAREA_LED_AUTOMATICO)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return xQueueSend(s_cola_comandos, &accion, 0) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

bool tarea_led_encendido(void)
{
    portENTER_CRITICAL(&s_mux_estado);
    const bool encendido = s_estado_led;
    portEXIT_CRITICAL(&s_mux_estado);
    return encendido;
}

void tarea_led_mostrar_trazas(bool mostrar)
{
    esp_log_level_set(TAG, mostrar ? ESP_LOG_INFO : ESP_LOG_NONE);
}

void tarea_led(void *argumento)
{
    (void)argumento;

    bool automatico = true;

    for (;;)
    {
        tarea_led_accion_t accion;
        if (!automatico)
        {
            if (xQueueReceive(s_cola_comandos, &accion, portMAX_DELAY) == pdTRUE)
            {
                aplicar_accion(accion, &automatico);
            }
            continue;
        }

        const int64_t inicio_periodo_us = esp_timer_get_time();
        escribir_estado(true);
        if (xQueueReceive(s_cola_comandos, &accion,
                          pdMS_TO_TICKS(ENCENDIDO_MS)) == pdTRUE)
        {
            aplicar_accion(accion, &automatico);
            continue;
        }

        const int64_t fin_encendido_us = esp_timer_get_time();
        escribir_estado(false);
        if (xQueueReceive(s_cola_comandos, &accion,
                          pdMS_TO_TICKS(APAGADO_MS)) == pdTRUE)
        {
            aplicar_accion(accion, &automatico);
            continue;
        }

        const int64_t fin_periodo_us = esp_timer_get_time();
        ESP_LOGI(TAG,
                 "LED: encendido=%" PRId64 " us, apagado=%" PRId64
                 " us, periodo=%" PRId64 " us",
                 fin_encendido_us - inicio_periodo_us,
                 fin_periodo_us - fin_encendido_us,
                 fin_periodo_us - inicio_periodo_us);
    }
}
