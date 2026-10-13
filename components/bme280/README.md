# Componente `bme280`

El sensor BME280 como dispositivo del bus I2C. **Solo lo identifica**: lo
cuelga del bus en su dirección y lee el registro de identificación. Leer las
medidas, que llegan como valores en bruto, y compensarlas con los
coeficientes del propio sensor es trabajo del Taller 1.

Según la hoja de datos del BME280, de Bosch Sensortec, la dirección es 0x76
con la patilla SDO a masa y 0x77 con SDO a la alimentación, y el registro de
identificación, en 0xD0, vale 0x60.

No crea el bus: lo recibe ya creado. En el código de `main`:

```c
#include "bme280.h"

static bme280_t sensor;

/* bus: un bus_i2c_t ya creado con bus_i2c_iniciar(). El sensor recibe su
 * manejador, el de ESP-IDF. */
ESP_ERROR_CHECK(bme280_iniciar(&sensor, bus.manejador, 0x76, 100000));
uint8_t id;
if (bme280_leer_id(&sensor, &id) == ESP_OK && id == BME280_ID) {
    /* es un BME280 */
}
```

- `bme280_iniciar()` cuelga el sensor del bus en una dirección de 7 bits, sin
  desplazar: el controlador añade el bit de sentido. Si el sensor ya está
  colgado, devuelve `ESP_ERR_INVALID_STATE`. El último argumento es la
  velocidad de reloj de sus transacciones, en hercios.
- `bme280_quitar()` lo descuelga. Para cambiar de dirección se quita y se
  vuelve a iniciar.
- `bme280_leer_id()` escribe la dirección del registro y lee un byte, sin
  soltar el bus entre las dos cosas, con una espera máxima de 100 ms. Devuelve
  el error del controlador tal cual, para poder enseñarlo.

Depende de `esp_driver_i2c`, declarado en `REQUIRES` porque su cabecera
`driver/i2c_master.h` aparece en la cabecera pública del componente.
