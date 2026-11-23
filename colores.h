#ifndef COLORES_H_
#define COLORES_H_

/*
 * Cada grupo de vistas define sus propios pares de color y los numeros se
 * repiten (1, 2, ...). Por eso hay que reaplicarlos cada vez que se cambia
 * de un grupo de vistas a otro.
 */
typedef enum {
    COLORES_AUTH,       /* login y registro (ui_auth) */
    COLORES_INTERFAZ    /* biblioteca, buscar y playlists (interfaz.h / ui_*) */
} ModoColores;

/* Redefine los pares de color, fija el fondo y limpia la pantalla */
void colores_aplicar(ModoColores modo);

#endif // COLORES_H_
