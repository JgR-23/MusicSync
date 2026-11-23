#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "interfaz.h"
#include "ui_form.h"

/* ------------------------------------------------------------------ */
/*  Formulario generico (login, registro, perfil, administrador)      */
/* ------------------------------------------------------------------ */

static void dibujar_campo(int fila, int x, int ancho_campo,
                          const Campo *c, int activo)
{
    char visible[80];
    int len = (int)strlen(c->valor);
    int util = ancho_campo - 1;
    int inicio = 0;
    int i, k = 0;
    int atributo = activo ? atributo_seleccion() : (int)A_UNDERLINE;

    if (util > 78) {
        util = 78;
    }
    if (len > util) {
        inicio = len - util;
    }
    for (i = inicio; i < len && k < util; i++) {
        visible[k++] = c->oculto ? '*' : c->valor[i];
    }
    if (activo && len < c->max && k < util) {
        visible[k++] = '_';
    }
    visible[k] = '\0';

    attron(atributo);
    mvprintw(fila, x, "%-*s", ancho_campo, visible);
    attroff(atributo);
}

/* Botones al final: [ boton ] y [ Volver ]. ESC tambien vuelve.
 * Devuelve 1 si se envio el formulario, 0 si se volvio atras. */
int ejecutar_formulario(const char *titulo, const char *subtitulo,
                      Campo *campos, int n, const char *boton,
                      Validador validar, int activo_inicial)
{
    int activo = activo_inicial;
    char error[100] = "";

    for (;;) {
        int filas, columnas, ancho = 64, alto = n * 2 + 9;
        int y, x, i, tecla, ancho_campo;
        char texto_boton[40];

        getmaxyx(stdscr, filas, columnas);
        if (ancho > columnas - 2) {
            ancho = columnas - 2;
        }
        y = (filas - alto) / 2;
        if (y < 0) {
            y = 0;
        }
        x = (columnas - ancho) / 2;
        ancho_campo = ancho - 29;
        if (ancho_campo < 8) {
            ancho_campo = 8;
        }

        erase();
        dibujar_marco(y, x, alto, ancho, titulo);
        attron(A_DIM);
        centrar_texto(y + 1, x, ancho, subtitulo);
        attroff(A_DIM);
        dibujar_separador(y + 2, x, ancho);

        for (i = 0; i < n; i++) {
            mvprintw(y + 4 + i * 2, x + 3, "%-22s", campos[i].etiqueta);
            dibujar_campo(y + 4 + i * 2, x + 26, ancho_campo, &campos[i], activo == i);
        }

        snprintf(texto_boton, sizeof(texto_boton), "  [ %s ]  ", boton);
        {
            const char *texto_volver = "  [ Volver ]  ";
            int largo1 = (int)strlen(texto_boton);
            int largo2 = (int)strlen(texto_volver);
            int xb = x + (ancho - (largo1 + 3 + largo2)) / 2;

            if (activo == n) {
                attron(atributo_seleccion());
            }
            mvprintw(y + 5 + n * 2, xb, "%s", texto_boton);
            if (activo == n) {
                attroff(atributo_seleccion());
            }

            if (activo == n + 1) {
                attron(atributo_seleccion());
            }
            mvprintw(y + 5 + n * 2, xb + largo1 + 3, "%s", texto_volver);
            if (activo == n + 1) {
                attroff(atributo_seleccion());
            }
        }

        if (error[0] != '\0') {
            attron(COLOR_PAIR(PAR_ERROR) | A_BOLD);
            centrar_texto(y + alto - 2, x, ancho, error);
            attroff(COLOR_PAIR(PAR_ERROR) | A_BOLD);
        }

        dibujar_barra_estado("Flechas/TAB: mover   ENTER: elegir/siguiente   ESC: volver");
        refresh();

        tecla = getch();
        error[0] = '\0';

        if (tecla == KEY_UP) {
            activo = (activo - 1 + n + 2) % (n + 2);
        } else if (tecla == KEY_DOWN || tecla == '\t') {
            activo = (activo + 1) % (n + 2);
        } else if ((tecla == KEY_LEFT || tecla == KEY_RIGHT) && activo >= n) {
            activo = (activo == n) ? n + 1 : n;
        } else if (tecla == 27) {
            return 0;
        } else if (tecla == '\n' || tecla == '\r' || tecla == KEY_ENTER) {
            if (activo < n) {
                activo++;
            } else if (activo == n + 1) {
                return 0;
            } else if (validar(campos, error, sizeof(error))) {
                return 1;
            }
        } else if (activo < n) {
            char *valor = campos[activo].valor;
            size_t len = strlen(valor);

            if (tecla == KEY_BACKSPACE || tecla == 127 || tecla == 8) {
                if (len > 0) {
                    valor[len - 1] = '\0';
                }
            } else if (tecla >= 32 && tecla <= 126 && (int)len < campos[activo].max) {
                valor[len] = (char)tecla;
                valor[len + 1] = '\0';
            }
        }
    }
}
