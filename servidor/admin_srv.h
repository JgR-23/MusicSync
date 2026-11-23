#ifndef ADMIN_SRV_H
#define ADMIN_SRV_H

#include "protocolo.h"
#include "sesion.h"

/* Administracion de canciones (productos). Los datos de la cancion llegan en
 * m->entrada con el formato: titulo;artista;album;genero;duracion;precio */
void srv_adm_agregar_cancion(Mensaje *m, const Sesion *s);
void srv_adm_modificar_cancion(Mensaje *m, const Sesion *s);   /* m->arg = id */
void srv_adm_eliminar_cancion(Mensaje *m, const Sesion *s);    /* m->arg = id */

#endif
