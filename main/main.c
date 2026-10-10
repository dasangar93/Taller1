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

#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#include "Led.h"
#include "pines.h"

static led_t s_led;

#define ENCENDIDO_MS  500;
#define APAGADO_MS    500;

static void tarea_parpadeo(void *argumento)
{
    (void)argumento;

    TickType_t referencia = xTaskGetTickCount();

    led_encender(&s_led);
    xTaskDelayUntil(referencia, pdMS_TO_TICKS(duracion_ms));

    led_apagar(&s_led);
    xTaskDelayUntil(referencia, pdMS_TO_TICKS(duracion_ms));
}


void app_main(void)
{
    led_iniciar(&s_led, LED, LED_NIVEL_ENCENDIDO);
    tarea_parpadeo();

}