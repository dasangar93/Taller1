/* linea.h - acumular y normalizar una línea de la consola.
 *
 * Es el segundo de los cuatro bloques de la consola: leer, acumular y
 * normalizar, analizar y responder. Recibe los caracteres de uno en uno, tal
 * como llegan por la UART, y avisa cuando hay una línea completa.
 *
 * No depende de ESP-IDF: es C estándar y se puede probar en el ordenador.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char *buf;          /* memoria de la línea, que pone quien la usa      */
    size_t cap;         /* tamaño de buf, contando el '\0' final           */
    size_t largo;       /* caracteres guardados ahora                       */
    bool desbordada;    /* la línea en curso no cabía: se descartará       */
    bool normalizar;    /* false: la consola «ingenua», para ver el fallo  */
    bool tras_cr;       /* el último carácter recibido fue un CR           */
} linea_t;

typedef enum {
    LINEA_SIGUE,        /* el carácter se ha guardado o ignorado: sigue    */
    LINEA_COMPLETA,     /* hay una línea en buf, terminada en '\0'         */
    LINEA_VACIA,        /* fin de línea sin nada dentro: no hay que hacer nada */
    LINEA_DESCARTADA,   /* fin de una línea que no cabía en buf            */
    LINEA_BORRADO,      /* se ha borrado el último carácter (retroceso)    */
} linea_estado_t;

/* Prepara la línea sobre una memoria de cap bytes. cap tiene que ser 2 o más. */
void linea_iniciar(linea_t *l, char *buf, size_t cap, bool normalizar);

/* Entrega un carácter recibido. Cuando devuelve LINEA_COMPLETA, la línea
 * está en l->buf hasta la siguiente llamada.
 *
 * Con normalizar a true:
 *   - El retorno de carro (CR) y el salto de línea (LF) terminan la línea, un
 *     LF que llega justo detrás de un CR se ignora y una línea vacía no se
 *     entrega. Así da igual que el terminal mande CR, LF o los dos.
 *   - El retroceso borra el último carácter, y los demás caracteres de
 *     control se ignoran.
 *   - Al terminar, se recortan los espacios y tabuladores de los extremos.
 * Con normalizar a false, la consola «ingenua»: CR y LF también terminan la
 * línea, pero no se recorta nada, cualquier otro carácter se guarda tal cual
 * y una línea vacía se entrega como completa. Sirve para ver por qué hace
 * falta normalizar.
 *
 * En los dos casos la línea está acotada: si llegan más caracteres de los que
 * caben, se descartan hasta el fin de línea, y entonces se devuelve
 * LINEA_DESCARTADA en lugar de entregar una línea cortada. */
linea_estado_t linea_recibir(linea_t *l, char c);
