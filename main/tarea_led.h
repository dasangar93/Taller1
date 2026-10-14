#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef enum
{
    TAREA_LED_ENCENDER,
    TAREA_LED_APAGAR,
    TAREA_LED_CONMUTAR,
    TAREA_LED_AUTOMATICO
} tarea_led_accion_t;

/* Configura el LED y crea la tarea que lo alterna. */
esp_err_t tarea_led_iniciar(void);

/* Envía una acción a la tarea propietaria del LED. */
esp_err_t tarea_led_controlar(tarea_led_accion_t accion);

/* Devuelve el último estado escrito al LED. */
bool tarea_led_encendido(void);

/* Activa o desactiva las trazas periódicas del LED. */
void tarea_led_mostrar_trazas(bool mostrar);

/* Punto de entrada de la tarea FreeRTOS del LED. */
void tarea_led(void *argumento);
