#pragma once

#include "esp_err.h"

/* Inicializa el bus I2C y el sensor, comprueba su configuración y crea la
 * tarea que informa temperatura, presión y humedad cada dos segundos.
 */
esp_err_t sensor_bme280_iniciar(void);
