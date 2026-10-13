# CMakeLists.txt de componente. Registra las fuentes y las cabeceras publicas.
# linea.c no depende de ESP-IDF: es la parte que acumula y normaliza la linea.
# consola.c lee de la UART y despacha los comandos.
# La cabecera publica solo usa esp_err.h, de esp_common, que se incluye siempre,
# asi que no hace falta REQUIRES. PRIV_REQUIRES lleva lo que solo usa el codigo.
idf_component_register(SRCS "consola.c" "linea.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_driver_uart)
