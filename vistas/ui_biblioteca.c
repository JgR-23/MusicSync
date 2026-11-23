#include "ui_biblioteca.h"
#include "ui_buscar.h"
#include "ipc_cliente.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

/* Muestra las playlists del usuario y agrega la cancion a la que elija */
static void agregar_a_playlist(const CancionUI *cancion) {
    RegPlaylist playlists[IPC_MAX_PLAYLISTS];
    int total = ipc_listar_playlists(playlists, IPC_MAX_PLAYLISTS);
    int sel = 0;
    int scroll = 0;
    char msg[MAX_TEXTO * 2];

    if (total < 0) {
        mostrar_mensaje_ui(ipc_ultimo_error());
        return;
    }
    if (total == 0) {
        mostrar_mensaje_ui("No tienes playlists. Crea una en Gestion de playlist.");
        return;
    }

    while (1) {
        int alto, ancho;
        getmaxyx(stdscr, alto, ancho);
        erase();
        box(stdscr, 0, 0);

        mvprintw(1, 2, "Agregar a playlist");
        mvhline(2, 1, ACS_HLINE, ancho - 2);
        mvprintw(3, 2, "Cancion: %.*s", ancho - 14 > 0 ? ancho - 14 : 1, cancion->titulo);
        mvhline(4, 1, ACS_HLINE, ancho - 2);

        int top = 6;
        int visibles = alto - top - 3;
        if (visibles < 1) visibles = 1;

        if (sel < scroll) scroll = sel;
        else if (sel >= scroll + visibles) scroll = sel - visibles + 1;

        for (int i = 0; i < visibles; i++) {
            int idx = scroll + i;
            if (idx >= total) break;

            if (idx == sel) attron(A_REVERSE | A_BOLD);
            mvprintw(top + i, 4, "%-30.30s %2d cancion(es)",
                     playlists[idx].nombre, playlists[idx].cantidad);
            if (idx == sel) attroff(A_REVERSE | A_BOLD);
        }

        mvhline(alto - 2, 1, ACS_HLINE, ancho - 2);
        mvprintw(alto - 1, 2, "Flechas arriba/abajo | ENTER agregar | ESC cancelar");
        refresh();

        int ch = getch();
        if (ch == KEY_UP) {
            sel = (sel - 1 + total) % total;
        } else if (ch == KEY_DOWN) {
            sel = (sel + 1) % total;
        } else if (ch == '\n' || ch == KEY_ENTER) {
            if (ipc_agregar_cancion(playlists[sel].nombre, cancion->id) == 0) {
                snprintf(msg, sizeof(msg), "Cancion agregada a \"%s\".", playlists[sel].nombre);
                mostrar_mensaje_ui(msg);
            } else {
                mostrar_mensaje_ui(ipc_ultimo_error());
            }
            return;
        } else if (ch == TECLA_ESC) {
            return;
        }
    }
}

void pantalla_biblioteca(const CancionUI *catalogo, int total_catalogo) {
    CancionUI lista[MAX_CANCIONES];
    int total = total_catalogo > MAX_CANCIONES ? MAX_CANCIONES : total_catalogo;
    memcpy(lista, catalogo, (size_t)total * sizeof(CancionUI));

    char filtro[MAX_TEXTO * 2] = "Todas";
    int foco = 0;   /* 0 = "Buscar cancion", 1..total = lista[foco - 1] */
    int scroll = 0;

    while (1) {
        int alto, ancho;
        getmaxyx(stdscr, alto, ancho);
        erase();
        box(stdscr, 0, 0);

        mvprintw(1, 2, "Biblioteca");
        if (ancho > 26) {
            mvprintw(1, ancho - 24, "[%.*s]", 20, filtro);
        }
        mvhline(2, 1, ACS_HLINE, ancho - 2);

        if (foco == 0) attron(A_REVERSE | A_BOLD);
        mvprintw(3, 4, "%-20s", "Buscar cancion");
        if (foco == 0) attroff(A_REVERSE | A_BOLD);

        mvhline(4, 1, ACS_HLINE, ancho - 2);
        mvprintw(5, 2, "Lista de canciones (%d)", total);

        int top = 7;
        int visibles = alto - top - 3;
        if (visibles < 1) visibles = 1;

        int max_foco = 1 + total;
        if (foco >= max_foco) foco = max_foco - 1;

        int idx_sel = foco > 0 ? foco - 1 : 0;
        if (idx_sel < scroll) scroll = idx_sel;
        else if (idx_sel >= scroll + visibles) scroll = idx_sel - visibles + 1;

        int ancho_titulo = ancho - 20;
        if (ancho_titulo < 10) ancho_titulo = 10;

        for (int i = 0; i < visibles; i++) {
            int idx = scroll + i;
            if (idx >= total) break;
            int item_foco = idx + 1;

            if (foco == item_foco) attron(A_REVERSE | A_BOLD);
            mvprintw(top + i, 4, "%-*.*s  $%6.2f",
                     ancho_titulo, ancho_titulo,
                     lista[idx].titulo, lista[idx].precio);
            if (foco == item_foco) attroff(A_REVERSE | A_BOLD);
        }

        mvhline(alto - 2, 1, ACS_HLINE, ancho - 2);
        mvprintw(alto - 1, 2, "Flechas arriba/abajo | ENTER seleccionar | ESC salir");
        refresh();

        int ch = getch();
        if (ch == KEY_UP) {
            if (foco > 0) foco--;
        } else if (ch == KEY_DOWN) {
            if (foco + 1 < max_foco) foco++;
        } else if (ch == '\n' || ch == KEY_ENTER) {
            if (foco == 0) {
                char nueva_etiqueta[MAX_TEXTO * 2];
                if (pantalla_buscar_por(catalogo, total_catalogo, lista, &total,
                                        nueva_etiqueta, sizeof(nueva_etiqueta))) {
                    snprintf(filtro, sizeof(filtro), "%s", nueva_etiqueta);
                    foco = total > 0 ? 1 : 0;
                    scroll = 0;
                }
            } else {
                /* ENTER sobre una cancion: elegir a que playlist agregarla */
                agregar_a_playlist(&lista[foco - 1]);
            }
        } else if (ch == TECLA_ESC) {
            return;
        }
    }
}
