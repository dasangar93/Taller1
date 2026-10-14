#pragma once

#include "esp_err.h"

/* Configura el LED y crea la tarea que lo alterna. */
esp_err_t tarea_led_iniciar(void);

/* Punto de entrada de la tarea FreeRTOS del LED. */
void tarea_led(void *argumento);
