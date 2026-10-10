/* pines.h - los pines del proyecto, en un solo sitio.
 *
 * Cada número de pin se escribe aquí y en ningún otro fichero de código. Los
 * componentes lo reciben como parámetro al iniciarse, y la tabla de pines del
 * README tiene que decir lo mismo que este fichero.
 */
#pragma once

#include "driver/gpio.h"

/* Pin del LED externo.
 *
 * GPIO_NUM_NC significa «sin conectar»: el repositorio llega así a propósito,
 * porque el pin lo eliges tú. Mientras no lo cambies, el programa hace las
 * mismas medidas sin mover ningún pin: lo que se mide son los instantes, y
 * salen en las trazas.
 *
 * Para elegirlo, abre el esquema de tu placa y la tabla de pines de la hoja de
 * características del ESP32-S3, y descarta los que sirven a la memoria externa,
 * los de arranque, los del USB y la consola y los que la placa ya usa. Después
 * escribe aquí su número, por ejemplo GPIO_NUM_xx, compila y graba.
 */
#define PIN_LED              GPIO_NUM_4
#define PIN_PULSADOR         GPIO_NUM_5

/* Nivel lógico que enciende el LED. Lo decide el montaje, no el código:
 *   1 si el LED y su resistencia van entre el pin y masa;
 *   0 si van entre el pin y 3,3 V. */
#define LED_NIVEL_ENCENDIDO  1

/* Nivel lógico que se lee con el pulsador apretado. Lo decide el montaje:
 *   0 si el pulsador va entre el pin y masa, con pull-up: es el de la sesión;
 *   1 si va entre el pin y 3,3 V, con pull-down. */
#define PULSADOR_NIVEL_PULSADO  0

/* Pines del bus I2C: SDA, la línea de datos, y SCL, la de reloj.
 *
 * GPIO_NUM_NC significa «sin conectar»: el repositorio llega así a propósito,
 * porque los pines los eliges tú. Mientras no los cambies, la consola funciona,
 * pero los comandos del bus avisan de que no hay pines.
 *
 * Para elegirlos, abre el esquema de tu placa y la tabla de pines de la hoja de
 * características del ESP32-S3, y descarta los que sirven a la memoria externa,
 * los de arranque, los del USB y la consola y los que la placa ya usa. Después
 * escribe aquí sus números, por ejemplo GPIO_NUM_xx, compila y graba.
 *
 * Las resistencias de pull-up del bus no van en la protoboard: en el montaje del
 * taller las lleva la placa del sensor. Compruébalo en su esquema. Si la tuya
 * no las lleva, el bus necesita un par externo. El proyecto deja desactivadas
 * las internas del ESP32-S3.
 */
#define PIN_SDA     GPIO_NUM_8
#define PIN_SCL     GPIO_NUM_9