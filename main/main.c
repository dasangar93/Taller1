 /*
 * Qué hace este programa
 * ----------------------
 * Lee comandos por UART0 para activar el parpadeo del LED, la lectura del
 * pulsador o la comunicacion I2C con el BME280.
 *
 * Qué hardware necesita
 * ---------------------
 * La placa del curso, conectada por su conector USB-a-UART: es por donde
 * llegan las teclas. El LED, el pulsador y el BME280 son opcionales; sus pines
 * se declaran en pines.h.
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
#include "I2C.h"
#include "BME280.h"

static led_t s_led;
static pulsador_t s_pulsador;
static TaskHandle_t s_tarea_entrada;
static bus_i2c_t s_bus_i2c;
static bme280_t s_bme280;
static bool s_led_iniciado;
static bool s_parpadeo_activo;
static bool s_pulsador_activo;
static bool s_lectura_bme280_activa;

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

static void iniciar_led(void)
{
    if (s_parpadeo_activo) {
        ESP_LOGI("consola", "El parpadeo del LED ya esta activo");
        return;
    }

    if (!s_led_iniciado) {
        const esp_err_t error = led_iniciar(&s_led, PIN_LED, LED_NIVEL_ENCENDIDO);
        if (error != ESP_OK) {
            ESP_LOGE("consola", "No se pudo iniciar el LED: %s",
                     esp_err_to_name(error));
            return;
        }
        s_led_iniciado = true;
    }

    if (xTaskCreate(tarea_parpadeo, "parpadeo", 2048, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE("consola", "No se pudo crear la tarea de parpadeo");
        return;
    }

    s_parpadeo_activo = true;
    ESP_LOGI("consola", "Parpadeo del LED iniciado");
}

static void iniciar_pulsador(void)
{
    if (s_pulsador_activo) {
        ESP_LOGI("consola", "La lectura del pulsador ya esta activa");
        return;
    }

    esp_err_t error = pulsador_iniciar(&s_pulsador, PIN_PULSADOR,
                                       PULSADOR_NIVEL_PULSADO);
    if (error != ESP_OK) {
        ESP_LOGE("consola", "No se pudo iniciar el pulsador: %s",
                 esp_err_to_name(error));
        return;
    }

    error = gpio_install_isr_service(0);
    if (error != ESP_OK && error != ESP_ERR_INVALID_STATE) {
        ESP_LOGE("consola", "No se pudo instalar el servicio ISR: %s",
                 esp_err_to_name(error));
        return;
    }

    if (xTaskCreate(tarea_leer_entrada, "leer_entrada", 2048, NULL, 5,
                    &s_tarea_entrada) != pdPASS) {
        ESP_LOGE("consola", "No se pudo crear la tarea de lectura del pulsador");
        s_tarea_entrada = NULL;
        return;
    }

    error = gpio_isr_handler_add(PIN_PULSADOR, leer_entrada, NULL);
    if (error != ESP_OK) {
        vTaskDelete(s_tarea_entrada);
        s_tarea_entrada = NULL;
        ESP_LOGE("consola", "No se pudo registrar la interrupcion del pulsador: %s",
                 esp_err_to_name(error));
        return;
    }

    s_pulsador_activo = true;
    ESP_LOGI("consola", "Lectura del pulsador iniciada");
}

static void iniciar_bme280(void)
{
    if (s_lectura_bme280_activa) {
        ESP_LOGI("consola", "La lectura del BME280 ya esta activa");
        return;
    }

    if (s_bus_i2c.manejador == NULL) {
        const esp_err_t error = bus_i2c_iniciar(&s_bus_i2c, PIN_SDA, PIN_SCL, false);
        if (error != ESP_OK) {
            ESP_LOGE("consola", "No se pudo iniciar el bus I2C: %s",
                     esp_err_to_name(error));
            return;
        }
        ESP_LOGI("i2c", "Bus I2C inicializado (SDA=%d, SCL=%d)", PIN_SDA, PIN_SCL);
    }

    if (s_bme280.manejador == NULL) {
        const esp_err_t error = bme280_iniciar(&s_bme280, s_bus_i2c.manejador,
                                               DIRECCION_BME280, 100000);
        if (error != ESP_OK) {
            if (s_bme280.manejador != NULL) {
                const esp_err_t limpieza = bme280_quitar(&s_bme280);
                if (limpieza != ESP_OK) {
                    ESP_LOGE("consola", "No se pudo liberar el BME280: %s",
                             esp_err_to_name(limpieza));
                }
            }
            ESP_LOGE("consola", "No se pudo iniciar el BME280: %s",
                     esp_err_to_name(error));
            return;
        }
        ESP_LOGI("bme280", "Sensor BME280 conectado y configurado");
    }

    const esp_err_t error = bme280_iniciar_tarea_lectura(&s_bme280);
    if (error != ESP_OK) {
        ESP_LOGE("consola", "No se pudo iniciar la lectura del BME280: %s",
                 esp_err_to_name(error));
        return;
    }

    s_lectura_bme280_activa = true;
    ESP_LOGI("consola", "Lectura del BME280 iniciada (cada 2 segundos)");
}

static void mostrar_ayuda(void)
{
    printf("\nComandos por UART0:\n"
           "  q - iniciar el parpadeo del LED\n"
           "  w - activar la lectura del pulsador\n"
           "  s - iniciar I2C y la lectura periodica del BME280\n"
           "Escribe una tecla para ejecutar el comando. "
           "Pulsa q, w o s; cualquier otra tecla es un error.\n\n");
    fflush(stdout);
}

void app_main(void)
{
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0));
    uart_vfs_dev_use_driver(UART_NUM_0);

    mostrar_ayuda();

    for (;;) {
        const int tecla = getchar();
        if (tecla == '\r' || tecla == '\n') {
            continue;
        }

        switch (tecla) {
        case 'q':
            iniciar_led();
            break;
        case 'w':
            iniciar_pulsador();
            break;
        case 's':
            iniciar_bme280();
            break;
        default:
            ESP_LOGE("consola", "Tecla no valida (0x%02X). Usa q, w o s.",
                     (unsigned int)(unsigned char)tecla);
            break;
        }
    }
}