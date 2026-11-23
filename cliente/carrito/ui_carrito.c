#include <stdio.h>
#include <string.h>

#include "comunicacion.h"
#include "interfaz.h"
#include "ui_carrito.h"

#define MAX_ITEMS 50

typedef struct {
    int    id;
    char   titulo[64];
    char   artista[64];
    double precio;
} ItemCarrito;

void agregar_al_carrito(int id_cancion)
{
    int r;

    com_preparar(ACC_AGREGAR_CARRITO);
    com_msg->arg = id_cancion;
    r = com_transaccion();
    if (r < 0) {
        return;
    }
    mostrar_aviso(com_msg->mensaje, r == RESP_OK ? PAR_EXITO : PAR_ERROR);
}

/* Pide el carrito al servidor. Devuelve cuantas canciones hay o -1 si se perdio la conexion */
static int cargar_carrito(ItemCarrito *items, int max)
{
    char *guardar = NULL;
    char *linea;
    int n = 0;
    int r;

    com_preparar(ACC_VER_CARRITO);
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

        ItemCarrito *it = &items[n];

        memset(it, 0, sizeof(*it));
        if (sscanf(linea, "%d;%63[^;];%63[^;];%lf",
                   &it->id, it->titulo, it->artista, &it->precio) == 4) {
            n++;
        }
    }
    return n;
}

static double total_carrito(const ItemCarrito *items, int n)
{
    double total = 0.0;
    int i;

    for (i = 0; i < n; i++) {
        total += items[i].precio;
    }
    return total;
}

/* Hace una accion simple en el servidor y muestra su mensaje. -1 = conexion perdida */
static int accion_simple(int accion, int arg)
{
    int r;

    com_preparar(accion);
    com_msg->arg = arg;
    r = com_transaccion();
    if (r < 0) {
        return -1;
    }
    mostrar_aviso(com_msg->mensaje, r == RESP_OK ? PAR_EXITO : PAR_ERROR);
    return r;
}

static void dibujar(const ItemCarrito *items, int n, int foco, int inicio)
{
    static const char *const acciones[] = {
        "$ Comprar todo",
        "- Vaciar carrito",
        "< Regresar"
    };
    int filas, columnas, ancho, alto, x0, y0 = 1, yb, visibles, i;
    int ancho_titulo;
    char total[32];

    getmaxyx(stdscr, filas, columnas);
    ancho = columnas - 4;
    if (ancho > 72) {
        ancho = 72;
    }
    alto = filas - 3;
    x0 = (columnas - ancho) / 2;
    yb = y0 + alto - 1;
    visibles = alto - 11;
    if (visibles < 1) {
        visibles = 1;
    }
    ancho_titulo = ancho - 24;

    erase();
    dibujar_marco(y0, x0, alto, ancho, "MusicSync - Carrito");

    attron(COLOR_PAIR(PAR_TITULO) | A_BOLD);
    centrar_texto(y0 + 1, x0, ancho, "MI CARRITO");
    attroff(COLOR_PAIR(PAR_TITULO) | A_BOLD);
    attron(A_DIM);
    mvprintw(y0 + 1, x0 + ancho - 9, "%2d/%-2d", n, MAX_ITEMS);
    mvprintw(y0 + 3, x0 + 2, " %-*.*s  %8s ", ancho_titulo + 8, ancho_titulo + 8, "Cancion / Artista", "Precio");
    attroff(A_DIM);
    dibujar_separador(y0 + 2, x0, ancho);

    if (n == 0) {
        attron(A_DIM);
        centrar_texto(y0 + 5, x0, ancho, "(Tu carrito esta vacio)");
        attroff(A_DIM);
    }

    for (i = 0; i < visibles && inicio + i < n; i++) {
        int idx = inicio + i;
        char nombre[140];

        snprintf(nombre, sizeof(nombre), "%s - %s", items[idx].titulo, items[idx].artista);
        if (idx == foco) {
            attron(atributo_seleccion());
        }
        mvprintw(y0 + 4 + i, x0 + 2, " %-*.*s  $%7.2f ", ancho_titulo + 8, ancho_titulo + 8,
                 nombre, items[idx].precio);
        if (idx == foco) {
            attroff(atributo_seleccion());
        }
    }
    if (inicio > 0) {
        mvaddch(y0 + 4, x0 + ancho - 2, '^');
    }
    if (inicio + visibles < n) {
        mvaddch(y0 + 3 + visibles, x0 + ancho - 2, 'v');
    }

    dibujar_separador(yb - 6, x0, ancho);
    snprintf(total, sizeof(total), "TOTAL: $%.2f", total_carrito(items, n));
    attron(COLOR_PAIR(PAR_EXITO) | A_BOLD);
    centrar_texto(yb - 5, x0, ancho, total);
    attroff(COLOR_PAIR(PAR_EXITO) | A_BOLD);

    for (i = 0; i < 3; i++) {
        if (foco == n + i) {
            attron(atributo_seleccion());
        }
        mvprintw(yb - 3 + i, x0 + 2, " %-*.*s ", ancho - 6, ancho - 6, acciones[i]);
        if (foco == n + i) {
            attroff(atributo_seleccion());
        }
    }

    dibujar_barra_estado("Flechas: mover   ENTER: elegir (sobre una cancion = quitarla)   ESC: volver");
    refresh();
}

void mostrar_carrito(void)
{
    ItemCarrito items[MAX_ITEMS];
    int n, foco = 0, inicio = 0;

    n = cargar_carrito(items, MAX_ITEMS);
    if (n < 0) {
        return;
    }

    for (;;) {
        int filas, columnas, visibles, total_focos, tecla;
        char pregunta[160];

        getmaxyx(stdscr, filas, columnas);
        (void)columnas;
        visibles = (filas - 3) - 11;
        if (visibles < 1) {
            visibles = 1;
        }

        total_focos = n + 3;
        if (foco >= total_focos) {
            foco = total_focos - 1;
        }
        if (foco < n) {
            if (foco < inicio) {
                inicio = foco;
            }
            if (foco >= inicio + visibles) {
                inicio = foco - visibles + 1;
            }
        }
        if (inicio > n - visibles) {
            inicio = n - visibles;
        }
        if (inicio < 0) {
            inicio = 0;
        }

        dibujar(items, n, foco, inicio);
        tecla = getch();

        if (tecla == KEY_UP) {
            foco = (foco - 1 + total_focos) % total_focos;
        } else if (tecla == KEY_DOWN) {
            foco = (foco + 1) % total_focos;
        } else if (tecla == 27) {
            return;
        } else if (tecla == '\n' || tecla == '\r' || tecla == KEY_ENTER) {
            if (foco < n) {
                snprintf(pregunta, sizeof(pregunta), "Quitar \"%.40s\" del carrito?", items[foco].titulo);
                if (confirmar(pregunta)) {
                    if (accion_simple(ACC_QUITAR_CARRITO, items[foco].id) < 0) {
                        return;
                    }
                    n = cargar_carrito(items, MAX_ITEMS);
                    if (n < 0) {
                        return;
                    }
                }
            } else if (foco == n) {
                if (n == 0) {
                    mostrar_aviso("Tu carrito esta vacio.", PAR_ERROR);
                } else {
                    snprintf(pregunta, sizeof(pregunta), "Comprar %d canciones por $%.2f?",
                             n, total_carrito(items, n));
                    if (confirmar(pregunta)) {
                        if (accion_simple(ACC_COMPRAR, 0) < 0) {
                            return;
                        }
                        n = cargar_carrito(items, MAX_ITEMS);
                        if (n < 0) {
                            return;
                        }
                        foco = 0;
                    }
                }
            } else if (foco == n + 1) {
                if (n == 0) {
                    mostrar_aviso("Tu carrito ya esta vacio.", PAR_ERROR);
                } else if (confirmar("Vaciar todo el carrito?")) {
                    if (accion_simple(ACC_VACIAR_CARRITO, 0) < 0) {
                        return;
                    }
                    n = cargar_carrito(items, MAX_ITEMS);
                    if (n < 0) {
                        return;
                    }
                    foco = 0;
                }
            } else {
                return;
            }
        }
    }
}
