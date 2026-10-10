/* bme280.h - el sensor BME280 como dispositivo del bus I2C.
 *
 * Direcciones y registro, según la hoja de datos del BME280 de Bosch Sensortec:
 * 0x76 con la patilla SDO a masa y 0x77 con SDO a la alimentación. El registro
 * de identificación está en 0xD0 y vale 0x60.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

#define BME280_REG_ID        0xD0
#define BME280_ID            0x60

typedef struct {
    i2c_master_dev_handle_t manejador;  /* NULL mientras no está colgado */
    uint8_t direccion;                  /* de 7 bits, sin desplazar       */
    uint32_t velocidad_hz;
    bool calibrado;
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
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

/* Configura el sensor con oversampling mínimo y lee temperatura, presión y
 * humedad compensadas. Las unidades devueltas son: °C, hPa y %RH. */
esp_err_t bme280_leer_medicion(bme280_t *s, float *temperatura_c,
                              float *presion_hpa, float *humedad_rh);

/* Crea una tarea FreeRTOS que lee e imprime las mediciones cada dos segundos. */
esp_err_t bme280_iniciar_tarea_lectura(bme280_t *s);
