#include "tarea_pulsador.h"

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "pines.h"
#include "pulsador.h"

#define VENTANA_ANTIRREBOTE_US (CONFIG_SEYTR_GUARDA_MS * 1000LL)
#define LONGITUD_COLA CONFIG_SEYTR_TAMANO_COLA

typedef struct
{
    int64_t instante_us;
} evento_pulsador_t;

static const char *TAG = "traza-pulsador";
static pulsador_t s_pulsador;
static QueueHandle_t s_cola;

void pulsador_isr(void *argumento)
{
    (void)argumento;

    const evento_pulsador_t evento = {.instante_us = esp_timer_get_time()};
    BaseType_t debe_ceder = pdFALSE;
    xQueueSendFromISR(s_cola, &evento, &debe_ceder);
    if (debe_ceder == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

void tarea_pulsador(void *argumento)
{
    (void)argumento;

    /* El cero inicial permite aceptar la primera pulsación tras el arranque. */
    int64_t ultima_aceptada_us = 0;

    for (;;)
    {
        evento_pulsador_t evento;
        if (xQueueReceive(s_cola, &evento, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }

        if (evento.instante_us - ultima_aceptada_us < VENTANA_ANTIRREBOTE_US)
        {
            continue;
        }

        ultima_aceptada_us = esp_timer_get_time();
        ESP_LOGI(TAG, "Pulsador activado");
    }
}

static esp_err_t configurar_pulsador(void)
{
    esp_err_t err =
        pulsador_iniciar(&s_pulsador, PIN_PULSADOR, PULSADOR_NIVEL_PULSADO);
    if (err != ESP_OK)
    {
        return err;
    }

    /* El montaje usa una resistencia externa de polarización. */
    err = pulsador_polarizar(&s_pulsador, false);
    if (err != ESP_OK)
    {
        return err;
    }

    s_cola = xQueueCreate(LONGITUD_COLA, sizeof(evento_pulsador_t));
    return s_cola == NULL ? ESP_ERR_NO_MEM : ESP_OK;
}

esp_err_t tarea_pulsador_iniciar(void)
{
    esp_err_t err = configurar_pulsador();
    if (err != ESP_OK)
    {
        return err;
    }

    err = gpio_install_isr_service(0);
    if (err != ESP_OK)
    {
        return err;
    }

    err = gpio_isr_handler_add(PIN_PULSADOR, pulsador_isr, NULL);
    if (err != ESP_OK)
    {
        return err;
    }

    if (xTaskCreate(tarea_pulsador, "entrada", 4096, NULL, 10, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "No hay memoria suficiente para crear la tarea del pulsador");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

int tarea_pulsador_nivel(void)
{
    return pulsador_nivel(&s_pulsador);
}

void tarea_pulsador_mostrar_trazas(bool mostrar)
{
    esp_log_level_set(TAG, mostrar ? ESP_LOG_INFO : ESP_LOG_NONE);
}
