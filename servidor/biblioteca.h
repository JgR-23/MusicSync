#ifndef BIBLIOTECA_H
#define BIBLIOTECA_H

#include "protocolo.h"
#include "sesion.h"

#define MAX_BIBLIOTECA 200

typedef struct {
    int    id;
    char   titulo[64];
    char   artista[64];
    char   album[64];
    char   genero[64];
    char   duracion[10];
    double precio;
} CancionSrv;

int  biblioteca_buscar(int id, CancionSrv *salida);   /* 1 = encontrada */
int  biblioteca_cargar(CancionSrv *lista, int max);   /* devuelve cuantas leyo */
int  biblioteca_guardar(const CancionSrv *lista, int n); /* 0 = ok, -1 = error */
void srv_biblioteca(Mensaje *m, const Sesion *s);

#endif
