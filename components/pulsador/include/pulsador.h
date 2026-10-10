/* pulsador.h - un pulsador en un pin de entrada digital.
 *
 * Cabecera pública del componente. Como el del LED, el componente no sabe en
 * qué pin está el pulsador ni cómo está montado: se lo dice quien lo inicia.
 *
 * El componente se ocupa del pin: lo configura como entrada, activa o quita la
 * resistencia de polarización interna y lee el nivel. Lo que hace el programa
 * cuando llega una pulsación, y cómo se entera, no es cosa suya: la rutina de
 * interrupción, la cola o la notificación y el antirrebote están en main, que
 * es donde se estudian.
 */
#pragma once

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Estado de un pulsador. Se declara en quien lo usa y se pasa por puntero. */
typedef struct {
    gpio_num_t pin;            /* GPIO del pulsador */
    int        nivel_pulsado;  /* nivel lógico que se lee con el pulsador apretado: 0 o 1 */
    bool       polarizado;     /* si la resistencia de polarización interna está activada */
} pulsador_t;

/* Configura el pin como entrada, con la resistencia de polarización interna
 * que corresponde al montaje activada, y con la interrupción por el flanco que
 * va hacia el nivel de pulsado. La interrupción queda desactivada hasta que se
 * le añade un manejador con gpio_isr_handler_add(), que la activa.
 *
 * pin: el GPIO del pulsador. Tiene que ser un pin válido: sin pulsador no hay
 *      nada que medir, y GPIO_NUM_NC da error.
 * nivel_pulsado: 0 si el pulsador va entre el pin y masa, que es el montaje de
 *      la sesión, con pull-up y lógica invertida; 1 si va entre el pin y
 *      3,3 V, con pull-down.
 *
 * Devuelve ESP_OK; ESP_ERR_INVALID_ARG si pulsador es NULL o si el pin no
 * existe; o el error que devuelva gpio_config().
 */
esp_err_t pulsador_iniciar(pulsador_t *pulsador, gpio_num_t pin, int nivel_pulsado);

/* Activa o quita la resistencia de polarización interna. Sin ella, y sin una
 * externa, el pin queda flotante y su lectura no está determinada. */
esp_err_t pulsador_polarizar(pulsador_t *pulsador, bool activa);

/* Nivel lógico del pin, 0 o 1, tal como lo da gpio_get_level(). */
int pulsador_nivel(const pulsador_t *pulsador);

/* true si el nivel del pin es el de pulsado. */
bool pulsador_pulsado(const pulsador_t *pulsador);

#ifdef __cplusplus
}
#endif
