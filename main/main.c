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

#include "led.h"
#include "pines.h"
#include "pulsador.h"
#include "bus_i2c.h"
#include "bme280.h"

/* Etiqueta de los mensajes de arranque de la aplicación. */
static const char *TAG_init = "inicio";

/* Instancias compartidas por las tareas y las rutinas de interrupción. */
static led_t s_led;
static pulsador_t s_pulsador;
static TaskHandle_t s_tarea_entrada;
static bus_i2c_t s_bus_i2c;
static bme280_t s_bme280;

/* Duración de cada fase del parpadeo, derivada de la configuración del
 * proyecto. El tiempo apagado completa el período después del encendido.
 */
#define PERIODO_LED_MS CONFIG_SEYTR_PERIODO_LED_MS
#define CICLO_TRABAJO_ENCENDIDO CONFIG_SEYTR_DUTY_CYCLE_LED / 100
#define ENCENDIDO_MS (PERIODO_LED_MS * CICLO_TRABAJO_ENCENDIDO)
#define APAGADO_MS (PERIODO_LED_MS - ENCENDIDO_MS)

/* Dirección I2C de 7 bits configurada para el BME280. */
#define DIRECCION_BME280 0x76u

/* Alterna el LED según el período configurado. La espera hasta una referencia
 * fija mantiene el ritmo aunque una iteración se retrase.
 */
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

/* Atiende las notificaciones del pulsador y registra su nivel lógico y el
 * resultado de la interpretación del estado pulsado.
 */
static void tarea_leer_entrada(void *argumento)
{
    (void)argumento;

    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        const int nivel = pulsador_nivel(&s_pulsador);
        ESP_LOGI("pulsador", "nivel del pin: %d", nivel);

        const bool pulsado = pulsador_pulsado(&s_pulsador);
        ESP_LOGI("pulsador", "pulsado: %s", pulsado ? "true" : "false");
    }
}

/* Rutina de interrupción: despierta la tarea para procesar el pulsador fuera
 * del contexto de interrupción.
 */
static void IRAM_ATTR leer_entrada(void *argumento)
{
    (void)argumento;

    BaseType_t tarea_despertada = pdFALSE;
    vTaskNotifyGiveFromISR(s_tarea_entrada, &tarea_despertada);
    portYIELD_FROM_ISR(tarea_despertada);
}

void app_main(void)
{
    /* Informa de la frecuencia del tick configurada en FreeRTOS. */
    ESP_LOGI(TAG_init, "SEyTR - Taller 1. Tick a %d Hz",
             CONFIG_FREERTOS_HZ);

    ESP_ERROR_CHECK(led_iniciar(&s_led, PIN_LED, LED_NIVEL_ENCENDIDO));

    ESP_ERROR_CHECK(pulsador_iniciar(&s_pulsador, PIN_PULSADOR, PULSADOR_NIVEL_PULSADO));

    ESP_ERROR_CHECK(bus_i2c_iniciar(&s_bus_i2c, PIN_SDA, PIN_SCL, false));
    ESP_ERROR_CHECK(bme280_iniciar(&s_bme280, s_bus_i2c.manejador, DIRECCION_BME280, 100000));

    /* Instala UART0 con 256 bytes de recepción y enlaza su controlador a
     * la entrada/salida estándar gestionada por VFS.
     */
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0));
    uart_vfs_dev_use_driver(UART_NUM_0);

    // Preparar aquí el servicio y el manejador de interrupción del pulsador.

    if (xTaskCreate(tarea_parpadeo, "parpadeo", 2048, NULL, 5, NULL) != pdPASS)
    {
        ESP_LOGE(TAG_init, "No se pudo crear la tarea de parpadeo");
        return;
    }

    // Crear aquí las otras tareas, una vez listos sus recursos.
}
