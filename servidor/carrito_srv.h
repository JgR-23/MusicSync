#ifndef CARRITO_SRV_H
#define CARRITO_SRV_H

#include "protocolo.h"
#include "sesion.h"

void srv_ver_carrito(Mensaje *m, const Sesion *s);
void srv_agregar_carrito(Mensaje *m, const Sesion *s);
void srv_quitar_carrito(Mensaje *m, const Sesion *s);
void srv_vaciar_carrito(Mensaje *m, const Sesion *s);
void srv_comprar(Mensaje *m, const Sesion *s);

#endif
