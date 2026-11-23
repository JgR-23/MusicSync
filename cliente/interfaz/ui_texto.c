#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "interfaz.h"
#include "ui_texto.h"

#define MAX_LINEAS 600
#define MAX_TEXTO_VISOR 20000

void mostrar_texto(const char *titulo, const char *texto)
{
    static char copia[MAX_TEXTO_VISOR];
    const char *lineas[MAX_LINEAS];
    int total = 0;
    int inicio = 0;
    char *p;

    snprintf(copia, sizeof(copia), "%s", texto);

    /* Separa el texto en lineas sin copiar cada una */
    p = copia;
    while (*p != '\0' && total < MAX_LINEAS) {
        lineas[total++] = p;
        p = strchr(p, '\n');
        if (p == NULL) {
            break;
        }
        *p++ = '\0';
    }

    for (;;) {
        int filas, columnas, ancho, alto, visibles, i, tecla;
        int x0 = 2;
        char estado[128];

        getmaxyx(stdscr, filas, columnas);
        ancho = columnas - 4;
        alto = filas - 2;
        visibles = alto - 2;
        if (visibles < 1) {
            visibles = 1;
        }
        if (inicio > total - visibles) {
            inicio = total - visibles;
        }
        if (inicio < 0) {
            inicio = 0;
        }

        erase();
        dibujar_marco(0, x0, alto, ancho, titulo);

        for (i = 0; i < visibles && inicio + i < total; i++) {
            mvprintw(1 + i, x0 + 2, "%.*s", ancho - 4, lineas[inicio + i]);
        }
        if (inicio > 0) {
            mvaddch(1, x0 + ancho - 2, '^');
        }
        if (inicio + visibles < total) {
            mvaddch(visibles, x0 + ancho - 2, 'v');
        }

        snprintf(estado, sizeof(estado),
                 "Flechas/AvPag/RePag: mover   ESC: volver   (lineas %d-%d de %d)",
                 total > 0 ? inicio + 1 : 0,
                 inicio + visibles < total ? inicio + visibles : total, total);
        dibujar_barra_estado(estado);
        refresh();

        tecla = getch();
        if (tecla == 27 || tecla == '\n' || tecla == '\r' || tecla == KEY_ENTER || tecla == 'q') {
            return;
        } else if (tecla == KEY_UP) {
            inicio--;
        } else if (tecla == KEY_DOWN) {
            inicio++;
        } else if (tecla == KEY_NPAGE) {
            inicio += visibles;
        } else if (tecla == KEY_PPAGE) {
            inicio -= visibles;
        } else if (tecla == KEY_HOME) {
            inicio = 0;
        } else if (tecla == KEY_END) {
            inicio = total;
        }
    }
}
