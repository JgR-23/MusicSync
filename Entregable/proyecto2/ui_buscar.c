#include "ui_buscar.h"

#include <ncurses.h>
#include <stdio.h>

bool pantalla_buscar_por(const CancionUI *catalogo, int total_catalogo,
                       CancionUI *salida, int *total_salida,
                       char *etiqueta, size_t etiqueta_len) {
    const char *opciones[] = {
        "Nombre",
        "Artista",
        "Album",
        "Genero",
        "Cancelar"
    };
    const int num_ops = 5;
    int sel = 0;

    while (1) {
        int alto, ancho;
        getmaxyx(stdscr, alto, ancho);
        erase();
        box(stdscr, 0, 0);

        mvprintw(1, 2, "Buscar por");
        mvhline(2, 1, ACS_HLINE, ancho - 2);

        for (int i = 0; i < num_ops; i++) {
            if (i == sel) attron(A_REVERSE | A_BOLD);
            mvprintw(4 + i * 2, 4, "%-14s", opciones[i]);
            if (i == sel) attroff(A_REVERSE | A_BOLD);
        }

        mvhline(alto - 2, 1, ACS_HLINE, ancho - 2);
        mvprintw(alto - 1, 2, "Flechas arriba/abajo | ENTER seleccionar | ESC cancelar");
        refresh();

        int ch = getch();
        if (ch == KEY_UP) {
            sel = (sel - 1 + num_ops) % num_ops;
        } else if (ch == KEY_DOWN) {
            sel = (sel + 1) % num_ops;
        } else if (ch == TECLA_ESC || ((ch == '\n' || ch == KEY_ENTER) && sel == 4)) {
            return false;
        } else if (ch == '\n' || ch == KEY_ENTER) {
            CriterioBusqueda crit = (CriterioBusqueda)sel;
            char raw[MAX_TEXTO * 2] = {0};
            char query[MAX_TEXTO] = {0};

            erase();
            box(stdscr, 0, 0);
            mvprintw(1, 2, "Buscar por -> %s", nombre_criterio(crit));
            mvhline(2, 1, ACS_HLINE, ancho - 2);
            mvprintw(4, 4, "Ingrese termino de busqueda:");
            mvhline(6, 4, '_', MAX_TEXTO - 1);
            move(6, 4);

            echo();
            curs_set(1);
            getnstr(raw, (int)sizeof(raw) - 1);
            noecho();
            curs_set(0);

            if (!validar_texto(raw, query, sizeof(query))) {
                mostrar_mensaje_ui("Texto invalido o vacio.");
                continue;
            }

            int n = 0;
            for (int i = 0; i < total_catalogo && n < MAX_CANCIONES; i++) {
                const char *campo = catalogo[i].titulo;
                if (crit == CRITERIO_ARTISTA) campo = catalogo[i].artista;
                else if (crit == CRITERIO_ALBUM) campo = catalogo[i].album;
                else if (crit == CRITERIO_GENERO) campo = catalogo[i].genero;

                if (contiene_texto(campo, query)) {
                    salida[n++] = catalogo[i];
                }
            }

            if (n == 0) {
                mostrar_mensaje_ui("Sin resultados para esa busqueda.");
                continue;
            }

            *total_salida = n;
            snprintf(etiqueta, etiqueta_len, "%s: %s", nombre_criterio(crit), query);
            return true;
        }
    }
}
