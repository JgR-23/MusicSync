#include <stdio.h>
#include <string.h>

#include "admin_srv.h"
#include "biblioteca.h"
#include "respuesta.h"

static int requiere_admin(Mensaje *m, const Sesion *s)
{
    if (!s->logueado || !s->es_admin) {
        resp_error(m, "Operacion solo para el administrador");
        return 0;
    }
    return 1;
}

/* Valida y separa "titulo;artista;album;genero;duracion;precio" */
static int leer_cancion(const char *entrada, CancionSrv *c)
{
    double precio;
    int minutos, segundos;
    char extra;

    memset(c, 0, sizeof(*c));
    if (sscanf(entrada, "%63[^;];%63[^;];%63[^;];%63[^;];%9[^;];%lf",
               c->titulo, c->artista, c->album, c->genero,
               c->duracion, &precio) != 6) {
        return 0;
    }
    if (sscanf(c->duracion, "%d:%d%c", &minutos, &segundos, &extra) != 2 ||
        minutos < 0 || minutos > 99 || segundos < 0 || segundos > 59) {
        return 0;
    }
    if (precio <= 0.0 || precio > 99999.0) {
        return 0;
    }
    c->precio = precio;
    return 1;
}

void srv_adm_agregar_cancion(Mensaje *m, const Sesion *s)
{
    CancionSrv lista[MAX_BIBLIOTECA];
    CancionSrv nueva;
    int n, i, mayor = 0;

    if (!requiere_admin(m, s)) {
        return;
    }
    if (!leer_cancion(m->entrada, &nueva)) {
        resp_error(m, "Datos de la cancion invalidos");
        return;
    }
    n = biblioteca_cargar(lista, MAX_BIBLIOTECA);
    if (n >= MAX_BIBLIOTECA) {
        resp_error(m, "La biblioteca esta llena");
        return;
    }
    for (i = 0; i < n; i++) {
        if (lista[i].id > mayor) {
            mayor = lista[i].id;
        }
    }
    nueva.id = mayor + 1;
    lista[n++] = nueva;

    if (biblioteca_guardar(lista, n) != 0) {
        resp_error(m, "No se pudo guardar la biblioteca");
        return;
    }
    resp_ok(m, "Cancion agregada correctamente");
}

void srv_adm_modificar_cancion(Mensaje *m, const Sesion *s)
{
    CancionSrv lista[MAX_BIBLIOTECA];
    CancionSrv nueva;
    int n, i, idx = -1;

    if (!requiere_admin(m, s)) {
        return;
    }
    if (!leer_cancion(m->entrada, &nueva)) {
        resp_error(m, "Datos de la cancion invalidos");
        return;
    }
    n = biblioteca_cargar(lista, MAX_BIBLIOTECA);
    for (i = 0; i < n; i++) {
        if (lista[i].id == m->arg) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        resp_error(m, "Cancion no encontrada");
        return;
    }
    nueva.id = lista[idx].id;
    lista[idx] = nueva;

    if (biblioteca_guardar(lista, n) != 0) {
        resp_error(m, "No se pudo guardar la biblioteca");
        return;
    }
    resp_ok(m, "Cancion modificada correctamente");
}

void srv_adm_eliminar_cancion(Mensaje *m, const Sesion *s)
{
    CancionSrv lista[MAX_BIBLIOTECA];
    int n, i, idx = -1;

    if (!requiere_admin(m, s)) {
        return;
    }
    n = biblioteca_cargar(lista, MAX_BIBLIOTECA);
    for (i = 0; i < n; i++) {
        if (lista[i].id == m->arg) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        resp_error(m, "Cancion no encontrada");
        return;
    }
    for (i = idx; i < n - 1; i++) {
        lista[i] = lista[i + 1];
    }
    n--;

    if (biblioteca_guardar(lista, n) != 0) {
        resp_error(m, "No se pudo guardar la biblioteca");
        return;
    }
    resp_ok(m, "Cancion eliminada correctamente");
}
