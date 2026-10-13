# Componente `led`

Un LED en un pin de salida digital. Configura el pin como salida, lo deja
apagado y ofrece encenderlo, apagarlo y conmutarlo.

El componente **no sabe en qué pin está el LED ni cómo está montado**: los
recibe al iniciarse. Así el número del pin se escribe en un solo sitio, el
fichero de pines del proyecto, y el componente sirve igual en otro proyecto.

En el código de `main`, que es quien sabe el pin:

```c
#include "led.h"
#include "pines.h"   /* el fichero de pines del proyecto, no del componente */

static led_t led;

void app_main(void)
{
    ESP_ERROR_CHECK(led_iniciar(&led, PIN_LED, LED_NIVEL_ENCENDIDO));
    led_encender(&led);
    led_apagar(&led);
    led_conmutar(&led);
}
```

- `nivel_encendido` es el nivel lógico que enciende el LED: 1 si está montado
  entre el pin y masa, 0 si está entre el pin y la alimentación. Lo decide el
  montaje, no el código.
- Con `GPIO_NUM_NC` como pin, el componente no toca ningún pin y solo lleva la
  cuenta del estado. Sirve para trabajar sin LED.
- `led_iniciar()` escribe el nivel de apagado **antes** de configurar el pin
  como salida, para que el LED no llegue a encenderse al arrancar.
- `led_iniciar()` devuelve `ESP_ERR_INVALID_ARG` si el puntero es nulo o si el
  pin no existe o no puede ser salida, y el error de `gpio_config()` si la
  configuración falla.

Depende de `esp_driver_gpio`, declarado en `REQUIRES` porque su cabecera
`driver/gpio.h` aparece en la cabecera pública del componente.
