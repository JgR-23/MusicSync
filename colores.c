#include "colores.h"

#include <ncurses.h>

#include "interfaz.h"
#include "ui_auth.h"

void colores_aplicar(ModoColores modo){
    if(modo == COLORES_AUTH){
        colores();               /* pares de ui_auth.c */
        bkgd(COLOR_PAIR(1));
    }
    else{
        configurar_colores_interfaz();   /* pares de interfaz.c */
        bkgd(' ');
    }

    clear();
}
