#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct
{
    bool dispositivo_detectado;
    bool medicion_disponible;
    uint8_t direccion;
    uint8_t id;
    int32_t temperatura_centigrados;
    uint32_t presion_pa;
    uint32_t humedad_centiporcentaje;
} sensor_bme280_estado_t;

/* Inicializa el bus I2C y el sensor, comprueba su configuración y crea la
 * tarea que informa temperatura, presión y humedad cada dos segundos.
 */
esp_err_t sensor_bme280_iniciar(void);

/* Copia el último estado conocido del sensor. */
esp_err_t sensor_bme280_estado(sensor_bme280_estado_t *estado);

/* Activa o desactiva los mensajes periódicos de medidas. */
void sensor_bme280_mostrar_trazas(bool mostrar);
