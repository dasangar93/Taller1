# Componente `bus_i2c`

El bus I2C en modo maestro y su exploración. Crea el bus en dos pines, lo
elimina y lo explora. Los dispositivos no los cuelga este componente, sino
quien los maneja: en este proyecto, el componente `bme280`.

**No sabe en qué pines está el bus**: los recibe al iniciarse. En el código de
`main`, que es quien sabe los pines:

```c
#include "bus_i2c.h"
#include "pines.h"   /* el fichero de pines del proyecto, no del componente */

static bus_i2c_t bus;

void app_main(void)
{
    ESP_ERROR_CHECK(bus_i2c_iniciar(&bus, PIN_SDA, PIN_SCL, false));

    bus_i2c_exploracion_t r;
    bus_i2c_explorar(&bus, &r);
    for (int i = 0; i < r.n_encontradas; i++) {
        printf("0x%02X\n", r.encontradas[i]);
    }
}
```

- `bus_i2c_iniciar()` crea el bus en el puerto 0 con el pull-up interno
  desactivado: en I2C las resistencias son externas. Con el último argumento a
  `true` intercambia SDA y SCL, que es lo mismo que cruzar los dos cables sin
  tocar el montaje. Devuelve el error de `i2c_new_master_bus()`,
  `ESP_ERR_INVALID_ARG` si algún pin no es válido o `ESP_ERR_INVALID_STATE` si
  el bus ya existe.
- `bus_i2c_eliminar()` elimina el bus. Antes hay que haber quitado sus
  dispositivos.
- `bus_i2c_explorar()` prueba las direcciones de 0x08 a 0x77 con
  `i2c_master_probe()`, con una espera de 50 ms cada una, y anota cuáles
  responden y cuántas fallan con cada error: `ESP_ERR_NOT_FOUND` si nadie
  reconoce la dirección y `ESP_ERR_TIMEOUT` si la operación no llega a
  completarse. Si cuatro direcciones seguidas agotan el tiempo, se detiene.
  Mientras explora, calla las trazas del controlador, que escribiría un error
  por cada tiempo agotado: los cuenta ella y los resume al final.

Depende de `esp_driver_i2c`, declarado en `REQUIRES` porque su cabecera
`driver/i2c_master.h` aparece en la cabecera pública del componente.
