/* led.c - implementación del componente led.
 *
 * Todo el acceso al pin del LED pasa por aquí. Quien usa el componente habla
 * de «encender» y «apagar»; qué nivel lógico es cada cosa lo resuelve este
 * fichero con el dato del montaje que recibió al iniciarse.
 */

#include "led.h"

#include "esp_log.h"

static const char *TAG = "led";

/* Escribe en el pin el nivel que corresponde al estado pedido. */
static void escribir(led_t *led, bool encender)
{
    led->encendido = encender;
    if (led->pin == GPIO_NUM_NC) {
        return;
    }
    const int nivel = encender ? led->nivel_encendido : !led->nivel_encendido;
    gpio_set_level(led->pin, nivel);
}

esp_err_t led_iniciar(led_t *led, gpio_num_t pin, int nivel_encendido)
{
    if (led == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    led->pin = pin;
    led->nivel_encendido = nivel_encendido ? 1 : 0;
    led->encendido = false;

    if (pin == GPIO_NUM_NC) {
        ESP_LOGW(TAG, "sin pin asignado: no se mueve ningun pin, solo se cuenta el estado");
        return ESP_OK;
    }

    if (!GPIO_IS_VALID_OUTPUT_GPIO(pin)) {
        ESP_LOGE(TAG, "el GPIO %d no existe o no puede ser salida", (int)pin);
        return ESP_ERR_INVALID_ARG;
    }

    /* Configurar no fija el nivel. Se escribe primero el estado seguro, LED
     * apagado, y después se configura el pin como salida: gpio_config() no
     * cambia el nivel escrito, de modo que el pin empieza a actuar como
     * salida ya con el nivel correcto y el LED no llega a encenderse al
     * arrancar. */
    escribir(led, false);

    /* pin_bit_mask es una máscara de bits, no un número de pin: un bit por
     * pin. Es de 64 bits porque el ESP32-S3 tiene pines hasta el 48, y por
     * eso el 1 se escribe 1ULL. Con un 1 de tipo int, de 32 bits y con
     * signo, desplazarlo 31 posiciones o más no está definido en C. */
    const gpio_config_t configuracion = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    const esp_err_t error = gpio_config(&configuracion);
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "no se ha podido configurar el GPIO %d: %s",
                 (int)pin, esp_err_to_name(error));
        return error;
    }

    ESP_LOGI(TAG, "LED en el GPIO %d, se enciende con nivel %d",
             (int)pin, led->nivel_encendido);
    return ESP_OK;
}

void led_encender(led_t *led)
{
    escribir(led, true);
}

void led_apagar(led_t *led)
{
    escribir(led, false);
}

void led_conmutar(led_t *led)
{
    escribir(led, !led->encendido);
}
