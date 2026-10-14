#include "sensor_bme280.h"

#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "bme280.h"
#include "bus_i2c.h"
#include "pines.h"

#define BME280_REG_CALIB_TP 0x88
#define BME280_REG_CALIB_H1 0xA1
#define BME280_REG_CALIB_H2 0xE1
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_STATUS 0xF3
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG 0xF5
#define BME280_REG_DATA 0xF7

#define BME280_I2C_TIMEOUT_MS 100
#define BME280_MEASUREMENT_TIMEOUT_US 100000
#define BME280_TASK_PERIOD_MS 2000

static const char *TAG = "bme280";
static bus_i2c_t s_bus;
static bme280_t s_sensor;

typedef struct
{
    uint16_t t1;
    int16_t t2;
    int16_t t3;
    uint16_t p1;
    int16_t p2;
    int16_t p3;
    int16_t p4;
    int16_t p5;
    int16_t p6;
    int16_t p7;
    int16_t p8;
    int16_t p9;
    uint8_t h1;
    int16_t h2;
    uint8_t h3;
    int16_t h4;
    int16_t h5;
    int16_t h6;
} bme280_calibracion_t;

typedef struct
{
    int32_t temperatura_centigrados;
    uint32_t presion_pa;
    uint32_t humedad_centiporcentaje;
} bme280_medicion_t;

static bme280_calibracion_t s_calibracion;

static uint16_t leer_u16_le(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[1] << 8 | bytes[0]);
}

static int16_t leer_i16_le(const uint8_t *bytes)
{
    const uint16_t valor = leer_u16_le(bytes);
    const int32_t extendido = valor >= 0x8000U ? (int32_t)valor - 0x10000 : valor;
    return (int16_t)extendido;
}

static int16_t extender_signo_12(uint16_t valor)
{
    const int32_t extendido = (valor & 0x0800U) ? (int32_t)valor - 0x1000 : valor;
    return (int16_t)extendido;
}

static int16_t extender_signo_8(uint8_t valor)
{
    const int32_t extendido = valor >= 0x80U ? (int32_t)valor - 0x100 : valor;
    return (int16_t)extendido;
}

static esp_err_t leer_registros(bme280_t *sensor, uint8_t registro,
                                uint8_t *datos, size_t longitud)
{
    return i2c_master_transmit_receive(sensor->manejador, &registro, 1, datos,
                                       longitud, BME280_I2C_TIMEOUT_MS);
}

static esp_err_t escribir_registro(bme280_t *sensor, uint8_t registro, uint8_t valor)
{
    const uint8_t datos[] = {registro, valor};
    return i2c_master_transmit(sensor->manejador, datos, sizeof(datos),
                               BME280_I2C_TIMEOUT_MS);
}

static esp_err_t cargar_calibracion(bme280_t *sensor)
{
    uint8_t calib_tp[24];
    uint8_t calib_h1;
    uint8_t calib_h[7];

    esp_err_t err = leer_registros(sensor, BME280_REG_CALIB_TP, calib_tp, sizeof(calib_tp));
    if (err != ESP_OK)
    {
        return err;
    }
    err = leer_registros(sensor, BME280_REG_CALIB_H1, &calib_h1, sizeof(calib_h1));
    if (err != ESP_OK)
    {
        return err;
    }
    err = leer_registros(sensor, BME280_REG_CALIB_H2, calib_h, sizeof(calib_h));
    if (err != ESP_OK)
    {
        return err;
    }

    s_calibracion.t1 = leer_u16_le(&calib_tp[0]);
    s_calibracion.t2 = leer_i16_le(&calib_tp[2]);
    s_calibracion.t3 = leer_i16_le(&calib_tp[4]);
    s_calibracion.p1 = leer_u16_le(&calib_tp[6]);
    s_calibracion.p2 = leer_i16_le(&calib_tp[8]);
    s_calibracion.p3 = leer_i16_le(&calib_tp[10]);
    s_calibracion.p4 = leer_i16_le(&calib_tp[12]);
    s_calibracion.p5 = leer_i16_le(&calib_tp[14]);
    s_calibracion.p6 = leer_i16_le(&calib_tp[16]);
    s_calibracion.p7 = leer_i16_le(&calib_tp[18]);
    s_calibracion.p8 = leer_i16_le(&calib_tp[20]);
    s_calibracion.p9 = leer_i16_le(&calib_tp[22]);

    s_calibracion.h1 = calib_h1;
    s_calibracion.h2 = leer_i16_le(&calib_h[0]);
    s_calibracion.h3 = calib_h[2];
    s_calibracion.h4 = extender_signo_12(
        (uint16_t)((uint16_t)calib_h[3] << 4 | (calib_h[4] & 0x0FU)));
    s_calibracion.h5 = extender_signo_12(
        (uint16_t)((uint16_t)calib_h[5] << 4 | (calib_h[4] >> 4)));
    s_calibracion.h6 = extender_signo_8(calib_h[6]);

    if (s_calibracion.p1 == 0)
    {
        ESP_LOGE(TAG, "Calibracion invalida: dig_P1 es cero");
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

static esp_err_t configurar_medicion(bme280_t *sensor)
{
    esp_err_t err = escribir_registro(sensor, BME280_REG_CTRL_MEAS, 0x00);
    if (err != ESP_OK)
    {
        return err;
    }

    err = escribir_registro(sensor, BME280_REG_CONFIG, 0x00);
    if (err != ESP_OK)
    {
        return err;
    }

    err = escribir_registro(sensor, BME280_REG_CTRL_HUM, 0x01);
    if (err != ESP_OK)
    {
        return err;
    }

    /* Temperatura x1, presión x1 y modo sleep hasta iniciar cada muestra. */
    return escribir_registro(sensor, BME280_REG_CTRL_MEAS, 0x24);
}

static esp_err_t inicializar_medicion(bme280_t *sensor)
{
    uint8_t id = 0;
    esp_err_t err = bme280_leer_id(sensor, &id);
    if (err != ESP_OK)
    {
        return err;
    }
    if (id != BME280_ID)
    {
        ESP_LOGE(TAG, "ID inesperado: 0x%02X (esperado 0x%02X)", id, BME280_ID);
        return ESP_ERR_INVALID_RESPONSE;
    }
    ESP_LOGI(TAG, "BME280 detectado: direccion=0x%02X, ID=0x%02X",
             sensor->direccion, id);

    err = cargar_calibracion(sensor);
    if (err != ESP_OK)
    {
        return err;
    }

    return configurar_medicion(sensor);
}

static esp_err_t esperar_medicion(bme280_t *sensor)
{
    const int64_t inicio_us = esp_timer_get_time();

    for (;;)
    {
        uint8_t estado = 0;
        esp_err_t err = leer_registros(sensor, BME280_REG_STATUS, &estado, sizeof(estado));
        if (err != ESP_OK)
        {
            return err;
        }
        if ((estado & 0x08U) == 0)
        {
            return ESP_OK;
        }
        if (esp_timer_get_time() - inicio_us >= BME280_MEASUREMENT_TIMEOUT_US)
        {
            return ESP_ERR_TIMEOUT;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static esp_err_t leer_datos_brutos(bme280_t *sensor, uint32_t *presion,
                                   uint32_t *temperatura, uint16_t *humedad)
{
    uint8_t datos[8];
    esp_err_t err = escribir_registro(sensor, BME280_REG_CTRL_MEAS, 0x25);
    if (err != ESP_OK)
    {
        return err;
    }

    err = esperar_medicion(sensor);
    if (err != ESP_OK)
    {
        return err;
    }

    err = leer_registros(sensor, BME280_REG_DATA, datos, sizeof(datos));
    if (err != ESP_OK)
    {
        return err;
    }

    *presion = ((uint32_t)datos[0] << 12) | ((uint32_t)datos[1] << 4) | (datos[2] >> 4);
    *temperatura =
        ((uint32_t)datos[3] << 12) | ((uint32_t)datos[4] << 4) | (datos[5] >> 4);
    *humedad = (uint16_t)((uint16_t)datos[6] << 8 | datos[7]);
    return ESP_OK;
}

static int32_t compensar_temperatura(uint32_t adc_temperatura, int64_t *t_fine)
{
    const int64_t var1 =
        ((((int64_t)adc_temperatura >> 3) - ((int64_t)s_calibracion.t1 << 1)) *
         s_calibracion.t2) >>
        11;
    const int64_t delta = ((int64_t)adc_temperatura >> 4) - s_calibracion.t1;
    const int64_t var2 = (((delta * delta) >> 12) * s_calibracion.t3) >> 14;

    *t_fine = var1 + var2;
    return (int32_t)((*t_fine * 5 + 128) >> 8);
}

static uint32_t compensar_presion(uint32_t adc_presion, int64_t t_fine)
{
    int64_t var1 = t_fine - 128000;
    int64_t var2 = var1 * var1 * s_calibracion.p6;
    var2 += (var1 * s_calibracion.p5) << 17;
    var2 += (int64_t)s_calibracion.p4 << 35;
    var1 = ((var1 * var1 * s_calibracion.p3) >> 8) +
           ((var1 * s_calibracion.p2) << 12);
    var1 = (((((int64_t)1 << 47) + var1)) * s_calibracion.p1) >> 33;

    if (var1 == 0)
    {
        return 0;
    }

    int64_t pressure = ((((int64_t)1048576 - adc_presion) << 31) - var2) * 3125;
    pressure /= var1;
    var1 = ((int64_t)s_calibracion.p9 * (pressure >> 13) * (pressure >> 13)) >> 25;
    var2 = ((int64_t)s_calibracion.p8 * pressure) >> 19;
    pressure = ((pressure + var1 + var2) >> 8) + ((int64_t)s_calibracion.p7 << 4);

    return (uint32_t)((pressure + 128) >> 8);
}

static uint32_t compensar_humedad(uint16_t adc_humedad, int64_t t_fine)
{
    int64_t humedad = t_fine - 76800;
    humedad = (((((int64_t)adc_humedad << 14) -
                 ((int64_t)s_calibracion.h4 << 20) -
                 ((int64_t)s_calibracion.h5 * humedad)) +
                16384) >>
               15) *
              (((((((humedad * s_calibracion.h6) >> 10) *
                   (((humedad * s_calibracion.h3) >> 11) + 32768)) >>
                  10) +
                 2097152) *
                    s_calibracion.h2 +
                8192) >>
               14);
    humedad -= (((((humedad >> 15) * (humedad >> 15)) >> 7) *
                 s_calibracion.h1) >>
                4);

    if (humedad < 0)
    {
        humedad = 0;
    }
    else if (humedad > 419430400)
    {
        humedad = 419430400;
    }

    /* El cálculo Bosch queda en Q22.10 tras quitar los 12 bits fraccionarios. */
    const uint32_t humedad_q22_10 = (uint32_t)(humedad >> 12);
    return (humedad_q22_10 * 100U + 512U) >> 10;
}

static esp_err_t leer_medicion(bme280_t *sensor, bme280_medicion_t *medicion)
{
    uint32_t adc_presion;
    uint32_t adc_temperatura;
    uint16_t adc_humedad;
    esp_err_t err =
        leer_datos_brutos(sensor, &adc_presion, &adc_temperatura, &adc_humedad);
    if (err != ESP_OK)
    {
        return err;
    }

    int64_t t_fine;
    medicion->temperatura_centigrados =
        compensar_temperatura(adc_temperatura, &t_fine);
    medicion->presion_pa = compensar_presion(adc_presion, t_fine);
    medicion->humedad_centiporcentaje = compensar_humedad(adc_humedad, t_fine);
    return ESP_OK;
}

static void tarea_sensor(void *argumento)
{
    bme280_t *sensor = argumento;
    TickType_t referencia = xTaskGetTickCount();

    for (;;)
    {
        bme280_medicion_t medicion;
        esp_err_t err = leer_medicion(sensor, &medicion);
        if (err == ESP_OK)
        {
            const bool temperatura_negativa = medicion.temperatura_centigrados < 0;
            const uint32_t temperatura_absoluta =
                temperatura_negativa
                    ? (uint32_t)(-(int64_t)medicion.temperatura_centigrados)
                    : (uint32_t)medicion.temperatura_centigrados;
            ESP_LOGI(TAG,
                     "BME280: T=%s%" PRIu32 ".%02" PRIu32
                     " C, P=%" PRIu32 ".%02" PRIu32
                     " hPa, HR=%" PRIu32 ".%02" PRIu32 "%%",
                     temperatura_negativa ? "-" : "",
                     temperatura_absoluta / 100, temperatura_absoluta % 100,
                     medicion.presion_pa / 100, medicion.presion_pa % 100,
                     medicion.humedad_centiporcentaje / 100,
                     medicion.humedad_centiporcentaje % 100);
        }
        else
        {
            ESP_LOGE(TAG, "Fallo al leer el sensor: %s", esp_err_to_name(err));
        }

        xTaskDelayUntil(&referencia, pdMS_TO_TICKS(BME280_TASK_PERIOD_MS));
    }
}

esp_err_t sensor_bme280_iniciar(void)
{
    esp_err_t err = bus_i2c_iniciar(&s_bus, PIN_SDA, PIN_SCL, false);
    if (err != ESP_OK)
    {
        return err;
    }

    err = bme280_iniciar(&s_sensor, s_bus.manejador, 0x76, 100000);
    if (err != ESP_OK)
    {
        return err;
    }

    err = inicializar_medicion(&s_sensor);
    if (err != ESP_OK)
    {
        return err;
    }

    if (xTaskCreate(tarea_sensor, "sensor", 4096, &s_sensor, 5, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "No hay memoria suficiente para crear la tarea del sensor");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
