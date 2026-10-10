/* led.h - un LED en un pin de salida digital.
 *
 * Cabecera pública del componente. El componente no sabe en qué pin está el
 * LED ni cómo está montado: se lo dice quien lo inicia. Así sirve en cualquier
 * proyecto, y el número del pin se escribe en un solo sitio, el fichero de
 * pines del proyecto que lo usa.
 */
#pragma once

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Estado de un LED. Se declara en quien lo usa y se pasa por puntero. */
typedef struct {
    gpio_num_t pin;              /* GPIO del LED, o GPIO_NUM_NC si no hay LED */
    int        nivel_encendido;  /* nivel lógico que lo enciende: 1 o 0 */
    bool       encendido;        /* último estado pedido */
} led_t;

/* Configura el pin como salida y deja el LED apagado. Hay que llamarla una
 * vez, antes que a las otras tres.
 *
 * pin: el GPIO del LED. Con GPIO_NUM_NC no se toca ningún pin y el componente
 *      solo lleva la cuenta del estado, para poder trabajar sin LED.
 * nivel_encendido: 1 si el LED está montado entre el pin y masa; 0 si está
 *      entre el pin y la alimentación.
 *
 * Devuelve ESP_OK; ESP_ERR_INVALID_ARG si led es NULL o si el pin no existe
 * o no puede ser salida; o el error que devuelva gpio_config().
 */
esp_err_t led_iniciar(led_t *led, gpio_num_t pin, int nivel_encendido);

void led_encender(led_t *led);
void led_apagar(led_t *led);
void led_conmutar(led_t *led);

#ifdef __cplusplus
}
#endif
