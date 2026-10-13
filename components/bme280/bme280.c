/* bme280.c - el sensor BME280 como dispositivo del bus I2C. Ver bme280.h. */
#include "bme280.h"

#include "esp_check.h"

static const char *TAG = "bme280";

/* Espera máxima de una transacción, en milisegundos. */
#define ESPERA_MS 100

esp_err_t bme280_iniciar(bme280_t *s, i2c_master_bus_handle_t bus,
                         uint8_t direccion, uint32_t velocidad_hz)
{
    ESP_RETURN_ON_FALSE(s && bus, ESP_ERR_INVALID_ARG, TAG, "puntero nulo");
    ESP_RETURN_ON_FALSE(s->manejador == NULL, ESP_ERR_INVALID_STATE, TAG,
                        "el sensor ya esta colgado: quitalo antes");
    s->direccion = direccion;
    s->velocidad_hz = velocidad_hz;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = direccion,
        .scl_speed_hz = velocidad_hz,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &s->manejador);
    if (err != ESP_OK)
    {
        s->manejador = NULL;
    }
    return err;
}

esp_err_t bme280_quitar(bme280_t *s)
{
    ESP_RETURN_ON_FALSE(s, ESP_ERR_INVALID_ARG, TAG, "puntero nulo");
    if (s->manejador == NULL)
    {
        return ESP_OK;
    }
    esp_err_t err = i2c_master_bus_rm_device(s->manejador);
    if (err == ESP_OK)
    {
        s->manejador = NULL;
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
