/* linea.c - acumular y normalizar una línea de la consola. Ver linea.h. */
#include "linea.h"

static bool es_espacio(char c)
{
    return c == ' ' || c == '\t';
}

void linea_iniciar(linea_t *l, char *buf, size_t cap, bool normalizar)
{
    l->buf = buf;
    l->cap = cap;
    l->largo = 0;
    l->desbordada = false;
    l->normalizar = normalizar;
    l->tras_cr = false;
    l->buf[0] = '\0';
}

/* Cierra la línea en curso y decide qué se entrega. */
static linea_estado_t cerrar(linea_t *l)
{
    bool desbordada = l->desbordada;
    size_t largo = l->largo;

    l->largo = 0;
    l->desbordada = false;

    if (desbordada) {
        l->buf[0] = '\0';
        return LINEA_DESCARTADA;
    }

    l->buf[largo] = '\0';
    if (l->normalizar) {
        /* recortar por el final */
        while (largo > 0 && es_espacio(l->buf[largo - 1])) {
            l->buf[--largo] = '\0';
        }
        /* recortar por el principio */
        size_t ini = 0;
        while (ini < largo && es_espacio(l->buf[ini])) {
            ini++;
        }
        if (ini > 0) {
            for (size_t i = 0; i <= largo - ini; i++) {
                l->buf[i] = l->buf[i + ini];
            }
            largo -= ini;
        }
        if (largo == 0) {
            return LINEA_VACIA;
        }
    }
    return LINEA_COMPLETA;
}

linea_estado_t linea_recibir(linea_t *l, char c)
{
    bool tras_cr = l->tras_cr;
    l->tras_cr = (c == '\r');

    if (c == '\n' && tras_cr && l->normalizar) {
        return LINEA_SIGUE;         /* segunda mitad de un CR LF: ya se cerró */
    }
    if (c == '\r' || c == '\n') {
        return cerrar(l);
    }

    if (l->normalizar) {
        if (c == '\b' || c == 0x7f) {
            if (l->largo > 0 && !l->desbordada) {
                l->largo--;
                return LINEA_BORRADO;
            }
            return LINEA_SIGUE;
        }
        if ((unsigned char)c < 0x20 && c != '\t') {
            return LINEA_SIGUE;     /* otros caracteres de control: fuera */
        }
    }

    /* La cota: se guarda como mucho cap - 1 caracteres, para el '\0'. */
    if (l->largo >= l->cap - 1) {
        l->desbordada = true;
        return LINEA_SIGUE;
    }
    l->buf[l->largo++] = c;
    return LINEA_SIGUE;
}
