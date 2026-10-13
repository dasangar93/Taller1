# CMakeLists.txt de componente. Registra las fuentes y las cabeceras publicas.
# La cabecera publica incluye driver/i2c_master.h, asi que esp_driver_i2c va en
# REQUIRES y no en PRIV_REQUIRES: quien use este componente la necesita tambien.
idf_component_register(SRCS "bus_i2c.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_i2c)
