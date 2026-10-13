/* consola.c - una consola de comandos sobre la UART del monitor. Ver consola.h. */
#include "consola.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "driver/uart.h"
#include "driver/uart_vfs.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "linea.h"

static const char *TAG = "consola";

/* La velocidad de la UART del monitor, la que fija la configuración de
 * ESP-IDF: si no se reconfigura igual, el monitor deja de entenderla. */
#ifdef CONFIG_ESP_CONSOLE_UART_BAUDRATE
#define BAUDIOS_MONITOR CONFIG_ESP_CONSOLE_UART_BAUDRATE
#else
#define BAUDIOS_MONITOR 115200
#endif

static consola_config_t cfg;
static linea_t linea;
static char *buf_linea;     /* la línea que se está acumulando        */
static char *buf_visible;   /* la misma línea, con lo invisible escrito */

/* ---- 4. Responder -------------------------------------------------------- */

static void prompt(void)
{
    printf("> ");
    fflush(stdout);
}

/* Escribe la línea de modo que se vea lo que no se ve: un retorno de carro
 * como \r, un tabulador como \t, otros caracteres de control como \xNN. Los
 * espacios ya se ven porque la línea va entre comillas. */
static const char *visible(const char *s)
{
    char *d = buf_visible;
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '\r') {
            *d++ = '\\'; *d++ = 'r';
        } else if (c == '\n') {
            *d++ = '\\'; *d++ = 'n';
        } else if (c == '\t') {
            *d++ = '\\'; *d++ = 't';
        } else if (c < 0x20 || c == 0x7f) {
            d += sprintf(d, "\\x%02X", c);
        } else {
            *d++ = (char)c;
        }
    }
    *d = '\0';
    return buf_visible;
}

void consola_ayuda(void)
{
    printf("Comandos:\n");
    for (size_t i = 0; i < cfg.n_comandos; i++) {
        const consola_comando_t *c = &cfg.tabla[i];
        char uso[48];
        snprintf(uso, sizeof uso, "%s%s%s", c->nombre,
                 c->argumentos ? " " : "", c->argumentos ? c->argumentos : "");
        printf("  %-18s %s\n", uso, c->ayuda);
    }
}

void consola_normalizar(bool activar)
{
    linea.normalizar = activar;
}

bool consola_normaliza(void)
{
    return linea.normalizar;
}

/* ---- 3. Analizar ---------------------------------------------------------- */

static const consola_comando_t *buscar(const char *nombre)
{
    for (size_t i = 0; i < cfg.n_comandos; i++) {
        if (strcmp(cfg.tabla[i].nombre, nombre) == 0) {
            return &cfg.tabla[i];
        }
    }
    return NULL;
}

static void analizar_y_responder(char *l)
{
    const consola_comando_t *c = NULL;
    const char *args = "";

    if (linea.normalizar) {
        /* El nombre llega hasta el primer espacio. Los argumentos empiezan
         * después de los espacios que sigan. La línea ya viene recortada. */
        char *p = l;
        while (*p && *p != ' ' && *p != '\t') {
            p++;
        }
        if (*p) {
            *p++ = '\0';
            while (*p == ' ' || *p == '\t') {
                p++;
            }
            args = p;
        }
        c = buscar(l);
    } else {
        /* La consola «ingenua»: compara la línea tal como ha llegado. Un
         * espacio de más al final o un carácter de control ya no coinciden. */
        for (size_t i = 0; i < cfg.n_comandos && !c; i++) {
            const consola_comando_t *t = &cfg.tabla[i];
            size_t n = strlen(t->nombre);
            if (strcmp(l, t->nombre) == 0) {
                c = t;
            } else if (t->argumentos && strncmp(l, t->nombre, n) == 0 && l[n] == ' ') {
                c = t;
                args = l + n + 1;
            }
        }
    }

    /* 4. Responder, siempre. */
    if (c == NULL) {
        printf("No conozco \"%s\". Escribe help para ver los comandos.\n", visible(l));
    } else if (c->argumentos == NULL && *args != '\0') {
        printf("%s no admite argumentos.\n", c->nombre);
    } else {
        c->ejecutar(args);
    }
}

/* ---- 1. Leer, y el eco de lo que se teclea --------------------------------- */

static void eco(char c, linea_estado_t e)
{
    if ((c == '\r' || c == '\n') && e != LINEA_SIGUE) {
        uart_write_bytes(cfg.puerto, "\r\n", 2);
    } else if (e == LINEA_BORRADO) {
        uart_write_bytes(cfg.puerto, "\b \b", 3);
    } else if (((unsigned char)c >= 0x20 && (unsigned char)c != 0x7f) || c == '\t') {
        uart_write_bytes(cfg.puerto, &c, 1);
    }
}

static void tarea_consola(void *arg)
{
    (void)arg;
    prompt();
    for (;;) {
        uint8_t b;
        /* Los caracteres llegan sueltos: se leen de uno en uno, del búfer
         * en el que el controlador guarda lo recibido. */
        int n = uart_read_bytes(cfg.puerto, &b, 1, pdMS_TO_TICKS(100));
        if (n != 1) {
            continue;
        }
        /* 2. Acumular y normalizar */
        linea_estado_t e = linea_recibir(&linea, (char)b);
        eco((char)b, e);
        switch (e) {
        case LINEA_COMPLETA:
            analizar_y_responder(linea.buf);
            prompt();
            break;
        case LINEA_VACIA:
            prompt();
            break;
        case LINEA_DESCARTADA:
            printf("Linea demasiado larga: caben %u caracteres. Se descarta entera.\n",
                   (unsigned)(cfg.largo_maximo));
            prompt();
            break;
        default:
            break;
        }
    }
}

esp_err_t consola_iniciar(const consola_config_t *config)
{
    ESP_RETURN_ON_FALSE(config && config->tabla && config->n_comandos > 0 &&
                        config->largo_maximo >= 8, ESP_ERR_INVALID_ARG, TAG,
                        "configuracion no valida");
    cfg = *config;

    buf_linea = malloc(cfg.largo_maximo + 1);
    buf_visible = malloc(4 * cfg.largo_maximo + 1);
    ESP_RETURN_ON_FALSE(buf_linea && buf_visible, ESP_ERR_NO_MEM, TAG, "sin memoria");
    linea_iniciar(&linea, buf_linea, cfg.largo_maximo + 1, true);

    /* Lo que ya se ha escrito tiene que salir antes de reconfigurar la UART
     * que lo transmite. ESP-IDF hace lo mismo en su propia consola. */
    fflush(stdout);
    fsync(fileno(stdout));

    /* El orden de la documentación de la UART: instalar el controlador con sus
     * búferes, aplicar la configuración y asignar los pines. */
    ESP_RETURN_ON_ERROR(uart_driver_install(cfg.puerto, 256, 0, 0, NULL, 0), TAG,
                        "no se pudo instalar el controlador de la UART");
    uart_config_t uc = {
        .baud_rate = BAUDIOS_MONITOR,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_RETURN_ON_ERROR(uart_param_config(cfg.puerto, &uc), TAG, "uart_param_config");
    /* La UART del monitor ya tiene sus pines: no se cambian. */
    ESP_RETURN_ON_ERROR(uart_set_pin(cfg.puerto, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE),
                        TAG, "uart_set_pin");
    /* printf y las trazas pasan a escribir a través del controlador, el mismo
     * camino que el eco, en lugar de escribir directamente en el registro. */
    uart_vfs_dev_use_driver(cfg.puerto);

    BaseType_t ok = xTaskCreate(tarea_consola, "consola", 4096, NULL, 5, NULL);
    ESP_RETURN_ON_FALSE(ok == pdPASS, ESP_ERR_NO_MEM, TAG, "no se pudo crear la tarea");
    return ESP_OK;
}
