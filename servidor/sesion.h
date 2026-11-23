#ifndef SESION_H
#define SESION_H

#include "protocolo.h"

/* Estado de la sesion de un cliente; vive dentro de su hilo */
typedef struct {
    int  logueado;
    int  es_admin;
    char usuario[LON_USUARIO];
} Sesion;

#endif
