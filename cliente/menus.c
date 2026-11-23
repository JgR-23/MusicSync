#include <string.h>

#include "interfaz.h"
#include "menus.h"

static int menu_core(const char *const banner[], int nb,
                     const char *titulo, const char *subtitulo,
                     const char *const opciones[], int n, int inicial)
{
    int seleccion = (inicial >= 0 && inicial < n) ? inicial : 0;
    int inicio = 0;

    for (;;) {
        int filas, columnas, visibles, max_visibles, alto, ancho, total;
        int y0, y, x, i, tecla;
        int ancho_texto;

        getmaxyx(stdscr, filas, columnas);

        max_visibles = filas - nb - 10;
        if (max_visibles < 3) {
            max_visibles = 3;
        }
        visibles = n < max_visibles ? n : max_visibles;

        ancho = 46;
        for (i = 0; i < n; i++) {
            int largo = (int)strlen(opciones[i]) + 10;
            if (largo > ancho) {
                ancho = largo;
            }
        }
        if (ancho > columnas - 2) {
            ancho = columnas - 2;
        }
        alto = visibles + 6;
        total = nb + (nb > 0 ? 1 : 0) + alto;
        y0 = (filas - total) / 2;
        if (y0 < 0) {
            y0 = 0;
        }
        y = y0 + nb + (nb > 0 ? 1 : 0);
        x = (columnas - ancho) / 2;
        ancho_texto = ancho - 8;

        if (seleccion < inicio) {
            inicio = seleccion;
        }
        if (seleccion >= inicio + visibles) {
            inicio = seleccion - visibles + 1;
        }

        erase();

        attron(COLOR_PAIR(PAR_TITULO) | A_BOLD);
        for (i = 0; i < nb; i++) {
            centrar_texto(y0 + i, 0, columnas, banner[i]);
        }
        attroff(COLOR_PAIR(PAR_TITULO) | A_BOLD);

        dibujar_marco(y, x, alto, ancho, titulo);

        attron(A_DIM);
        centrar_texto(y + 1, x, ancho, subtitulo);
        attroff(A_DIM);
        dibujar_separador(y + 2, x, ancho);

        for (i = 0; i < visibles; i++) {
            int idx = inicio + i;

            if (idx == seleccion) {
                attron(atributo_seleccion());
            }
            mvprintw(y + 4 + i, x + 3, " %-*.*s ", ancho_texto, ancho_texto, opciones[idx]);
            if (idx == seleccion) {
                attroff(atributo_seleccion());
            }
        }
        if (inicio > 0) {
            mvaddch(y + 4, x + ancho - 2, '^');
        }
        if (inicio + visibles < n) {
            mvaddch(y + 3 + visibles, x + ancho - 2, 'v');
        }

        dibujar_barra_estado("Flechas: mover   ENTER: elegir   ESC: salir");
        refresh();

        tecla = getch();

        if (tecla == KEY_UP) {
            seleccion = (seleccion - 1 + n) % n;
        } else if (tecla == KEY_DOWN) {
            seleccion = (seleccion + 1) % n;
        } else if (tecla == '\n' || tecla == '\r' || tecla == KEY_ENTER) {
            return seleccion;
        } else if (tecla == 27) {
            return -1;
        }
    }
}

int menu_opciones(const char *titulo, const char *subtitulo,
                  const char *const opciones[], int n)
{
    return menu_core(NULL, 0, titulo, subtitulo, opciones, n, 0);
}

int menu_opciones_desde(const char *titulo, const char *subtitulo,
                        const char *const opciones[], int n, int inicial)
{
    return menu_core(NULL, 0, titulo, subtitulo, opciones, n, inicial);
}

int pantalla_inicio(void)
{
    static const char *const banner[] = {
        " __  __           _      ____                   ",
        "|  \\/  |_   _ ___(_) ___/ ___| _   _ _ __   ___ ",
        "| |\\/| | | | / __| |/ __\\___ \\| | | | '_ \\ / __|",
        "| |  | | |_| \\__ \\ | (__ ___) | |_| | | | | (__ ",
        "|_|  |_|\\__,_|___/_|\\___|____/ \\__, |_| |_|\\___|",
        "                               |___/            ",
        "",
        "Tu musica, sincronizada"
    };
    static const char *const opciones[] = {
        "Iniciar sesion",
        "Registrarse",
        "Salir del sistema"
    };

    return menu_core(banner, 8, "Bienvenido", "Elige una opcion", opciones, 3, 0);
}
