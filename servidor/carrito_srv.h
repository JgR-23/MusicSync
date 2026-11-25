#ifndef CARRITO_SRV_H
#define CARRITO_SRV_H

#include "protocolo.h"
#include "sesion.h"

void srv_ver_carrito(Mensaje *m, const Sesion *s);
void srv_agregar_carrito(Mensaje *m, const Sesion *s);
void srv_quitar_carrito(Mensaje *m, const Sesion *s);
void srv_vaciar_carrito(Mensaje *m, const Sesion *s);
void srv_comprar(Mensaje *m, const Sesion *s);

/* Compras y saldo del cliente */
void srv_ver_compras(Mensaje *m, const Sesion *s);
void srv_ver_saldo(Mensaje *m, const Sesion *s);

/* 1 si el usuario ya compro esa cancion (consulta datos/ventas.txt) */
int  cancion_comprada(const char *usuario, int id);

#endif