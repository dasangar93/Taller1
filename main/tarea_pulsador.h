#pragma once

#include <stdbool.h>

#include "esp_attr.h"
#include "esp_err.h"

/* Manejador GPIO: encola el instante de la interrupción para filtrar rebotes. */
void IRAM_ATTR pulsador_isr(void *argumento);

/* Configura el pulsador, registra la interrupción y crea su tarea. */
esp_err_t tarea_pulsador_iniciar(void);

/* Devuelve el nivel lógico actual del GPIO. */
int tarea_pulsador_nivel(void);

/* Activa o desactiva los mensajes de detección del pulsador. */
void tarea_pulsador_mostrar_trazas(bool mostrar);

/* Punto de entrada de la tarea FreeRTOS que filtra las pulsaciones. */
void tarea_pulsador(void *argumento);