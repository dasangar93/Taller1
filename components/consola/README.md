# Componente `consola`

Una consola de comandos sobre una UART, la del monitor en este proyecto. Lee
lo que se teclea, forma la línea, busca el comando en una tabla y responde.

Está hecha en cuatro bloques separados:

1. **Leer.** Una tarea saca de la UART los caracteres que hayan llegado, de uno
   en uno, y hace el eco.
2. **Acumular y normalizar.** `linea.c` forma la línea con una cota de tamaño
   y la deja limpia: da igual que el terminal mande CR, LF o los dos, se
   recortan los espacios de los extremos, el retroceso borra y los demás
   caracteres de control se ignoran.
3. **Analizar.** Se separa el nombre del comando de sus argumentos y se busca
   el nombre en la tabla.
4. **Responder.** Siempre, también a un comando que no existe, a uno con
   argumentos que no admite y a una línea demasiado larga.

**No sabe qué comandos hay.** La tabla la pone quien usa la consola. En el
código de `main`:

```c
#include "consola.h"

static void cmd_hola(const char *args)
{
    printf("Hola\n");
}

static const consola_comando_t comandos[] = {
    { "hola", NULL, "Saluda", cmd_hola },
};

void app_main(void)
{
    consola_config_t cfg = {
        .puerto = 0,
        .largo_maximo = 64,
        .tabla = comandos,
        .n_comandos = sizeof comandos / sizeof comandos[0],
    };
    ESP_ERROR_CHECK(consola_iniciar(&cfg));
}
```

- Cada fila de la tabla tiene el nombre, cómo se escriben los argumentos
  (`NULL` si no admite ninguno), una línea de ayuda y la función que lo
  ejecuta. La función recibe los argumentos como una cadena, vacía si no hay.
- `largo_maximo` es la cota de la línea. Una línea más larga se descarta
  entera y la consola lo dice. Tiene que ser 8 o más.
- `consola_iniciar()` instala el controlador de la UART en el orden de su
  documentación: instalar el controlador, aplicar la configuración y asignar
  los pines. **El puerto tiene que ser el del monitor**, la UART0 por omisión:
  ya tiene sus pines y no se tocan, y se reconfigura a la velocidad que fija
  para el monitor la configuración de ESP-IDF. Después hace que `printf()` y
  las trazas escriban a través del controlador, el mismo camino que el eco, y
  arranca la tarea.
- `consola_ayuda()` escribe la lista de comandos a partir de la tabla.
- `consola_normalizar(false)` desactiva la normalización: la línea se compara
  tal como llega. Sirve para ver por qué hace falta. La cota se mantiene.

`linea.c` y `linea.h` no dependen de ESP-IDF: son C estándar y se pueden
probar en el ordenador.

Depende de `esp_driver_uart`, declarado en `PRIV_REQUIRES` porque su cabecera
no aparece en la cabecera pública del componente.
