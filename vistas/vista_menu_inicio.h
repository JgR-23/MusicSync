#ifndef VISTA_MENU_INICIO_H
#define VISTA_MENU_INICIO_H

typedef enum {
    MENU_SALIR = 0,
    MENU_BIBLIOTECA = 1,
    MENU_GESTION_PLAYLIST = 2
} OpcionMenuInicio;

/*
 * Muestra el menu de inicio en pantalla completa.
 * Devuelve MENU_BIBLIOTECA, MENU_GESTION_PLAYLIST o MENU_SALIR (si presiona ESC).
 */
OpcionMenuInicio mostrar_menu_inicio(void);

#endif
