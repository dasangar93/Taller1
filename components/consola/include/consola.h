/* consola.h - una consola de comandos sobre la UART del monitor.
 *
 * Cuatro bloques separados:
 *   1. Leer: una tarea saca de la UART los caracteres que hayan llegado.
 *   2. Acumular y normalizar: linea.c forma la línea y la deja limpia.
 *   3. Analizar: se separa el nombre del comando de sus argumentos y se busca
 *      en una tabla de comandos, no en una cadena de comparaciones.
 *   4. Responder: siempre, también a un comando desconocido.
 *
 * La tabla la pone quien usa la consola: el componente no sabe qué comandos
 * hay. Añadir un comando es añadir una fila.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

typedef struct {
    const char *nombre;         /* lo que se teclea                           */
    const char *argumentos;     /* cómo se escriben, para la ayuda. NULL si no admite */
    const char *ayuda;          /* una línea que explica qué hace              */
    void (*ejecutar)(const char *args);  /* args: "" si no hay argumentos      */
} consola_comando_t;

typedef struct {
    int puerto;                 /* la UART del monitor: la 0 (ver abajo)       */
    size_t largo_maximo;        /* caracteres que caben en una línea           */
    const consola_comando_t *tabla;
    size_t n_comandos;
} consola_config_t;

/* Instala el controlador de la UART en el orden de la documentación
 * —instalar, configurar, asignar pines— y arranca la tarea de la consola.
 *
 * El puerto tiene que ser el del monitor, la UART0 por omisión: la consola
 * no le asigna pines, porque ya los tiene, y printf y las trazas siguen
 * saliendo por él. Se reconfigura a la velocidad que fija la configuración
 * de ESP-IDF para el monitor, 115200 baudios por omisión, y 8N1, para que el
 * monitor la siga entendiendo. */
esp_err_t consola_iniciar(const consola_config_t *cfg);

/* Escribe la lista de comandos a partir de la tabla. */
void consola_ayuda(void);

/* Activa o desactiva la normalización de la línea. Sin ella, la consola se
 * comporta como una escrita sin cuidado: ver linea.h. */
void consola_normalizar(bool activar);
bool consola_normaliza(void);
