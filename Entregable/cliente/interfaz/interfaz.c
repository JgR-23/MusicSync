#include <ncurses.h>
#include <locale.h>
#include <string.h>

#include "interfaz.h"

/* ------------------------------------------------------------------ */
/*  Inicializacion                                                    */
/* ------------------------------------------------------------------ */

void inicializar_interfaz(void){
    setlocale(LC_ALL, "");  // Soporte de acentos / UTF-8

    initscr();              // Inicializar ncurses
    cbreak();               // Leer teclas sin esperar Enter
    noecho();               // No mostrar automaticamente las teclas
    keypad(stdscr, TRUE);   // Habilitar flechas y otras teclas especiales
    curs_set(0);            // Ocultar el cursor
    set_escdelay(25);       // ESC responde rapido

    if (has_colors()) {
        start_color();

        init_pair(PAR_SELECCION, COLOR_BLACK, COLOR_CYAN);
        init_pair(PAR_TITULO,    COLOR_CYAN,  COLOR_BLACK);
        init_pair(PAR_NORMAL,    COLOR_WHITE, COLOR_BLACK);
        init_pair(PAR_ERROR,     COLOR_RED,   COLOR_BLACK);
        init_pair(PAR_EXITO,     COLOR_GREEN, COLOR_BLACK);
        init_pair(PAR_AYUDA,     COLOR_WHITE, COLOR_BLUE);
    }
}

void finalizar_interfaz(void){
    endwin(); // Restaurar la terminal
}

/* ------------------------------------------------------------------ */
/*  Utilidades de dibujo                                              */
/* ------------------------------------------------------------------ */

int atributo_seleccion(void){
    if(has_colors()){
        return COLOR_PAIR(PAR_SELECCION) | A_BOLD;
    }
    return A_REVERSE;
}

void centrar_texto(int y, int x, int ancho, const char *texto){
    int largo = (int)strlen(texto);
    int inicio = x;

    if (largo < ancho){
        inicio = x + (ancho - largo) / 2;
    }

    mvprintw(y, inicio, "%s", texto);
}

void dibujar_marco(int y, int x, int alto, int ancho, const char *titulo){
    attron(COLOR_PAIR(PAR_TITULO));

    mvaddch(y, x, ACS_ULCORNER);
    mvhline(y, x + 1, ACS_HLINE, ancho - 2);
    mvaddch(y, x + ancho - 1, ACS_URCORNER);

    mvvline(y + 1, x, ACS_VLINE, alto - 2);
    mvvline(y + 1, x + ancho - 1, ACS_VLINE, alto - 2);

    mvaddch(y + alto - 1, x, ACS_LLCORNER);
    mvhline(y + alto - 1, x + 1, ACS_HLINE, ancho - 2);
    mvaddch(y + alto - 1, x + ancho - 1, ACS_LRCORNER);

    if(titulo != NULL && titulo[0] != '\0'){
        attron(A_BOLD);
        mvprintw(y, x + 2, " %s ", titulo);
        attroff(A_BOLD);
    }

    attroff(COLOR_PAIR(PAR_TITULO));
}

void dibujar_separador(int y, int x, int ancho){
    attron(COLOR_PAIR(PAR_TITULO));
    mvaddch(y, x, ACS_LTEE);
    mvhline(y, x + 1, ACS_HLINE, ancho - 2);
    mvaddch(y, x + ancho - 1, ACS_RTEE);
    attroff(COLOR_PAIR(PAR_TITULO));
}

void dibujar_barra_estado(const char *texto){
    int filas, columnas;
    int atributo = has_colors() ? COLOR_PAIR(PAR_AYUDA) : A_REVERSE;

    getmaxyx(stdscr, filas, columnas);

    attron(atributo);
    mvhline(filas - 1, 0, ' ', columnas);
    mvprintw(filas - 1, 1, "%s", texto);
    attroff(atributo);
}

void mostrar_mensaje(const char *mensaje){
    int filas;

    getmaxyx(stdscr, filas, COLS);

    move(filas - 2, 0);
    clrtoeol();

    attron(COLOR_PAIR(PAR_ERROR) | A_BOLD);
    mvprintw(filas - 2, 2, "%s", mensaje);
    attroff(COLOR_PAIR(PAR_ERROR) | A_BOLD);

    refresh();
}

/* ------------------------------------------------------------------ */
/*  Ventanas emergentes                                               */
/* ------------------------------------------------------------------ */

/* Dibuja una caja centrada y limpia el area que ocupa */
static void caja_centrada(int alto, int ancho, const char *titulo, int *y, int *x){
    int filas, columnas, r;

    getmaxyx(stdscr, filas, columnas);

    if(ancho > columnas - 2){
        ancho = columnas - 2;
    }

    *y = (filas - alto) / 2;
    *x = (columnas - ancho) / 2;

    for(r = 0; r < alto; r++){
        mvhline(*y + r, *x, ' ', ancho);
    }

    dibujar_marco(*y, *x, alto, ancho, titulo);
}

static int ancho_para(const char *texto, int minimo){
    int ancho = (int)strlen(texto) + 6;
    return ancho < minimo ? minimo : ancho;
}

void mostrar_aviso(const char *mensaje, int tipo){
    int y, x;
    int ancho = ancho_para(mensaje, 30);
    int filas, columnas;

    getmaxyx(stdscr, filas, columnas);
    (void)filas;
    if (ancho > columnas - 2) {
        ancho = columnas - 2;
    }

    caja_centrada(6, ancho, tipo == PAR_EXITO ? "Listo" : "Aviso", &y, &x);

    attron(COLOR_PAIR(tipo) | A_BOLD);
    centrar_texto(y + 2, x, ancho, mensaje);
    attroff(COLOR_PAIR(tipo) | A_BOLD);

    attron(A_DIM);
    centrar_texto(y + 4, x, ancho, "ENTER o ESC para continuar");
    attroff(A_DIM);

    refresh();

    while(1){
        int tecla = getch();

        if(tecla == '\n' || tecla == '\r' || tecla == KEY_ENTER || tecla == 27) {
            break;
        }
    }
}

int confirmar(const char *pregunta){
    int y, x, tecla;
    int ancho = ancho_para(pregunta, 34);
    int filas, columnas;

    getmaxyx(stdscr, filas, columnas);
    (void)filas;
    if(ancho > columnas - 2){
        ancho = columnas - 2;
    }

    const char *opciones[2] = { "Si", "No" };
    int seleccion = 1;      // Por seguridad, "No" queda marcado al inicio
    int i;

    caja_centrada(8, ancho, "Confirmar", &y, &x);

    attron(A_BOLD);
    centrar_texto(y + 2, x, ancho, pregunta);
    attroff(A_BOLD);

    while(1){
        for(i = 0; i < 2; i++){
            if(i == seleccion){
                attron(atributo_seleccion());
            }
            mvprintw(y + 4 + i, x + (ancho - 12) / 2, "  %-8s  ", opciones[i]);
            if(i == seleccion){
                attroff(atributo_seleccion());
            }
        }

        attron(A_DIM);
        centrar_texto(y + 6, x, ancho, "ENTER: elegir   ESC: cancelar");
        attroff(A_DIM);

        refresh();

        tecla = getch();

        if(tecla == KEY_UP || tecla == KEY_DOWN){
            seleccion = !seleccion;
        }
        else if(tecla == '\n' || tecla == '\r' || tecla == KEY_ENTER){
            return seleccion == 0;
        }
        else if(tecla == 27){
            return 0;
        }
    }
}

int pedir_texto(const char *titulo, const char *etiqueta, char *destino, int max){
    int y, x, tecla;
    int largo = 0;
    int ancho = 46;
    int filas, columnas;
    int campo_w, limite;

    getmaxyx(stdscr, filas, columnas);
    (void)filas;
    if(ancho > columnas - 2){
        ancho = columnas - 2;
    }

    campo_w = ancho - 6;
    limite = max - 1;
    if(limite > campo_w - 1){
        limite = campo_w - 1;
    }

    destino[0] = '\0';

    caja_centrada(8, ancho, titulo, &y, &x);

    mvprintw(y + 2, x + 3, "%s", etiqueta);

    attron(A_DIM);
    centrar_texto(y + 6, x, ancho, "ENTER: aceptar   ESC: cancelar");
    attroff(A_DIM);

    curs_set(1);

    while(1){
        attron(A_UNDERLINE | A_BOLD);
        mvprintw(y + 4, x + 3, "%-*s", campo_w, destino);
        attroff(A_UNDERLINE | A_BOLD);

        move(y + 4, x + 3 + largo);
        refresh();

        tecla = getch();

        if(tecla == '\n' || tecla == '\r' || tecla == KEY_ENTER){
            curs_set(0);
            return 1;
        }

        if(tecla == 27){
            curs_set(0);
            destino[0] = '\0';
            return 0;
        }

        if(tecla == KEY_BACKSPACE || tecla == 127 || tecla == 8){
            if(largo > 0){
                largo--;
                destino[largo] = '\0';
            }
            continue;
        }

        if(tecla >= 32 && tecla <= 255 && tecla != 127 && largo < limite){
            destino[largo++] = (char)tecla;
            destino[largo] = '\0';
        }
    }
}