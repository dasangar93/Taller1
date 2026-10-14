#include "tarea_consola.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"

#include "consola.h"
#include "sensor_bme280.h"
#include "tarea_led.h"
#include "tarea_pulsador.h"

#define LARGO_MAXIMO_LINEA 64

static const char *TAG = "tarea-consola";
static bool s_trazas_led = true;
static bool s_trazas_pulsador = true;
static bool s_trazas_sensor = true;

static void comando_ayuda(const char *argumentos)
{
    (void)argumentos;
    consola_ayuda();
}

static void comando_estado(const char *argumentos)
{
    (void)argumentos;

    printf("LED: %s\n", tarea_led_encendido() ? "encendido" : "apagado");
    printf("Pulsador: nivel logico %d\n", tarea_pulsador_nivel());

    sensor_bme280_estado_t estado;
    const esp_err_t err = sensor_bme280_estado(&estado);
    if (err != ESP_OK)
    {
        printf("Sensor BME280: estado no disponible (%s)\n", esp_err_to_name(err));
        return;
    }

    printf("BME280: direccion=0x%02X, ID=0x%02X\n",
           estado.direccion, estado.id);
    if (!estado.medicion_disponible)
    {
        printf("BME280: aun no hay una medida disponible\n");
        return;
    }

    const bool temperatura_negativa = estado.temperatura_centigrados < 0;
    const uint32_t temperatura_absoluta =
        temperatura_negativa
            ? (uint32_t)(-(int64_t)estado.temperatura_centigrados)
            : (uint32_t)estado.temperatura_centigrados;
    printf("Ultima medida: T=%s%" PRIu32 ".%02" PRIu32
           " C, P=%" PRIu32 ".%02" PRIu32
           " hPa, HR=%" PRIu32 ".%02" PRIu32 "%%\n",
           temperatura_negativa ? "-" : "",
           temperatura_absoluta / 100, temperatura_absoluta % 100,
           estado.presion_pa / 100, estado.presion_pa % 100,
           estado.humedad_centiporcentaje / 100,
           estado.humedad_centiporcentaje % 100);
}

static void comando_led(const char *argumentos)
{
    tarea_led_accion_t accion;
    if (strcmp(argumentos, "on") == 0)
    {
        accion = TAREA_LED_ENCENDER;
    }
    else if (strcmp(argumentos, "off") == 0)
    {
        accion = TAREA_LED_APAGAR;
    }
    else if (strcmp(argumentos, "toggle") == 0)
    {
        accion = TAREA_LED_CONMUTAR;
    }
    else if (strcmp(argumentos, "auto") == 0)
    {
        accion = TAREA_LED_AUTOMATICO;
    }
    else
    {
        printf("Uso: led <on|off|toggle|auto>\n");
        return;
    }

    const esp_err_t err = tarea_led_controlar(accion);
    if (err != ESP_OK)
    {
        printf("No se pudo enviar la orden al LED: %s\n", esp_err_to_name(err));
        return;
    }

    printf("Orden enviada al LED: %s\n", argumentos);
}

static void comando_trazas(const char *argumentos)
{
    char nombre[16];
    char accion[8];
    char extra;
    if (sscanf(argumentos, "%15s %7s %c", nombre, accion, &extra) != 2)
    {
        printf("Uso: trazas <led|pulsador|sensor> <on|off>\n");
        return;
    }
    if (strcmp(accion, "on") != 0 && strcmp(accion, "off") != 0)
    {
        printf("La accion de trazas debe ser on u off\n");
        return;
    }

    const bool mostrar = strcmp(accion, "on") == 0;
    if (strcmp(nombre, "led") == 0)
    {
        tarea_led_mostrar_trazas(mostrar);
        s_trazas_led = mostrar;
    }
    else if (strcmp(nombre, "pulsador") == 0)
    {
        tarea_pulsador_mostrar_trazas(mostrar);
        s_trazas_pulsador = mostrar;
    }
    else if (strcmp(nombre, "sensor") == 0)
    {
        sensor_bme280_mostrar_trazas(mostrar);
        s_trazas_sensor = mostrar;
    }
    else
    {
        printf("Tarea desconocida: %s\n", nombre);
        return;
    }

    printf("Trazas de %s: %s\n", nombre, mostrar ? "visibles" : "ocultas");
}

static const consola_comando_t s_comandos[] = {
    {"help", NULL, "Muestra esta lista de comandos", comando_ayuda},
    {"estado", NULL, "Muestra el LED, el pulsador y la ultima medida del sensor",
     comando_estado},
    {"led", "on|off|toggle|auto", "Controla el LED o reanuda el parpadeo automatico",
     comando_led},
    {"trazas", "led|pulsador|sensor on|off", "Muestra u oculta las trazas de una tarea",
     comando_trazas},
};

esp_err_t tarea_consola_iniciar(void)
{
    const consola_config_t configuracion = {
        .puerto = UART_NUM_0,
        .largo_maximo = LARGO_MAXIMO_LINEA,
        .tabla = s_comandos,
        .n_comandos = sizeof(s_comandos) / sizeof(s_comandos[0]),
    };

    ESP_LOGI(TAG, "Iniciando consola serie en UART0");
    ESP_LOGI(TAG, "Trazas activas: LED=%s, pulsador=%s, sensor=%s",
             s_trazas_led ? "si" : "no",
             s_trazas_pulsador ? "si" : "no",
             s_trazas_sensor ? "si" : "no");
    return consola_iniciar(&configuracion);
}
