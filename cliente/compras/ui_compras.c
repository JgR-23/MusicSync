#include <stdio.h>
#include <string.h>

#include "comunicacion.h"
#include "interfaz.h"
#include "ipc_util.h"
#include "menus.h"
#include "ui_compras.h"

#define MAX_COMPRAS 100

typedef struct {
    int    id;
    char   titulo[64];
    char   artista[64];
    char   fecha[12];
    double precio;
} Compra;

/* Devuelve cuantas compras hay, o -1 si se perdio la conexion */
static int cargar_compras(Compra *lista, int max)
{
    char *guardar = NULL;
    char *linea;
    int n = 0;
    int r;

    com_preparar(ACC_VER_COMPRAS);
    r = com_transaccion();
    if (r < 0) {
        return -1;
    }
    if (r != RESP_OK) {
        mostrar_aviso(com_msg->mensaje, PAR_ERROR);
        return 0;
    }
    for (linea = strtok_r(com_msg->datos, "\n", &guardar);
         linea != NULL && n < max;
         linea = strtok_r(NULL, "\n", &guardar)) {

        Compra *c = &lista[n];

        memset(c, 0, sizeof(*c));
        if (sscanf(linea, "%d;%63[^;];%63[^;];%11[^;];%lf",
                   &c->id, c->titulo, c->artista, c->fecha, &c->precio) == 5) {
            n++;
        }
    }
    return n;
}

/* Deja elegir una playlist del cliente.
 * 1 = elegida (nombre en "nombre"), 0 = cancelo o no tiene playlists, -1 = conexion perdida */
static int elegir_playlist(char *nombre, size_t tam)
{
    char nombres[MAX_PLAYLISTS_USR][LON_NOMBRE];
    const char *punteros[MAX_PLAYLISTS_USR];
    char *guardar = NULL;
    char *linea;
    int n = 0;
    int r, elegida;

    com_preparar(ACC_VER_PLAYLISTS);
    r = com_transaccion();
    if (r < 0) {
        return -1;
    }
    if (r != RESP_OK) {
        mostrar_aviso(com_msg->mensaje, PAR_ERROR);
        return 0;
    }
    for (linea = strtok_r(com_msg->datos, "\n", &guardar);
         linea != NULL && n < MAX_PLAYLISTS_USR;
         linea = strtok_r(NULL, "\n", &guardar)) {

        char *fin = strchr(linea, ';');

        if (fin != NULL) {
            *fin = '\0';
        }
        copiar_cadena(nombres[n], LON_NOMBRE, linea);
        punteros[n] = nombres[n];
        n++;
    }
    if (n == 0) {
        mostrar_aviso("No tienes playlists. Crea una primero.", PAR_ERROR);
        return 0;
    }

    elegida = menu_opciones("Agregar a playlist", "Elige la playlist", punteros, n);
    if (elegida < 0) {
        return 0;
    }
    copiar_cadena(nombre, tam, nombres[elegida]);
    return 1;
}

void mostrar_compras(void)
{
    Compra lista[MAX_COMPRAS];
    char etiquetas[MAX_COMPRAS][100];
    const char *punteros[MAX_COMPRAS];
    int ultima = 0;

    for (;;) {
        char playlist[LON_NOMBRE];
        int n, i, sel, r;

        n = cargar_compras(lista, MAX_COMPRAS);
        if (n < 0) {
            return;
        }
        if (n == 0) {
            mostrar_aviso("Aun no has comprado canciones.", PAR_ERROR);
            return;
        }
        for (i = 0; i < n; i++) {
            snprintf(etiquetas[i], sizeof(etiquetas[i]), "%-26.26s %-18.18s %.10s",
                     lista[i].titulo, lista[i].artista, lista[i].fecha);
            punteros[i] = etiquetas[i];
        }
        if (ultima >= n) {
            ultima = 0;
        }

        sel = menu_opciones_desde("Mis compras",
                                  "ENTER: agregar la cancion a una playlist",
                                  punteros, n, ultima);
        if (sel < 0) {
            return;
        }
        ultima = sel;

        r = elegir_playlist(playlist, sizeof(playlist));
        if (r < 0) {
            return;
        }
        if (r == 0) {
            continue;
        }

        com_preparar(ACC_AGREGAR_A_PL);
        copiar_cadena(com_msg->texto, sizeof(com_msg->texto), playlist);
        com_msg->arg = lista[sel].id;
        r = com_transaccion();
        if (r < 0) {
            return;
        }
        mostrar_aviso(com_msg->mensaje, r == RESP_OK ? PAR_EXITO : PAR_ERROR);
    }
}