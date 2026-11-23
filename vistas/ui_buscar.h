#ifndef UI_BUSCAR_H
#define UI_BUSCAR_H

#include "ui_comun.h"

bool pantalla_buscar_por(const CancionUI *catalogo, int total_catalogo,
                       CancionUI *salida, int *total_salida,
                       char *etiqueta, size_t etiqueta_len);

#endif
