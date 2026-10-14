/*
 * Aplicacion principal del Taller 1.
 *
 * Este archivo reúne las tareas y la inicialización de los periféricos para
 * las prácticas de LED, pulsador y sensor BME280. Actualmente, app_main()
 * instala únicamente el controlador de UART0; las tareas y los periféricos
 * declarados aquí todavía no se crean ni se inicializan.
 *
 * La UART0 se conecta normalmente al ordenador mediante el puente USB-UART de
 * la placa. Los pines de los periféricos se configuran en pines.h.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/uart_vfs.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_timer.h"
#include "freertos/queue.h"

#include "led.h"
#include "pines.h"
#include "pulsador.h"
#include "bus_i2c.h"
#include "bme280.h"

/* Etiqueta de los mensajes de arranque de la aplicación. */
static const char *TAG = "taller-1";

/*--------------------LED--------------------*/
static led_t s_led;

/* Duración de cada fase del parpadeo, derivada de la configuración del
 * proyecto. El tiempo apagado completa el período después del encendido.
 */
#define PERIODO_LED_MS CONFIG_SEYTR_PERIODO_LED_MS
#define CICLO_TRABAJO_ENCENDIDO CONFIG_SEYTR_DUTY_CYCLE_LED / 100
#define ENCENDIDO_MS (PERIODO_LED_MS * CICLO_TRABAJO_ENCENDIDO)
#define APAGADO_MS (PERIODO_LED_MS - ENCENDIDO_MS)

/*--------------------PULSADOR--------------------*/
static pulsador_t s_pulsador;

/* Datos temporales que se toman durante la interrupción. Se emplea para implementar
 * una guarda temporal y filtrar el rebote del pulsaador.
 */
typedef struct
{
    int64_t us;
} evento_t;

#define VENTANA_ANTIRREBOTE_US (CONFIG_SEYTR_GUARDA_MS * 1000LL)
#define LONGITUD_COLA CONFIG_SEYTR_TAMANO_COLA

static QueueHandle_t s_cola;

/* ------------------------------------------------------------------------
 * La tarea LED: Alterna el LED según el período configurado
 * ------------------------------------------------------------------------ */

static void tarea_parpadeo(void *argumento)
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

/* ------------------------------------------------------------------------
 * La tarea pulasdor:
 * ------------------------------------------------------------------------ */

static void IRAM_ATTR btn_isr(void *arg)
{
    evento_t e = {.us = esp_timer_get_time()};
    BaseType_t hp = pdFALSE;
    xQueueSendFromISR(s_cola, &e, &hp);
    if (hp)
        portYIELD_FROM_ISR();
}

static void tarea_entrada(void *argumento)
{
    (void)argumento;

    /* Permite aceptar la primera pulsación desde el arranque. */
    int64_t ultima_aceptada_us = 0;

    for (;;)
    {
        evento_t e;
        xQueueReceive(s_cola, &e, portMAX_DELAY);

        if (e.us - ultima_aceptada_us < VENTANA_ANTIRREBOTE_US)
            continue;

        ultima_aceptada_us = esp_timer_get_time();

        ESP_LOGI(TAG, "Pulsador activado");
    }
}

void app_main(void)
{
    /* Informa de la frecuencia del tick configurada en FreeRTOS. */
    ESP_LOGI(TAG, "SEyTR - Taller 1. Tick a %d Hz", configTICK_RATE_HZ);

    if (configTICK_RATE_HZ != 1000)
    {
        ESP_LOGW(TAG, "el tic no esta a 1000 Hz.");
    }

    ESP_ERROR_CHECK(led_iniciar(&s_led, PIN_LED, LED_NIVEL_ENCENDIDO));

    ESP_ERROR_CHECK(pulsador_iniciar(&s_pulsador, PIN_PULSADOR, PULSADOR_NIVEL_PULSADO));
    /*Se desactiva la resistencia de interna porque el montaje está pensado para pull-up externo.*/
    ESP_ERROR_CHECK(pulsador_polarizar(&s_pulsador, false));

    s_cola = xQueueCreate(LONGITUD_COLA, sizeof(evento_t));
    if (s_cola == NULL)
    {
        ESP_LOGE(TAG, "no hay memoria para las colas");
        return;
    }

    /* El servicio de interrupciones de GPIO se instala una sola vez para todo
     * el proyecto. Con 0, sin ESP_INTR_FLAG_IRAM. */
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_PULSADOR, btn_isr, NULL));

    /* «entrada» tiene la prioridad más alta de las tres, para que ni la
     * consola ni el teclado retrasen la lectura del pin. */
    const BaseType_t parpadeo =
        xTaskCreate(tarea_parpadeo, "parpadeo", 2048, NULL, 5, NULL);
    const BaseType_t entrada =
        xTaskCreate(tarea_entrada, "entrada", 4096, NULL, 10, NULL);
    if (parpadeo != pdPASS || entrada != pdPASS)
    {
        ESP_LOGE(TAG, "no hay memoria para las tareas");
        return;
    }

    /* app_main termina aquí; las tres tareas siguen. */
}
