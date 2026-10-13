/* bus_i2c.h - el bus I2C en modo maestro y su exploración.
 *
 * Primero se describe el bus y después se cuelgan de él los dispositivos. Este
 * componente crea el bus y lo explora. Los dispositivos los cuelga quien los
 * maneja: en este proyecto, el componente bme280.
 *
 * El componente no sabe en qué pines está el bus: los recibe al iniciarse.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c_master.h"

typedef struct
{
    i2c_master_bus_handle_t manejador; /* NULL mientras no hay bus          */
    gpio_num_t sda;                    /* los pines tal como los da main    */
    gpio_num_t scl;
    bool cruzado; /* SDA y SCL intercambiados a propósito */
} bus_i2c_t;

/* Resultado de una exploración. Las direcciones exploradas son las de
 * 0x08 a 0x77: por debajo y por encima están reservadas. */
typedef struct
{
    uint8_t encontradas[112];
    int n_encontradas;
    int n_sin_respuesta;  /* i2c_master_probe() dio ESP_ERR_NOT_FOUND      */
    int n_tiempo_agotado; /* dio ESP_ERR_TIMEOUT                            */
    int n_otros;          /* cualquier otro error                           */
    esp_err_t otro;       /* el último de esos otros errores                */
    bool interrumpida;    /* se paró tras varios tiempos agotados seguidos */
    int exploradas;
} bus_i2c_exploracion_t;

/* Crea el bus en los pines dados, con el pull-up interno desactivado: en I2C
 * las resistencias son externas, y en el montaje del taller las lleva la placa
 * del sensor. Con
 * cruzado a true, intercambia SDA y SCL, que es lo mismo que cruzar los dos
 * cables sin tocar el montaje. Si el bus ya existe, no hace nada y devuelve
 * ESP_ERR_INVALID_STATE. */
esp_err_t bus_i2c_iniciar(bus_i2c_t *b, gpio_num_t sda, gpio_num_t scl, bool cruzado);

/* Elimina el bus. Antes hay que haber quitado sus dispositivos. */
esp_err_t bus_i2c_eliminar(bus_i2c_t *b);

/* Prueba cada dirección con i2c_master_probe() y anota cuál responde y con
 * qué error responde cada una de las demás. Si cuatro direcciones seguidas
 * agotan el tiempo, para: el bus no funciona, y seguir solo haría esperar. */
void bus_i2c_explorar(bus_i2c_t *b, bus_i2c_exploracion_t *r);
