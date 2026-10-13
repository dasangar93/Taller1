# CMakeLists.txt de componente. Registra las fuentes y las cabeceras publicas.
# La cabecera publica incluye driver/i2c_master.h: esp_driver_i2c va en REQUIRES.
idf_component_register(SRCS "bme280.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_i2c)
