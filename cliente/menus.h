#ifndef MENUS_H
#define MENUS_H

/* Pantalla de inicio con banner: 0 = Iniciar sesion, 1 = Registrarse, 2 = Salir, -1 = ESC */
int pantalla_inicio(void);

/* Menu generico dentro de un marco. Devuelve la opcion elegida o -1 con ESC */
int menu_opciones(const char *titulo, const char *subtitulo,
                  const char *const opciones[], int n);

/* Igual que menu_opciones, pero empieza con la opcion "inicial" marcada */
int menu_opciones_desde(const char *titulo, const char *subtitulo,
                        const char *const opciones[], int n, int inicial);

#endif
