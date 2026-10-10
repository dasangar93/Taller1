/* bme280.c - el sensor BME280 como dispositivo del bus I2C. Ver BME280.h. */
#include "BME280.h"

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bme280";

/* Espera máxima de una transacción, en milisegundos. */
#define ESPERA_MS 100
#define INTERVALO_LECTURA_MS 2000

static esp_err_t bme280_escribir_registro(bme280_t *s, uint8_t reg,
                                          uint8_t valor)
{
    uint8_t trama[2] = {reg, valor};
    return i2c_master_transmit(s->manejador, trama, sizeof(trama), ESPERA_MS);
}

static esp_err_t bme280_leer_registros(bme280_t *s, uint8_t reg_inicio,
                                       uint8_t *datos, size_t n)
{
    if (datos == NULL || n == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_transmit_receive(s->manejador, &reg_inicio, 1, datos,
                                        n, ESPERA_MS);
}

static esp_err_t bme280_cargar_calibracion(bme280_t *s)
{
    uint8_t calibracion_t_p[24] = {0};
    uint8_t calibracion_h[7] = {0};
    uint8_t h1 = 0;
    esp_err_t err;

    err = bme280_leer_registros(s, 0x88, calibracion_t_p, sizeof(calibracion_t_p));
    if (err != ESP_OK) {
        return err;
    }

    err = bme280_leer_registros(s, 0xA1, &h1, 1);
    if (err != ESP_OK) {
        return err;
    }

    err = bme280_leer_registros(s, 0xE1, calibracion_h, sizeof(calibracion_h));
    if (err != ESP_OK) {
        return err;
    }

    s->dig_T1 = (uint16_t)calibracion_t_p[0] | ((uint16_t)calibracion_t_p[1] << 8);
    s->dig_T2 = (int16_t)((uint16_t)calibracion_t_p[2] |
                         ((uint16_t)calibracion_t_p[3] << 8));
    s->dig_T3 = (int16_t)((uint16_t)calibracion_t_p[4] |
                         ((uint16_t)calibracion_t_p[5] << 8));
    s->dig_P1 = (uint16_t)calibracion_t_p[6] | ((uint16_t)calibracion_t_p[7] << 8);
    s->dig_P2 = (int16_t)((uint16_t)calibracion_t_p[8] |
                         ((uint16_t)calibracion_t_p[9] << 8));
    s->dig_P3 = (int16_t)((uint16_t)calibracion_t_p[10] |
                         ((uint16_t)calibracion_t_p[11] << 8));
    s->dig_P4 = (int16_t)((uint16_t)calibracion_t_p[12] |
                         ((uint16_t)calibracion_t_p[13] << 8));
    s->dig_P5 = (int16_t)((uint16_t)calibracion_t_p[14] |
                         ((uint16_t)calibracion_t_p[15] << 8));
    s->dig_P6 = (int16_t)((uint16_t)calibracion_t_p[16] |
                         ((uint16_t)calibracion_t_p[17] << 8));
    s->dig_P7 = (int16_t)((uint16_t)calibracion_t_p[18] |
                         ((uint16_t)calibracion_t_p[19] << 8));
    s->dig_P8 = (int16_t)((uint16_t)calibracion_t_p[20] |
                         ((uint16_t)calibracion_t_p[21] << 8));
    s->dig_P9 = (int16_t)((uint16_t)calibracion_t_p[22] |
                         ((uint16_t)calibracion_t_p[23] << 8));

    s->dig_H1 = h1;
    s->dig_H2 = (int16_t)((uint16_t)calibracion_h[0] |
                         ((uint16_t)calibracion_h[1] << 8));
    s->dig_H3 = calibracion_h[2];
    s->dig_H4 = (int16_t)(((int16_t)calibracion_h[3] << 4) |
                          (calibracion_h[4] & 0x0F));
    s->dig_H5 = (int16_t)(((int16_t)calibracion_h[5] << 4) |
                          (calibracion_h[4] >> 4));
    s->dig_H6 = (int8_t)calibracion_h[6];

    s->calibrado = true;
    return ESP_OK;
}

esp_err_t bme280_iniciar(bme280_t *s, i2c_master_bus_handle_t bus,
                         uint8_t direccion, uint32_t velocidad_hz)
{
    ESP_RETURN_ON_FALSE(s && bus, ESP_ERR_INVALID_ARG, TAG, "puntero nulo");
    ESP_RETURN_ON_FALSE(s->manejador == NULL, ESP_ERR_INVALID_STATE, TAG,
                        "el sensor ya esta colgado: quitalo antes");

    memset(s, 0, sizeof(*s));
    s->direccion = direccion;
    s->velocidad_hz = velocidad_hz;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = direccion,
        .scl_speed_hz = velocidad_hz,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &s->manejador);
    if (err != ESP_OK) {
        s->manejador = NULL;
        return err;
    }

    err = bme280_escribir_registro(s, 0xF2, 0x01);   /* hum oversampling x1 */
    if (err == ESP_OK) {
        err = bme280_escribir_registro(s, 0xF4, 0x27); /* temp x1, pres x1, mode normal */
    }
    if (err == ESP_OK) {
        err = bme280_escribir_registro(s, 0xF5, 0x00); /* config por defecto */
    }
    if (err != ESP_OK) {
        bme280_quitar(s);
        return err;
    }

    return bme280_cargar_calibracion(s);
}

esp_err_t bme280_quitar(bme280_t *s)
{
    ESP_RETURN_ON_FALSE(s, ESP_ERR_INVALID_ARG, TAG, "puntero nulo");
    if (s->manejador == NULL) {
        return ESP_OK;
    }
    esp_err_t err = i2c_master_bus_rm_device(s->manejador);
    if (err == ESP_OK) {
        s->manejador = NULL;
        memset(s, 0, sizeof(*s));
    }
    return err;
}

esp_err_t bme280_leer_id(bme280_t *s, uint8_t *id)
{
    ESP_RETURN_ON_FALSE(s && s->manejador && id, ESP_ERR_INVALID_STATE, TAG,
                        "el sensor no esta colgado del bus");
    uint8_t reg = BME280_REG_ID;
    *id = 0;
    return i2c_master_transmit_receive(s->manejador, &reg, 1, id, 1, ESPERA_MS);
}

esp_err_t bme280_leer_medicion(bme280_t *s, float *temperatura_c,
                              float *presion_hpa, float *humedad_rh)
{
    ESP_RETURN_ON_FALSE(s && s->manejador, ESP_ERR_INVALID_STATE, TAG,
                        "el sensor no esta colgado del bus");
    if (temperatura_c == NULL && presion_hpa == NULL && humedad_rh == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s->calibrado) {
        esp_err_t err = bme280_cargar_calibracion(s);
        if (err != ESP_OK) {
            return err;
        }
    }

    uint8_t datos[8] = {0};
    esp_err_t err = bme280_leer_registros(s, 0xF7, datos, sizeof(datos));
    if (err != ESP_OK) {
        return err;
    }

    uint32_t adc_P = ((uint32_t)datos[0] << 12) | ((uint32_t)datos[1] << 4) |
                     ((uint32_t)datos[2] >> 4);
    uint32_t adc_T = ((uint32_t)datos[3] << 12) | ((uint32_t)datos[4] << 4) |
                     ((uint32_t)datos[5] >> 4);
    uint32_t adc_H = ((uint32_t)datos[6] << 8) | (uint32_t)datos[7];

    int32_t var1 = 0;
    int32_t var2 = 0;
    int32_t t_fine = 0;
    uint32_t presion = 0;

    var1 = (((adc_T >> 3) - ((int32_t)s->dig_T1 << 1)) * (int32_t)s->dig_T2) >> 11;
    var2 = (((((adc_T >> 4) - (int32_t)s->dig_T1) *
             ((adc_T >> 4) - (int32_t)s->dig_T1)) >> 12) *
            (int32_t)s->dig_T3) >>
           14;
    t_fine = var1 + var2;

    if (temperatura_c != NULL) {
        *temperatura_c = (float)((t_fine * 5 + 128) >> 8) / 100.0f;
    }

    if (presion_hpa != NULL) {
        var1 = (((int32_t)t_fine >> 1) - 64000);
        var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * (int32_t)s->dig_P6;
        var2 = var2 + ((var1 * (int32_t)s->dig_P5) << 1);
        var2 = (var2 >> 2) + ((int32_t)s->dig_P4 << 16);
        var1 = (((((int32_t)s->dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) +
                 (((int32_t)s->dig_P2 * var1) >> 1)) >>
                18);
        var1 = ((32768 + var1) * (int32_t)s->dig_P1) >> 15;
        if (var1 == 0) {
            return ESP_ERR_INVALID_STATE;
        }

        presion = (uint32_t)((((uint32_t)1048576 - adc_P) - (var2 >> 12)) * 3125);
        if (presion < 2147483648UL) {
            presion = (presion << 1) / (uint32_t)var1;
        } else {
            presion = (presion / (uint32_t)var1) * 2U;
        }

        var1 = ((int32_t)s->dig_P9 * (int32_t)(presion >> 3) * (int32_t)(presion >> 3)) >> 13;
        var2 = ((int32_t)(presion >> 2) * (int32_t)s->dig_P8) >> 13;
        presion = (uint32_t)((int32_t)presion + ((var1 + var2 + (int32_t)s->dig_P7) >> 4));
        *presion_hpa = presion / 100.0f;
    }

    if (humedad_rh != NULL) {
        int32_t var_h = ((((int32_t)t_fine) - ((int32_t)76800)));
        var_h = (((((adc_H << 14) - (((int32_t)s->dig_H4) << 20) -
                    (((int32_t)s->dig_H5) * var_h)) +
                   ((int32_t)16384)) >>
                  15) *
                 (((((((var_h * (int32_t)s->dig_H6) >> 10) *
                      (((var_h * (int32_t)s->dig_H3) >> 11) + (int32_t)32768)) >>
                     10) +
                    ((int32_t)2097152)) *
                   (int32_t)s->dig_H2 +
                   (int32_t)8192) >>
                  14));
        var_h = var_h - (((((var_h >> 15) * (var_h >> 15)) >> 7) *
                          (int32_t)s->dig_H1) >>
                         4);
        if (var_h < 0) {
            var_h = 0;
        }
        if (var_h > 419430400) {
            var_h = 419430400;
        }
        *humedad_rh = (float)(var_h >> 12) / 1024.0f;
        if (*humedad_rh > 100.0f) {
            *humedad_rh = 100.0f;
        }
    }

    return ESP_OK;
}

static void bme280_tarea_lectura(void *argumento)
{
    bme280_t *s = argumento;

    for (;;) {
        uint8_t id = 0;
        esp_err_t err = bme280_leer_id(s, &id);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "ID del sensor: 0x%02X", id);

            float temperatura_c = 0.0f;
            float presion_hpa = 0.0f;
            float humedad_rh = 0.0f;
            err = bme280_leer_medicion(s, &temperatura_c, &presion_hpa,
                                       &humedad_rh);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "T=%.2f C | P=%.2f hPa | H=%.2f %%",
                         temperatura_c, presion_hpa, humedad_rh);
            } else {
                ESP_LOGE(TAG, "Error leyendo la medicion: %s",
                         esp_err_to_name(err));
            }
        } else {
            ESP_LOGE(TAG, "No se pudo leer el ID del sensor: %s",
                     esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(INTERVALO_LECTURA_MS));
    }
}

esp_err_t bme280_iniciar_tarea_lectura(bme280_t *s)
{
    ESP_RETURN_ON_FALSE(s && s->manejador, ESP_ERR_INVALID_STATE, TAG,
                        "el sensor no esta inicializado");
    if (xTaskCreate(bme280_tarea_lectura, "leer_bme280", 4096, s, 5, NULL)
        != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
