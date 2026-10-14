/*
 * Aplicacion principal del Taller 1.
 *
 * Este archivo coordina la inicialización de los periféricos, el registro de
 * interrupciones y la creación de las tareas de LED y pulsador. La lógica de
 * cada tarea se mantiene en su propio módulo.
 *
 * La UART0 se conecta normalmente al ordenador mediante el puente USB-UART de
 * la placa. Los pines de los periféricos se configuran en pines.h.
 */

#include "esp_err.h"

#include "sensor_bme280.h"
#include "tarea_led.h"
#include "tarea_pulsador.h"

void app_main(void)
{
    ESP_ERROR_CHECK(tarea_led_iniciar());
    ESP_ERROR_CHECK(tarea_pulsador_iniciar());
    ESP_ERROR_CHECK(sensor_bme280_iniciar());
}
