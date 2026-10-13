# Taller1 — Sistemas Empotrados y Tiempo Real

Proyecto para una placa ESP32-S3 que permite probar un LED, un pulsador y un
sensor BME280. La aplicación está desarrollada con ESP-IDF y FREERTOS.

## Procedencia de los componentes

Los cuatro componentes reutilizados en este proyecto proceden de los códigos
proporcionados por el profesor de la asignatura **Sistemas Empotrados y Tiempo
Real**. Se incluyen como componentes ESP-IDF separados para que la aplicación
principal pueda utilizar cada funcionalidad de forma independiente. Por tanto,
estos componentes no se presentan como implementaciones originales de este
proyecto.

| Componente | Código | Función | Pin |
| --- | --- | --- | --- |
| LED | [`components/led/`](components/led/) | Configura un GPIO como salida y permite encender, apagar o conmutar el LED. | 4 |
| Pulsador | [`components/pulsador/`](components/pulsador/) | Configura un GPIO como entrada y permite consultar su nivel y estado. | 5 |
| Bus I2C | [`components/I2C/`](components/I2C/) | Inicializa y libera el bus I2C, y permite explorar las direcciones de dispositivos. | SCL9 |
| Sensor BME280 | [`components/BME280/`](components/BME280/) | Se comunica con el BME280 por I2C y obtiene temperatura, presión y humedad. | SDA 8 |

La aplicación de [`main/main.c`](main/main.c) integra estos componentes y
gestiona la consola UART, las tareas de FreeRTOS y la interrupción del
pulsador. La asignación de pines del proyecto está en
[`main/pines.h`](main/pines.h).

## Funcionamiento

La consola recibe comandos por UART0. Al iniciar, muestra la ayuda; se puede
escribir:

| Tecla | Acción |
| --- | --- |
| `q` | Inicia el parpadeo del LED. |
| `w` | Activa la lectura del pulsador mediante interrupción. |
| `s` | Inicia el bus I2C e intenta comenzar la lectura periódica del BME280. |

El LED cambia de estado cada 500 ms. El BME280 se lee cada dos segundos.
Los periféricos y el sensor son opcionales, pero para que cada función opere
correctamente deben estar conectados a los pines indicados en
[`main/pines.h`](main/pines.h), y el BME280 debe responder en la dirección I2C
configurada (`0x76`).

## Compilación y uso

1. Configura el entorno de ESP-IDF para tu sistema.
2. Revisa y ajusta los pines de [`main/pines.h`](main/pines.h) para que
   coincidan con tu placa y montaje.
3. Desde la raíz del proyecto, compila, graba y abre el monitor serie:

   ```text
   idf.py build
   idf.py flash monitor
   ```

4. Abre la consola UART0 y envía `q`, `w` o `s`.

La compilación requiere una versión de ESP-IDF compatible con el controlador
I2C utilizado por el proyecto.
