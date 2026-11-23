#ifndef INTERFAZ_H
#define INTERFAZ_H

#include <ncurses.h>

/* Pares de color */
#define PAR_SELECCION 1
#define PAR_TITULO    2
#define PAR_NORMAL    3
#define PAR_ERROR     4
#define PAR_EXITO     5
#define PAR_AYUDA     6

void inicializar_interfaz(void);
void finalizar_interfaz(void);

/* Mensaje rapido en la parte inferior (no bloquea) */
void mostrar_mensaje(const char *mensaje);

/* Utilidades de dibujo */
int  atributo_seleccion(void);
void centrar_texto(int y, int x, int ancho, const char *texto);
void dibujar_marco(int y, int x, int alto, int ancho, const char *titulo);
void dibujar_separador(int y, int x, int ancho);
void dibujar_barra_estado(const char *texto);

/* Ventanas emergentes */
void mostrar_aviso(const char *mensaje, int tipo);   /* tipo: PAR_ERROR o PAR_EXITO */
int  confirmar(const char *pregunta);                /* 1 = si, 0 = no */
int  pedir_texto(const char *titulo, const char *etiqueta, char *destino, int max);            /* 1 = aceptado, 0 = cancelado */

#endif