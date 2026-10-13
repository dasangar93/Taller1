/* pines.h - los pines del proyecto, en un solo sitio.
 *
 * Cada número de pin se escribe aquí y en ningún otro fichero de código. Los
 * componentes lo reciben como parámetro al iniciarse, y la tabla de pines del
 * README tiene que decir lo mismo que este fichero.
 */
#pragma once

/* Incluye la interfaz del controlador GPIO de ESP-IDF.
 *
 * Proporciona los tipos, constantes (como GPIO_NUM_x) y funciones necesarios
 * para identificar, configurar, leer y controlar los pines de entrada/salida del ESP32.
 * Este archivo usa sus constantes para definir los pines del proyecto.
 */
#include "driver/gpio.h"

/* Cómo seleccionar pines.
 *
 * GPIO_NUM_NC significa «sin conectar». Mientras no se seleccionen, la consola funciona,
 * pero los comandos del bus avisan de que no hay pines.
 *
 * Para elegirlo, abre el esquema de tu placa y la tabla de pines de la hoja de
 * características del ESP32-S3. Descarta los que sirven a la memoria externa,
 * los de arranque, los del USB y la consola y los que la placa ya usa. Después
 * escribe aquí su número, por ejemplo GPIO_NUM_xx, compila y graba.
 */
#define PIN_LED GPIO_NUM_4
#define PIN_PULSADOR GPIO_NUM_5

/* Nivel lógico que enciende el LED. Lo decide el montaje, no el código.
 *   1: si el LED y su resistencia van entre el pin y masa (resistencia externa pull-up)
 *   0: si van entre el pin y 3,3 V (resistencia externa pull-down)
 */
#define LED_NIVEL_ENCENDIDO 1

/* Nivel lógico que se lee con el pulsador apretado. Lo decide el montaje, no el código.
 *   0: si el pulsador va entre el pin y masa (resitencia externa pull-up)
 *   1: si va entre el pin y 3,3 V (resistencia externa pull-down)
 */
#define PULSADOR_NIVEL_PULSADO 0

/* Pines del bus I2C: SDA, la línea de datos, y SCL, la de reloj.
 *
 * Las resistencias de pull-up del bus no van en la protoboard: en el montaje del
 * taller las lleva la placa del sensor. Compruébalo en su esquema. Si la tuya
 * no las lleva, el bus necesita un par externo. El proyecto deja desactivadas
 * las internas del ESP32-S3.
 */
#define PIN_SDA GPIO_NUM_8
#define PIN_SCL GPIO_NUM_9