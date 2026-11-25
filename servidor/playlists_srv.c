#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "biblioteca.h"
#include "carrito_srv.h"
#include "ipc_util.h"
#include "playlists_srv.h"
#include "respuesta.h"

/* Archivo por usuario: datos/playlists_<usuario>.txt
 * Cada linea: nombre;id,id,id                                          */

typedef struct {
    char nombre[LON_NOMBRE];
    int  ids[MAX_CANC_PLAYLIST];
    int  n;
} Playlist;

static void ruta_archivo(const char *usuario, char *destino, size_t tam)
{
    snprintf(destino, tam, "datos/playlists_%.20s.txt", usuario);
}

static int cargar(const char *usuario, Playlist *pl, int max)
{
    char ruta[96];
    char linea[512];
    FILE *archivo;
    int total = 0;

    ruta_archivo(usuario, ruta, sizeof(ruta));
    archivo = fopen(ruta, "r");
    if (archivo == NULL) {
        return 0;
    }

    while (total < max && fgets(linea, sizeof(linea), archivo)) {
        char *separador;
        char *p;
        Playlist *actual = &pl[total];

        linea[strcspn(linea, "\r\n")] = '\0';
        separador = strchr(linea, ';');
        if (separador == NULL) {
            continue;
        }
        *separador = '\0';

        memset(actual, 0, sizeof(*actual));
        copiar_cadena(actual->nombre, sizeof(actual->nombre), linea);

        p = separador + 1;
        while (*p != '\0') {
            char *fin;
            long valor = strtol(p, &fin, 10);

            if (fin == p) {
                break;
            }
            if (actual->n < MAX_CANC_PLAYLIST) {
                actual->ids[actual->n++] = (int)valor;
            }
            p = (*fin == ',') ? fin + 1 : fin;
        }
        total++;
    }
    fclose(archivo);
    return total;
}

static int guardar(const char *usuario, const Playlist *pl, int total)
{
    char ruta[96];
    FILE *archivo;
    int i, j;

    ruta_archivo(usuario, ruta, sizeof(ruta));
    archivo = fopen(ruta, "w");
    if (archivo == NULL) {
        return -1;
    }
    for (i = 0; i < total; i++) {
        fprintf(archivo, "%s;", pl[i].nombre);
        for (j = 0; j < pl[i].n; j++) {
            fprintf(archivo, "%s%d", j > 0 ? "," : "", pl[i].ids[j]);
        }
        fprintf(archivo, "\n");
    }
    fclose(archivo);
    return 0;
}

static int buscar(const Playlist *pl, int total, const char *nombre)
{
    int i;

    for (i = 0; i < total; i++) {
        if (strcmp(pl[i].nombre, nombre) == 0) {
            return i;
        }
    }
    return -1;
}

static int nombre_valido(const char *nombre)
{
    size_t n = strlen(nombre);
    size_t i;
    int visible = 0;

    if (n == 0 || n >= LON_NOMBRE) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)nombre[i];
        if (c < 32 || c == 127 || c == ';') {
            return 0;
        }
        if (c != ' ') {
            visible = 1;
        }
    }
    return visible;
}

void srv_ver_playlists(Mensaje *m, const Sesion *s)
{
    Playlist pl[MAX_PLAYLISTS_USR];
    char fila[128];
    int total, i;

    if (!s->logueado) {
        resp_error(m, "Inicia sesion primero");
        return;
    }
    total = cargar(s->usuario, pl, MAX_PLAYLISTS_USR);
    for (i = 0; i < total; i++) {
        snprintf(fila, sizeof(fila), "%.49s;%d\n", pl[i].nombre, pl[i].n);
        datos_agregar(m, fila);
    }
    resp_ok(m, "OK");
}

void srv_crear_playlist(Mensaje *m, const Sesion *s)
{
    Playlist pl[MAX_PLAYLISTS_USR];
    int total, i;

    if (!s->logueado) {
        resp_error(m, "Inicia sesion primero");
        return;
    }
    if (!nombre_valido(m->texto)) {
        resp_error(m, "Nombre invalido (no uses ';' ni lo dejes vacio)");
        return;
    }

    total = cargar(s->usuario, pl, MAX_PLAYLISTS_USR);
    if (total >= MAX_PLAYLISTS_USR) {
        resp_error(m, "Limite de playlists alcanzado");
        return;
    }
    for (i = 0; i < total; i++) {
        if (strcasecmp(pl[i].nombre, m->texto) == 0) {
            resp_error(m, "Ese nombre ya existe");
            return;
        }
    }

    memset(&pl[total], 0, sizeof(pl[total]));
    copiar_cadena(pl[total].nombre, sizeof(pl[total].nombre), m->texto);
    total++;

    if (guardar(s->usuario, pl, total) != 0) {
        resp_error(m, "No se pudo guardar la playlist");
        return;
    }
    resp_ok(m, "Playlist creada");
}

void srv_eliminar_playlist(Mensaje *m, const Sesion *s)
{
    Playlist pl[MAX_PLAYLISTS_USR];
    int total, idx, i;

    if (!s->logueado) {
        resp_error(m, "Inicia sesion primero");
        return;
    }
    total = cargar(s->usuario, pl, MAX_PLAYLISTS_USR);
    idx = buscar(pl, total, m->texto);
    if (idx < 0) {
        resp_error(m, "Playlist no encontrada");
        return;
    }
    for (i = idx; i < total - 1; i++) {
        pl[i] = pl[i + 1];
    }
    total--;

    if (guardar(s->usuario, pl, total) != 0) {
        resp_error(m, "No se pudo guardar el cambio");
        return;
    }
    resp_ok(m, "Playlist eliminada");
}

void srv_ver_canciones_pl(Mensaje *m, const Sesion *s)
{
    Playlist pl[MAX_PLAYLISTS_USR];
    CancionSrv c;
    char fila[512];
    int total, idx, i;

    if (!s->logueado) {
        resp_error(m, "Inicia sesion primero");
        return;
    }
    total = cargar(s->usuario, pl, MAX_PLAYLISTS_USR);
    idx = buscar(pl, total, m->texto);
    if (idx < 0) {
        resp_error(m, "Playlist no encontrada");
        return;
    }

    for (i = 0; i < pl[idx].n; i++) {
        if (biblioteca_buscar(pl[idx].ids[i], &c)) {
            snprintf(fila, sizeof(fila), "%d;%.63s;%.63s;%.63s;%.63s;%.9s\n",
                     c.id, c.titulo, c.artista, c.album, c.genero, c.duracion);
        } else {
            /* Cancion que ya no esta en la biblioteca: se conserva la
             * posicion para que los indices del cliente sigan alineados */
            snprintf(fila, sizeof(fila), "%d;(no disponible);-;-;-;--:--\n",
                     pl[idx].ids[i]);
        }
        datos_agregar(m, fila);
    }
    resp_ok(m, "OK");
}

void srv_agregar_a_pl(Mensaje *m, const Sesion *s)
{
    Playlist pl[MAX_PLAYLISTS_USR];
    CancionSrv c;
    int total, idx, i;

    if (!s->logueado) {
        resp_error(m, "Inicia sesion primero");
        return;
    }
    total = cargar(s->usuario, pl, MAX_PLAYLISTS_USR);
    idx = buscar(pl, total, m->texto);
    if (idx < 0) {
        resp_error(m, "Playlist no encontrada");
        return;
    }
    if (!biblioteca_buscar(m->arg, &c)) {
        resp_error(m, "La cancion no existe en la biblioteca");
        return;
    }
    if (!cancion_comprada(s->usuario, m->arg)) {
        resp_error(m, "Primero debes comprar esta cancion");
        return;
    }
    for (i = 0; i < pl[idx].n; i++) {
        if (pl[idx].ids[i] == m->arg) {
            resp_error(m, "La cancion ya esta en esa playlist");
            return;
        }
    }
    if (pl[idx].n >= MAX_CANC_PLAYLIST) {
        resp_error(m, "La playlist esta llena");
        return;
    }

    pl[idx].ids[pl[idx].n++] = m->arg;

    if (guardar(s->usuario, pl, total) != 0) {
        resp_error(m, "No se pudo guardar el cambio");
        return;
    }
    resp_ok(m, "Cancion agregada a la playlist");
}

void srv_quitar_de_pl(Mensaje *m, const Sesion *s)
{
    Playlist pl[MAX_PLAYLISTS_USR];
    int total, idx, i;

    if (!s->logueado) {
        resp_error(m, "Inicia sesion primero");
        return;
    }
    total = cargar(s->usuario, pl, MAX_PLAYLISTS_USR);
    idx = buscar(pl, total, m->texto);
    if (idx < 0) {
        resp_error(m, "Playlist no encontrada");
        return;
    }
    if (m->arg < 0 || m->arg >= pl[idx].n) {
        resp_error(m, "Posicion de cancion invalida");
        return;
    }
    for (i = m->arg; i < pl[idx].n - 1; i++) {
        pl[idx].ids[i] = pl[idx].ids[i + 1];
    }
    pl[idx].n--;

    if (guardar(s->usuario, pl, total) != 0) {
        resp_error(m, "No se pudo guardar el cambio");
        return;
    }
    resp_ok(m, "Cancion eliminada de la playlist");
}