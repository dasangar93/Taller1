 /*
 * Qué hace este programa
 * ----------------------
 * Hace parpadear un LED con un periodo fijo
 *
 * Qué hardware necesita
 * ---------------------
 * La placa del curso, conectada por su conector USB-a-UART: es por donde
 * llegan las teclas. El LED, con su resistencia en serie, es opcional: su pin
 * se declara en pines.h, y sin LED el programa mide igual.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#include "led.h"
#include "pines.h"
#include "pulsador.h"
#include "I2C.h"
#include "BME280.h"

static led_t s_led;
static pulsador_t s_pulsador;
static TaskHandle_t s_tarea_entrada;
static bus_i2c_t s_bus_i2c;
static bme280_t s_bme280;


#define ENCENDIDO_MS      500
#define APAGADO_MS        500
#define DIRECCION_BME280  0x76u

static void tarea_parpadeo(void *argumento)
{
    (void)argumento;

    TickType_t referencia = xTaskGetTickCount();

    for (;;) {
        led_encender(&s_led);
        xTaskDelayUntil(&referencia, pdMS_TO_TICKS(ENCENDIDO_MS));

        led_apagar(&s_led);
        xTaskDelayUntil(&referencia, pdMS_TO_TICKS(APAGADO_MS));
    }
}

static void tarea_leer_entrada(void *argumento)
{
    (void)argumento;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        const int nivel = pulsador_nivel(&s_pulsador);
        ESP_LOGI("pulsador", "nivel del pin: %d", nivel);

        const bool pulsado = pulsador_pulsado(&s_pulsador);
        ESP_LOGI("pulsador", "pulsado: %s", pulsado ? "true" : "false");
    }
}

static void IRAM_ATTR leer_entrada(void *argumento)
{
    (void)argumento;

    BaseType_t tarea_despertada = pdFALSE;
    vTaskNotifyGiveFromISR(s_tarea_entrada, &tarea_despertada);
    portYIELD_FROM_ISR(tarea_despertada);
}

void app_main(void)
{
    led_iniciar(&s_led, PIN_LED, LED_NIVEL_ENCENDIDO);
    xTaskCreate(tarea_parpadeo, "parpadeo", 2048, NULL, 5, NULL);

    bus_i2c_iniciar(&s_bus_i2c, PIN_SDA, PIN_SCL, false);
    ESP_LOGI("i2c", "Bus I2C inicializado (SDA=%d, SCL=%d)", PIN_SDA, PIN_SCL);

    ESP_ERROR_CHECK(bme280_iniciar(&s_bme280, s_bus_i2c.manejador,
                                  DIRECCION_BME280, 100000));
    ESP_LOGI("bme280", "Sensor BME280 conectado y configurado");

    ESP_ERROR_CHECK(bme280_iniciar_tarea_lectura(&s_bme280));

    ESP_ERROR_CHECK(pulsador_iniciar(&s_pulsador, PIN_PULSADOR,
                                     PULSADOR_NIVEL_PULSADO));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(xTaskCreate(tarea_leer_entrada, "leer_entrada", 2048,
                                NULL, 5, &s_tarea_entrada) == pdPASS
                        ? ESP_OK
                        : ESP_FAIL);
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_PULSADOR, leer_entrada, NULL));
}