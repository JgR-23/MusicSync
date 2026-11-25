#include "ui_biblioteca.h"
#include "ui_buscar.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

void pantalla_biblioteca(const CancionUI *catalogo, int total_catalogo,
                         AccionCancion al_elegir, const char *info) {
    CancionUI lista[MAX_CANCIONES];
    int total_completo = total_catalogo > MAX_CANCIONES ? MAX_CANCIONES : total_catalogo;
    int total = total_completo;
    memcpy(lista, catalogo, (size_t)total * sizeof(CancionUI));

    char filtro[MAX_TEXTO * 2] = "Todas";
    int filtrado = 0;   /* 1 si la lista esta filtrada por una busqueda */
    /* foco: 0 = "Buscar cancion"
     *       1 = "Quitar filtro" (solo si filtrado)
     *       despues: las canciones de la lista                           */
    int foco = 0;
    int scroll = 0;

    while (1) {
        int alto, ancho;
        getmaxyx(stdscr, alto, ancho);
        erase();
        box(stdscr, 0, 0);

        int base = filtrado ? 2 : 1;   /* foco de la primera cancion */

        mvprintw(1, 2, "MusicSync - Biblioteca");
        if (ancho > 36) {
            mvprintw(1, ancho - 24, "[%.*s]", 20, filtro);
        }
        if (info != NULL && ancho > 76) {
            attron(A_BOLD);
            mvprintw(1, 28, "%.*s", ancho - 56, info);
            attroff(A_BOLD);
        }
        mvhline(2, 1, ACS_HLINE, ancho - 2);

        if (foco == 0) attron(A_REVERSE | A_BOLD);
        mvprintw(3, 4, "%-28s", "Buscar cancion");
        if (foco == 0) attroff(A_REVERSE | A_BOLD);

        if (filtrado) {
            if (foco == 1) attron(A_REVERSE | A_BOLD);
            mvprintw(4, 4, "%-28s", "Quitar filtro (ver todas)");
            if (foco == 1) attroff(A_REVERSE | A_BOLD);
        }

        mvhline(5, 1, ACS_HLINE, ancho - 2);
        mvprintw(6, 2, "Lista de canciones (%d)", total);

        int top = 8;
        int visibles = alto - top - 3;
        if (visibles < 1) visibles = 1;

        int max_foco = base + total;
        if (foco >= max_foco) foco = max_foco - 1;

        int idx_sel = foco >= base ? foco - base : 0;
        if (idx_sel < scroll) scroll = idx_sel;
        else if (idx_sel >= scroll + visibles) scroll = idx_sel - visibles + 1;

        int ancho_titulo = ancho - 20;
        if (ancho_titulo < 10) ancho_titulo = 10;

        for (int i = 0; i < visibles; i++) {
            int idx = scroll + i;
            if (idx >= total) break;
            int item_foco = idx + base;

            if (foco == item_foco) attron(A_REVERSE | A_BOLD);
            mvprintw(top + i, 4, "%-*.*s  $%6.2f",
                     ancho_titulo, ancho_titulo,
                     lista[idx].titulo, lista[idx].precio);
            if (foco == item_foco) attroff(A_REVERSE | A_BOLD);
        }

        mvhline(alto - 2, 1, ACS_HLINE, ancho - 2);
        mvprintw(alto - 1, 2, al_elegir != NULL
                 ? "Flechas arriba/abajo | ENTER: agregar al carrito | ESC salir"
                 : "Flechas arriba/abajo | ENTER seleccionar | ESC salir");
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
                    filtrado = 1;
                    foco = 2;          /* primera cancion del resultado */
                    scroll = 0;
                }
            } else if (filtrado && foco == 1) {
                /* Volver a la lista original, sin filtro */
                total = total_completo;
                memcpy(lista, catalogo, (size_t)total * sizeof(CancionUI));
                snprintf(filtro, sizeof(filtro), "Todas");
                filtrado = 0;
                foco = 0;
                scroll = 0;
            } else if (al_elegir != NULL) {
                al_elegir(&lista[foco - base]);
            }
        } else if (ch == TECLA_ESC) {
            return;
        }
    }
}