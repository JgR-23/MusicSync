#include "vista_menu_inicio.h"
#include <ncurses.h>
#include <string.h>

#define PAR_FONDO     1
#define PAR_SELECCION 2

static void configurar_colores_menu(void) {
    if (!has_colors()) return;

    if (COLORS >= 256) {
        // 231 = blanco puro, 63 = azul indigo (#5f5fff), 75 = celeste (#5fafff)
        init_pair(PAR_FONDO,     231, 63);
        init_pair(PAR_SELECCION, 231, 75);
    } else {
        init_pair(PAR_FONDO,     COLOR_WHITE, COLOR_BLUE);
        init_pair(PAR_SELECCION, COLOR_WHITE, COLOR_CYAN);
    }
}

static void dibujar_menu_inicio(WINDOW *win, int seleccion) {
    const char *titulo = "Menu de inicio";
    const char *opciones[] = {
        "Biblioteca",
        "Gestion de playlist"
    };
    const int num_opciones = 2;
    const int ancho_barra = 24;

    int filas, cols;
    getmaxyx(win, filas, cols);

    werase(win);
    wbkgd(win, COLOR_PAIR(PAR_FONDO));

    // 1. Titulo centrado en la parte superior
    int fila_titulo = filas / 6;
    if (fila_titulo < 2) fila_titulo = 2;
    int col_titulo = (cols - (int)strlen(titulo)) / 2;
    if (col_titulo < 0) col_titulo = 0;

    wattron(win, COLOR_PAIR(PAR_FONDO) | A_BOLD);
    mvwprintw(win, fila_titulo, col_titulo, "%s", titulo);
    wattroff(win, A_BOLD);

    // 2. Opciones centradas con barra de seleccion
    int fila_base = filas / 2 - 2;
    int col_menu = (cols - ancho_barra) / 2;
    if (col_menu < 1) col_menu = 1;

    for (int i = 0; i < num_opciones; i++) {
        int fila_actual = fila_base + (i * 2); // Deja una linea vacia entre opciones
        if (i == seleccion) {
            wattron(win, COLOR_PAIR(PAR_SELECCION));
            mvwprintw(win, fila_actual, col_menu, "  %-*s", ancho_barra - 2, opciones[i]);
            wattroff(win, COLOR_PAIR(PAR_SELECCION));
        } else {
            wattron(win, COLOR_PAIR(PAR_FONDO));
            mvwprintw(win, fila_actual, col_menu, "  %-*s", ancho_barra - 2, opciones[i]);
        }
    }

    // 3. Instrucciones en la esquina inferior izquierda
    int fila_pie = filas - 4;
    if (fila_pie > fila_base + 4) {
        wattron(win, COLOR_PAIR(PAR_FONDO));
        mvwprintw(win, fila_pie,     3, "Desplazamiento con flechas hacia arriba y abajo");
        mvwprintw(win, fila_pie + 1, 3, "Presione ENTER para seleccionar");
        mvwprintw(win, fila_pie + 2, 3, "Presione ESC para cerrar sesion");
    }

    wrefresh(win);
}

OpcionMenuInicio mostrar_menu_inicio(void) {
    configurar_colores_menu();

    WINDOW *win = newwin(0, 0, 0, 0);
    keypad(win, TRUE);

    int seleccion = 0;
    const int num_opciones = 2;

    while (1) {
        dibujar_menu_inicio(win, seleccion);

        int ch = wgetch(win);
        switch (ch) {
            case KEY_UP:
                seleccion = (seleccion - 1 + num_opciones) % num_opciones;
                break;

            case KEY_DOWN:
                seleccion = (seleccion + 1) % num_opciones;
                break;

            case '\n':
            case '\r':
            case KEY_ENTER:
                delwin(win);
                return (seleccion == 0) ? MENU_BIBLIOTECA : MENU_GESTION_PLAYLIST;

            case 27: // Tecla ESC
                delwin(win);
                return MENU_SALIR;

            case KEY_RESIZE:
                wresize(win, LINES, COLS);
                break;

            default:
                break;
        }
    }
}
