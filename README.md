# Taller 1 — Sistemas Empotrados y Tiempo Real

Aplicación para ESP32-S3 desarrollada con ESP-IDF y FreeRTOS. Integra un LED,
un pulsador con interrupción y antirrebote, un sensor BME280 conectado por I2C
y una consola de comandos por UART0.

## Hardware y conexiones

| Dispositivo | Señal | GPIO |
| --- | --- | ---: |
| LED | Salida | 4 |
| Pulsador | Entrada con interrupción | 5 |
| BME280 | SDA | 8 |
| BME280 | SCL | 9 |

La configuración está centralizada en [`main/pines.h`](main/pines.h). Comprueba
el esquema de tu placa antes de cambiar los GPIO. El nivel activo del LED y del
pulsador también se define allí según cómo estén montados.

El BME280 debe responder en la dirección I2C `0x76` (SDO a masa) y tener el ID
`0x60`. El bus funciona a 100 kHz y no activa los pull-ups internos; comprueba
que el montaje incluya resistencias pull-up externas. El programa inicializa
los periféricos al arrancar y detiene la aplicación si falla la inicialización
del sensor.

## Procedencia de los componentes

Los componentes `led`, `pulsador`, `bus_i2c` y `bme280` reutilizan código
proporcionado por el profesor de **Sistemas Empotrados y Tiempo Real**. Se
mantienen separados como componentes ESP-IDF para que cada funcionalidad pueda
utilizarse de forma independiente; no se presentan como implementaciones
originales de este proyecto. La aplicación de `main/` integra esos componentes
y realiza la adquisición y compensación de las medidas del sensor. La consola
se incluye como componente independiente del proyecto.

## Funcionamiento del código

### Arranque y tareas

El punto de entrada es `app_main()` en [`main/main.c`](main/main.c). Inicializa
en este orden la tarea del LED, la atención del pulsador, el sensor y la
consola. Cada inicializador devuelve un `esp_err_t`; `ESP_ERROR_CHECK` detiene
la aplicación si alguno falla, por lo que la consola solo arranca si los
periféricos anteriores se han inicializado correctamente.

La lógica de la aplicación está dividida en tareas de FreeRTOS:

- **LED (`parpadeo`):** es la única tarea que escribe al LED. Por defecto lo
  enciende durante 500 ms y lo apaga durante 500 ms. Las órdenes de la consola
  se envían mediante una cola de cuatro acciones; `on`, `off` y `toggle`
  interrumpen el modo automático, mientras que `auto` reanuda el parpadeo. Si
  la cola está llena, la orden no se acepta y la consola muestra el error.
- **Pulsador (`entrada`):** su interrupción no imprime ni procesa el evento:
  guarda la marca de tiempo en una cola de FreeRTOS. La tarea descarta los
  eventos que llegan dentro de la ventana antirrebote desde la última pulsación
  aceptada y registra los demás. La longitud de esta cola se configura en
  `menuconfig`; si se llena, no caben nuevos eventos de interrupción.
- **Sensor (`sensor`):** obtiene una medida al iniciar la tarea y repite la
  lectura cada dos segundos. Publica temperatura, presión y humedad para que
  otras partes de la aplicación puedan consultar la última muestra.
- **Consola:** lee y despacha las líneas recibidas por UART0. La tabla de
  comandos del proyecto está en [`main/tarea_consola.c`](main/tarea_consola.c);
  la lectura de líneas y el mecanismo genérico de despacho pertenecen al
  componente `consola`.

El estado compartido que consultan la consola y las tareas se copia o actualiza
protegido por secciones críticas: así `estado` no lee una muestra a medio
actualizar y el estado lógico del LED corresponde al último valor escrito.

En `idf.py menuconfig`, el menú **SEyTR - Taller 1** permite ajustar los
parámetros de tiempo y la capacidad de la cola del pulsador:

| Opción | Predeterminado | Rango |
| --- | ---: | ---: |
| Periodo del LED | 1000 ms | 100–10000 ms |
| Porcentaje encendido del LED | 50 % | 1–100 % |
| Ventana antirrebote | 200 ms | 1–500 ms |
| Cola del pulsador | 20 eventos | 5–50 eventos |

### Lectura y compensación del BME280

[`main/sensor_bme280.c`](main/sensor_bme280.c) crea el bus I2C maestro con los
pull-ups internos desactivados y conecta el sensor en `0x76`, a 100 kHz. Antes
de crear la tarea de muestreo comprueba que el registro de identificación
devuelve `0x60`, lee los coeficientes de calibración almacenados en el sensor y
configura sobremuestreo x1 para temperatura, presión y humedad.

Para cada muestra, el programa inicia una conversión forzada, consulta el
registro de estado hasta que termina y lee los registros de datos sin procesar.
La espera de conversión tiene un límite de 100 ms; las transacciones I2C tienen
un límite de 100 ms. Después aplica las fórmulas enteras de compensación del
BME280 usando los coeficientes leídos durante el arranque. La última medida se
guarda como temperatura en centésimas de grado Celsius, presión en pascales y
humedad en centésimas de porcentaje. Un error de lectura se registra y la
tarea vuelve a intentarlo en el siguiente periodo.

### Consola y diagnósticos

Las líneas de consola admiten hasta 64 caracteres. El componente normaliza los
finales de línea CR/LF, elimina espacios al principio y al final y permite
borrar con retroceso; una línea demasiado larga se descarta. El comando
`estado` no provoca una conversión: consulta el estado lógico del LED, el nivel
GPIO actual del pulsador y la última medida disponible del sensor. Si aún no
hay una medida, lo indica en lugar de mostrar valores sin inicializar.

Los comandos `trazas <led|pulsador|sensor> <on|off>` activan o silencian los
mensajes informativos periódicos de cada módulo. Los errores del sensor se
siguen registrando como errores aunque se oculten esas trazas informativas.

## Uso de la consola

Conecta el monitor serie a UART0. La consola acepta comandos de texto; escribe
`help` para mostrar la ayuda disponible:

| Comando | Acción |
| --- | --- |
| `help` | Muestra la lista de comandos. |
| `estado` | Muestra el estado del LED y del pulsador, la identificación del BME280 y la última medida disponible. |
| `led on` | Enciende el LED y detiene el parpadeo automático. |
| `led off` | Apaga el LED y detiene el parpadeo automático. |
| `led toggle` | Conmuta el LED y detiene el parpadeo automático. |
| `led auto` | Reanuda el parpadeo automático. |
| `trazas <led|pulsador|sensor> <on|off>` | Muestra u oculta las trazas del dispositivo indicado. |

El LED parpadea automáticamente al iniciar. El periodo, por defecto 1000 ms,
y el porcentaje de tiempo encendido, por defecto 50 %, se pueden cambiar en
`idf.py menuconfig`, en **SEyTR - Taller 1**. El pulsador filtra rebotes con
una ventana por defecto de 200 ms y envía los eventos a una cola configurable.
El BME280 toma y publica una medida cada 2 segundos; `estado` muestra la última
medida disponible, no inicia una lectura nueva.

## Compilar y ejecutar

Instala y configura ESP-IDF para tu sistema y selecciona el destino ESP32-S3.
Desde la raíz del proyecto:

```text
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

Sustituye `PORT` por el puerto serie de la placa, por ejemplo `COM3` en
Windows. `menuconfig` es opcional si los valores predeterminados son adecuados.
El destino y la frecuencia del tick de FreeRTOS (1000 Hz) se guardan como
valores predeterminados en [`sdkconfig.default`](sdkconfig.default).

## Organización del proyecto

- [`main/`](main/) contiene la aplicación, las tareas de FreeRTOS, la consola,
  la configuración de pines y la adquisición y compensación de medidas del
  BME280.
- [`components/`](components/) contiene los componentes reutilizables de LED,
  pulsador, bus I2C, BME280 y consola. Sus README describen sus interfaces y
  dependencias.
- [`main/main.c`](main/main.c) inicia las tareas y periféricos de la aplicación.
