/* bme280.h - el sensor BME280 como dispositivo del bus I2C.
 *
 * Solo lo identifica: cuelga el dispositivo del bus en su dirección y lee el
 * registro de identificación. Leer las medidas, que llegan como valores en
 * bruto, y compensarlas con los coeficientes del propio sensor es trabajo del
 * Taller 1, no de este ejemplo.
 *
 * Direcciones y registro, según la hoja de datos del BME280 de Bosch Sensortec:
 * 0x76 con la patilla SDO a masa y 0x77 con SDO a la alimentación. El registro
 * de identificación está en 0xD0 y vale 0x60.
 */
#pragma once

#include <stdint.h>
#include "driver/i2c_master.h"

#define BME280_REG_ID 0xD0
#define BME280_ID 0x60

typedef struct
{
    i2c_master_dev_handle_t manejador; /* NULL mientras no está colgado */
    uint8_t direccion;                 /* de 7 bits, sin desplazar       */
    uint32_t velocidad_hz;
} bme280_t;

/* Cuelga el sensor del bus en la dirección dada, de 7 bits y sin desplazar:
 * el controlador añade el bit de sentido. Si ya está colgado, no hace nada y
 * devuelve ESP_ERR_INVALID_STATE. */
esp_err_t bme280_iniciar(bme280_t *s, i2c_master_bus_handle_t bus,
                         uint8_t direccion, uint32_t velocidad_hz);

/* Descuelga el sensor del bus. */
esp_err_t bme280_quitar(bme280_t *s);

/* Lee el registro de identificación: escribe su dirección y lee un byte, sin
 * soltar el bus entre las dos cosas. Devuelve el error del controlador tal
 * cual, para poder enseñarlo. */
esp_err_t bme280_leer_id(bme280_t *s, uint8_t *id);
