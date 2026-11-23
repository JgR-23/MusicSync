#include "vista_menu_inicio.h"
#include <ncurses.h>
#include <stdio.h>

int main(void) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    set_escdelay(25); // Evita el retraso de 1 segundo al presionar ESC

    if (has_colors()) {
        start_color();
    }

    OpcionMenuInicio opcion = mostrar_menu_inicio();

    endwin();

    if (opcion == MENU_BIBLIOTECA) {
        printf("Seleccionaste: Biblioteca\n");
    } else if (opcion == MENU_GESTION_PLAYLIST) {
        printf("Seleccionaste: Gestion de playlist\n");
    } else {
        printf("Saliste del menu (ESC)\n");
    }

    return 0;
}
