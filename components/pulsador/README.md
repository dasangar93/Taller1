# Componente `pulsador`

Un pulsador en un pin de entrada digital. Configura el pin como entrada, con la
resistencia de polarización interna que corresponde al montaje y la
interrupción por el flanco que va hacia el nivel de pulsado; permite quitar y
volver a poner esa resistencia, y lee el nivel.

Como el componente `led`, **no sabe en qué pin está el pulsador ni cómo está
montado**: los recibe al iniciarse.

Lo que **no** hace es atender la interrupción. El componente deja el pin
preparado para interrumpir, pero la rutina de interrupción, la cola o la
notificación y el antirrebote son del programa que lo usa: es lo que se
estudia en U2-S2, y cada programa los resuelve a su manera.

En el código de `main`, que es quien sabe el pin:

```c
#include "pulsador.h"
#include "pines.h"   /* el fichero de pines del proyecto, no del componente */

static pulsador_t pulsador;

void app_main(void)
{
    ESP_ERROR_CHECK(pulsador_iniciar(&pulsador, PIN_PULSADOR, PULSADOR_NIVEL_PULSADO));
    if (pulsador_pulsado(&pulsador)) {
        /* ... */
    }
}
```

- `nivel_pulsado` es el nivel lógico que se lee con el pulsador apretado: 0 si
  está montado entre el pin y masa, con pull-up, que es lo habitual; 1 si está
  entre el pin y la alimentación, con pull-down. Lo decide el montaje, no el
  código.
- `pulsador_iniciar()` deja la interrupción pedida en el flanco de bajada con
  `nivel_pulsado` 0, y en el de subida con 1, pero desactivada. Se activa al
  añadir un manejador para el pin con `gpio_isr_handler_add()`, después de
  instalar el servicio de interrupciones de GPIO con
  `gpio_install_isr_service()`; `gpio_isr_handler_remove()` la vuelve a
  desactivar.
- `pulsador_polarizar(&pulsador, false)` quita la resistencia interna: sin
  ella, y sin una externa, el pin queda flotante. Sirve para verlo.
- `pulsador_iniciar()` devuelve `ESP_ERR_INVALID_ARG` si el puntero es nulo o
  si el pin no está elegido o no existe, y el error de `gpio_config()` si la
  configuración falla. A diferencia del LED, no admite `GPIO_NUM_NC`: sin
  pulsador no hay nada que leer.

Depende de `esp_driver_gpio`, declarado en `REQUIRES` porque su cabecera
`driver/gpio.h` aparece en la cabecera pública del componente.
