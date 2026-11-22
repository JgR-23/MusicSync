#ifndef UI_COMUN_H
#define UI_COMUN_H

#include <stdbool.h>
#include <stddef.h>

#define TECLA_ESC 27
#define MAX_CANCIONES 32
#define MAX_TEXTO 64

typedef enum {
    CRITERIO_NOMBRE = 0,
    CRITERIO_ARTISTA,
    CRITERIO_ALBUM,
    CRITERIO_GENERO
} CriterioBusqueda;

typedef struct {
    int id;
    char titulo[MAX_TEXTO];
    char artista[MAX_TEXTO];
    char album[MAX_TEXTO];
    char genero[MAX_TEXTO];
    double precio;
} CancionUI;

const char *nombre_criterio(CriterioBusqueda c);
bool validar_texto(const char *raw, char *out, size_t out_size);
bool contiene_texto(const char *haystack, const char *needle);
void mostrar_mensaje_ui(const char *msg);

#endif
