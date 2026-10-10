/* pulsador.c - implementación del componente pulsador.
 *
 * Todo el acceso al pin del pulsador pasa por aquí, salvo el registro de la
 * rutina de interrupción, que hace main. Quien usa el componente habla de
 * «pulsado»; qué nivel lógico es eso, y qué resistencia de polarización le
 * corresponde, lo resuelve este fichero con el dato del montaje.
 */

#include <stdio.h>
#include "pulsador.h"
#include "esp_log.h"

static const char *TAG = "pulsador";

esp_err_t pulsador_iniciar(pulsador_t *pulsador, gpio_num_t pin, int nivel_pulsado)
{
    if (pulsador == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (pin == GPIO_NUM_NC || !GPIO_IS_VALID_GPIO(pin)) {
        ESP_LOGE(TAG, "el pin del pulsador no esta elegido o no existe");
        return ESP_ERR_INVALID_ARG;
    }

    pulsador->pin = pin;
    pulsador->nivel_pulsado = nivel_pulsado ? 1 : 0;
    pulsador->polarizado = true; //False si tengo resistencia externa de polarización, true si la polarización es interna

    /* Con el pulsador a masa, el reposo es el nivel alto y lo da el pull-up;
     * pulsar lleva el pin a nivel bajo, y la interrupción se pide en el
     * flanco de bajada. Con el pulsador a 3,3 V, todo al revés. */
    const bool a_masa = (pulsador->nivel_pulsado == 0);
    const gpio_config_t configuracion = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = a_masa ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = a_masa ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
        .intr_type = a_masa ? GPIO_INTR_NEGEDGE : GPIO_INTR_POSEDGE,
    };

    const esp_err_t error = gpio_config(&configuracion);
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "no se ha podido configurar el GPIO %d: %s",
                 (int)pin, esp_err_to_name(error));
        return error;
    }

    /* gpio_config() deja la interrupción del pin activada. Se desactiva hasta
     * que el programa le añada un manejador con gpio_isr_handler_add(), que
     * la vuelve a activar: así, mientras se lee el pin sin interrupción, un
     * pin flotante no dispara interrupciones que nadie atiende. */
    gpio_intr_disable(pin);

    ESP_LOGI(TAG, "pulsador en el GPIO %d, pulsado con nivel %d, %s interno",
             (int)pin, pulsador->nivel_pulsado, a_masa ? "pull-up" : "pull-down");
    return ESP_OK;
}

esp_err_t pulsador_polarizar(pulsador_t *pulsador, bool activa)
{
    gpio_pull_mode_t modo = GPIO_FLOATING;
    if (activa) {
        modo = (pulsador->nivel_pulsado == 0) ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY;
    }
    const esp_err_t error = gpio_set_pull_mode(pulsador->pin, modo);
    if (error == ESP_OK) {
        pulsador->polarizado = activa;
    }
    return error;
}

int pulsador_nivel(const pulsador_t *pulsador)
{
    return gpio_get_level(pulsador->pin);
}

bool pulsador_pulsado(const pulsador_t *pulsador)
{
    return gpio_get_level(pulsador->pin) == pulsador->nivel_pulsado;
}

