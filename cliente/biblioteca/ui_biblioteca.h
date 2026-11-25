#ifndef UI_BIBLIOTECA_H
#define UI_BIBLIOTECA_H

#include "ui_comun.h"

/* al_elegir: funcion que se ejecuta con ENTER sobre una cancion (puede ser NULL)
 * info: texto corto para el encabezado, por ejemplo "Saldo: $500.00" (puede ser NULL) */
void pantalla_biblioteca(const CancionUI *catalogo, int total_catalogo,
                         AccionCancion al_elegir, const char *info);

#endif