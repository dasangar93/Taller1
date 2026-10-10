/* bus_i2c.c - el bus I2C en modo maestro y su exploración. Ver i2c.h. */
#include <stdio.h>
#include "I2C.h"
#include <string.h>
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "I2C";

/* Etiqueta con la que traza el controlador I2C de ESP-IDF. */
#define TAG_CONTROLADOR "i2c.master"

/* Espera máxima de cada prueba, en milisegundos. */
#define ESPERA_PRUEBA_MS 50

/* Tiempos agotados seguidos tras los que se deja de explorar. */
#define TIEMPOS_AGOTADOS_SEGUIDOS 4

esp_err_t bus_i2c_iniciar(bus_i2c_t *b, gpio_num_t PIN_SDA, gpio_num_t PIN_SCL, bool cruzado)
{
    ESP_RETURN_ON_FALSE(b, ESP_ERR_INVALID_ARG, TAG, "puntero nulo");
    ESP_RETURN_ON_FALSE(GPIO_IS_VALID_OUTPUT_GPIO(PIN_SDA) && GPIO_IS_VALID_OUTPUT_GPIO(PIN_SCL),
                        ESP_ERR_INVALID_ARG, TAG, "pines no validos");
    ESP_RETURN_ON_FALSE(b->manejador == NULL, ESP_ERR_INVALID_STATE, TAG,
                        "el bus ya existe: eliminalo antes");

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = cruzado ? PIN_SCL : PIN_SDA,
        .scl_io_num = cruzado ? PIN_SDA : PIN_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &b->manejador);
    if (err == ESP_OK) {
        b->sda = PIN_SDA;
        b->scl = PIN_SCL;
        b->cruzado = cruzado;
    } else {
        b->manejador = NULL;
    }
    return err;
}

esp_err_t bus_i2c_eliminar(bus_i2c_t *b)
{
    ESP_RETURN_ON_FALSE(b, ESP_ERR_INVALID_ARG, TAG, "puntero nulo");
    if (b->manejador == NULL) {
        return ESP_OK;
    }
    esp_err_t err = i2c_del_master_bus(b->manejador);
    if (err == ESP_OK) {
        b->manejador = NULL;
    }
    return err;
}

void bus_i2c_explorar(bus_i2c_t *b, bus_i2c_exploracion_t *r)
{
    memset(r, 0, sizeof *r);
    if (b == NULL || b->manejador == NULL) {
        r->n_otros = 1;
        r->otro = ESP_ERR_INVALID_STATE;
        return;
    }

    /* El controlador traza un error por cada prueba que agota el tiempo. La
     * exploración cuenta esos errores ella misma y los resume al final, así
     * que mientras dura se callan las trazas del controlador. */
    esp_log_level_t antes = esp_log_level_get(TAG_CONTROLADOR);
    esp_log_level_set(TAG_CONTROLADOR, ESP_LOG_NONE);

    int agotados_seguidos = 0;
    for (uint16_t dir = 0x08; dir <= 0x77; dir++) {
        esp_err_t err = i2c_master_probe(b->manejador, dir, ESPERA_PRUEBA_MS);
        r->exploradas++;
        if (err == ESP_OK) {
            r->encontradas[r->n_encontradas++] = (uint8_t)dir;
            agotados_seguidos = 0;
        } else if (err == ESP_ERR_NOT_FOUND) {
            r->n_sin_respuesta++;
            agotados_seguidos = 0;
        } else if (err == ESP_ERR_TIMEOUT) {
            r->n_tiempo_agotado++;
            if (++agotados_seguidos >= TIEMPOS_AGOTADOS_SEGUIDOS) {
                r->interrumpida = true;
                break;
            }
        } else {
            r->n_otros++;
            r->otro = err;
            agotados_seguidos = 0;
        }
    }

    esp_log_level_set(TAG_CONTROLADOR, antes);
}
