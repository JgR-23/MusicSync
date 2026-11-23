#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "playlists.h"
#include "interfaz.h"
#include "ipc_cliente.h"

#define MAX_PLAYLISTS 20
#define MAX_NOMBRE 50
#define MAX_CANCIONES 20

#define MIN_FILAS 20
#define MIN_COLUMNAS 50

typedef struct{
    char nombre[80];
    char artista[80];
    char album[80];
    char genero[40];
    char duracion[10];
    int id;
}Cancion;

typedef struct{
    char nombre[MAX_NOMBRE];
    Cancion canciones[MAX_CANCIONES];
    int cantidad;
}Playlist;

/* -------------------------------------------------- */
/* Datos (se piden al servidor)                       */
/* -------------------------------------------------- */

/* Carga las playlists del usuario. Devuelve 1 si salio bien */
static int cargar_playlists(Playlist playlists[], int *cantidad){
    RegPlaylist registros[IPC_MAX_PLAYLISTS];
    int n, i;

    *cantidad = 0;

    n = ipc_listar_playlists(registros, IPC_MAX_PLAYLISTS);
    if(n < 0){
        mostrar_aviso(ipc_ultimo_error(), PAR_ERROR);
        return 0;
    }

    if(n > MAX_PLAYLISTS){
        n = MAX_PLAYLISTS;
    }

    for(i = 0; i < n; i++){
        snprintf(playlists[i].nombre, MAX_NOMBRE, "%s", registros[i].nombre);
        playlists[i].cantidad = registros[i].cantidad;   /* las canciones se piden al abrir la playlist */
    }

    *cantidad = n;
    return 1;
}

/* Carga las canciones de una playlist. Devuelve 1 si salio bien */
static int cargar_canciones(Playlist *playlist){
    RegCancion registros[IPC_MAX_CANCIONES_PLAYLIST];
    int n, i;

    n = ipc_listar_canciones_playlist(playlist->nombre, registros, IPC_MAX_CANCIONES_PLAYLIST);
    if(n < 0){
        mostrar_aviso(ipc_ultimo_error(), PAR_ERROR);
        return 0;
    }

    if(n > MAX_CANCIONES){
        n = MAX_CANCIONES;
    }

    for(i = 0; i < n; i++){
        Cancion *c = &playlist->canciones[i];

        snprintf(c->nombre,   sizeof(c->nombre),   "%s", registros[i].titulo);
        snprintf(c->artista,  sizeof(c->artista),  "%s", registros[i].artista);
        snprintf(c->album,    sizeof(c->album),    "%s", registros[i].album);
        snprintf(c->genero,   sizeof(c->genero),   "%s", registros[i].genero);
        snprintf(c->duracion, sizeof(c->duracion), "%s", registros[i].duracion);
        c->id = registros[i].id;
    }

    playlist->cantidad = n;
    return 1;
}

/* -------------------------------------------------- */
/* Utilidades de dibujo compartidas                   */
/* -------------------------------------------------- */

/* Calcula el marco principal (siempre empieza en la fila 1) */
static void geometria(int filas, int columnas, int max_ancho, int *x0, int *ancho, int *alto){
    *ancho = columnas - 4;
    if(*ancho > max_ancho){
        *ancho = max_ancho;
    }

    *alto = filas - 4;
    *x0 = (columnas - *ancho) / 2;
}

/* Mantiene la seleccion visible dentro de la lista con scroll */
static int ajustar_scroll(int seleccion, int inicio, int cantidad, int visibles){
    if(seleccion < cantidad){
        if(seleccion < inicio){
            inicio = seleccion;
        }
        if(seleccion >= inicio + visibles){
            inicio = seleccion - visibles + 1;
        }
    }

    if(inicio > cantidad - visibles){
        inicio = cantidad - visibles;
    }
    if(inicio < 0){
        inicio = 0;
    }

    return inicio;
}

static void dibujar_indicadores_scroll(int x0, int ancho, int ly, int visibles, int inicio, int cantidad){
    if(inicio > 0){
        mvaddch(ly, x0 + ancho - 2, '^');
    }
    if(inicio + visibles < cantidad){
        mvaddch(ly + visibles - 1, x0 + ancho - 2, 'v');
    }
}

static void dibujar_titulo_y_contador(int y0, int x0, int ancho, const char *titulo, int cantidad, int maximo)
{
    attron(COLOR_PAIR(PAR_TITULO) | A_BOLD);
    centrar_texto(y0 + 1, x0, ancho, titulo);
    attroff(COLOR_PAIR(PAR_TITULO) | A_BOLD);

    attron(A_DIM);
    mvprintw(y0 + 1, x0 + ancho - 9, "%2d/%-2d", cantidad, maximo);
    attroff(A_DIM);

    dibujar_separador(y0 + 2, x0, ancho);
}

static void dibujar_acciones(const char *acciones[], int n, int y, int x0, int w, int cantidad, int seleccion, int deshabilitadas){
    int k;

    for(k = 0; k < n; k++){
        int activo = (!deshabilitadas && seleccion == cantidad + k);

        if(deshabilitadas){
            attron(A_DIM);
        }
        if(activo){
            attron(atributo_seleccion());
        }

        mvprintw(y + k, x0 + 2, " %-*.*s ", w, w, acciones[k]);

        if(activo){
            attroff(atributo_seleccion());
        }
        if(deshabilitadas){
            attroff(A_DIM);
        }
    }
}

/* Devuelve 1 si el usuario pidio salir con ESC */
static int aviso_terminal_pequena(void){
    erase();
    mvprintw(0, 0, "Aumenta el tamano de la terminal (min %dx%d).", MIN_COLUMNAS, MIN_FILAS);
    mvprintw(1, 0, "ESC para salir.");
    refresh();

    return getch() == 27;
}

/* -------------------------------------------------- */
/* Eliminar una cancion                               */
/* -------------------------------------------------- */

static void borrar_cancion(Playlist *playlist, int indice){
    int i;

    for(i = indice; i < playlist->cantidad - 1; i++){
        playlist->canciones[i] = playlist->canciones[i + 1];
    }

    playlist->cantidad--;
}

/* -------------------------------------------------- */
/* Informacion de una cancion                         */
/* -------------------------------------------------- */

static void mostrar_campo(int y, int x, int vw, const char *etiqueta, const char *valor){
    attron(COLOR_PAIR(PAR_TITULO) | A_BOLD);
    mvprintw(y, x + 3, "%-10s", etiqueta);
    attroff(COLOR_PAIR(PAR_TITULO) | A_BOLD);

    mvprintw(y, x + 14, "%.*s", vw, valor);
}

static void mostrar_informacion_cancion(Cancion *cancion){
    int filas, columnas;
    int alto = 12;
    int ancho, x0, y;
    int vw;
    char titulo[100];

    getmaxyx(stdscr, filas, columnas);

    ancho = columnas - 4;
    if(ancho > 60){
        ancho = 60;
    }

    x0 = (columnas - ancho) / 2;
    y = (filas - alto) / 2;
    vw = ancho - 17;

    erase();

    dibujar_marco(y, x0, alto, ancho, "Informacion de la cancion");

    snprintf(titulo, sizeof(titulo), "%.*s", ancho - 6, cancion->nombre);

    attron(COLOR_PAIR(PAR_TITULO) | A_BOLD);
    centrar_texto(y + 2, x0, ancho, titulo);
    attroff(COLOR_PAIR(PAR_TITULO) | A_BOLD);

    dibujar_separador(y + 3, x0, ancho);

    mostrar_campo(y + 5, x0, vw, "Artista",  cancion->artista);
    mostrar_campo(y + 6, x0, vw, "Album",    cancion->album);
    mostrar_campo(y + 7, x0, vw, "Genero",   cancion->genero);
    mostrar_campo(y + 8, x0, vw, "Duracion", cancion->duracion);

    dibujar_barra_estado("ENTER o ESC para regresar");
    refresh();

    while(1){
        int tecla = getch();

        if (tecla == '\n' || tecla == '\r' ||
            tecla == KEY_ENTER || tecla == 27) {
            break;
        }
    }
}

/* -------------------------------------------------- */
/* Pantalla de canciones de una playlist              */
/* -------------------------------------------------- */

static void dibujar_canciones(Playlist *playlist, int seleccion, int inicio, int modo_eliminar, int filas, int columnas){
    int ancho, alto, x0;
    int y0 = 1;
    int yb, ly, visibles, w, tw, aw;
    int fila, i;
    char titulo[MAX_NOMBRE + 4];
    const char *acciones[] = {
        "- Eliminar cancion",
        "< Regresar"
    };

    geometria(filas, columnas, 70, &x0, &ancho, &alto);

    yb = y0 + alto - 1;
    ly = y0 + 4;
    visibles = alto - 8;
    w = ancho - 6;
    aw = (ancho - 17) * 2 / 5;
    tw = ancho - 17 - aw;

    erase();

    snprintf(titulo, sizeof(titulo), "%.*s", ancho - 6, playlist->nombre);
    dibujar_marco(y0, x0, alto, ancho, titulo);

    dibujar_titulo_y_contador(y0, x0, ancho, modo_eliminar ? "ELIMINAR CANCION" : "CANCIONES", playlist->cantidad, MAX_CANCIONES);

    /* Encabezado de columnas */
    attron(A_DIM);
    mvprintw(y0 + 3, x0 + 2, " %-4s%-*.*s %-*.*s %5s ", "#", tw, tw, "Titulo", aw, aw, "Artista", "Dur.");
    attroff(A_DIM);

    if(playlist->cantidad == 0){
        attron(A_DIM);
        centrar_texto(ly + 1, x0, ancho, "Esta playlist todavia no tiene canciones.");
        attroff(A_DIM);
    }

    for(fila = 0; fila < visibles; fila++){
        i = inicio + fila;
        if(i >= playlist->cantidad){
            break;
        }

        if(i == seleccion){
            attron(atributo_seleccion());
        }

        mvprintw(ly + fila, x0 + 2, " %2d. %-*.*s %-*.*s %5s ", i + 1, tw, tw, playlist->canciones[i].nombre, aw, aw, playlist->canciones[i].artista, playlist->canciones[i].duracion);

        if(i == seleccion){
            attroff(atributo_seleccion());
        }
    }

    dibujar_indicadores_scroll(x0, ancho, ly, visibles, inicio, playlist->cantidad);

    dibujar_separador(yb - 3, x0, ancho);
    dibujar_acciones(acciones, 2, yb - 2, x0, w, playlist->cantidad, seleccion, modo_eliminar);

    dibujar_barra_estado(modo_eliminar  ? "Flechas:mover  ENTER:eliminar  ESC:cancelar" : "Flechas:mover  ENTER:elegir  ESC:volver");
}

static void mostrar_detalle(Playlist *playlist){
    int seleccion = 0;
    int inicio = 0;
    int modo_eliminar = 0;
    int total, visibles, tecla;
    int filas, columnas;
    char pregunta[120];

    /* Las canciones de la playlist se piden al servidor al abrirla */
    if(!cargar_canciones(playlist)){
        return;
    }

    while(1){

        getmaxyx(stdscr, filas, columnas);

        if(filas < MIN_FILAS || columnas < MIN_COLUMNAS){
            if(aviso_terminal_pequena()){
                return;
            }
            continue;
        }

        visibles = (filas - 4) - 8;

        if(modo_eliminar && playlist->cantidad == 0){
            modo_eliminar = 0;
            seleccion = 0;
        }

        total = modo_eliminar ? playlist->cantidad : playlist->cantidad + 2;

        if(seleccion >= total){
            seleccion = total - 1;
        }
        if(seleccion < 0){
            seleccion = 0;
        }

        inicio = ajustar_scroll(seleccion, inicio, playlist->cantidad, visibles);

        dibujar_canciones(playlist, seleccion, inicio, modo_eliminar, filas, columnas);
        refresh();

        tecla = getch();

        switch(tecla){

            case KEY_UP:
                seleccion = (seleccion - 1 + total) % total;
            break;

            case KEY_DOWN:
                seleccion = (seleccion + 1) % total;
            break;

            case '\n':
            case '\r':
            case KEY_ENTER:

                if(modo_eliminar){

                    snprintf(pregunta, sizeof(pregunta), "Eliminar \"%s\"?", playlist->canciones[seleccion].nombre);

                    if(confirmar(pregunta)){

                        if(ipc_quitar_cancion(playlist->nombre, playlist->canciones[seleccion].id) == 0){
                            borrar_cancion(playlist, seleccion);

                            modo_eliminar = 0;
                            seleccion = 0;

                            mostrar_aviso("Cancion eliminada.", PAR_EXITO);
                        }
                        else{
                            mostrar_aviso(ipc_ultimo_error(), PAR_ERROR);
                        }
                    }
                }

                else if(seleccion < playlist->cantidad){
                    mostrar_informacion_cancion(&playlist->canciones[seleccion]);
                }

                else if(seleccion == playlist->cantidad){

                    if(playlist->cantidad == 0){
                        mostrar_aviso("No hay canciones para eliminar.", PAR_ERROR);
                    }
                    else{
                        modo_eliminar = 1;
                        seleccion = 0;
                    }
                }

                else{
                    return;
                }

            break;

            case 27:

                if(modo_eliminar){
                    modo_eliminar = 0;
                    seleccion = 0;
                }
                else{
                    return;
                }

            break;

            default:
            break;
        }
    }
}

/* -------------------------------------------------- */
/* Eliminar una playlist                              */
/* -------------------------------------------------- */

static void borrar_playlist(Playlist playlists[], int *cantidad, int indice){
    int i;

    for(i = indice; i < *cantidad - 1; i++){
        playlists[i] = playlists[i + 1];
    }

    (*cantidad)--;
}

/* -------------------------------------------------- */
/* Menu principal de playlists                        */
/* -------------------------------------------------- */

static void dibujar_pantalla(Playlist playlists[], int cantidad, int seleccion, int inicio, int modo_eliminar, int filas, int columnas){
    int ancho, alto, x0;
    int y0 = 1;
    int yb, ly, visibles, w, nw;
    int fila, i;
    char cuenta[24];
    const char *acciones[] = {
        "+ Crear playlist",
        "- Eliminar playlist",
        "< Regresar"
    };

    geometria(filas, columnas, 65, &x0, &ancho, &alto);

    yb = y0 + alto - 1;
    ly = y0 + 4;
    visibles = alto - 9;
    w = ancho - 6;
    nw = ancho - 23;

    erase();

    dibujar_marco(y0, x0, alto, ancho, "Reproductor");

    dibujar_titulo_y_contador(y0, x0, ancho, modo_eliminar ? "ELIMINAR PLAYLIST" : "MIS PLAYLISTS", cantidad, MAX_PLAYLISTS);

    /* Encabezado de columnas */
    attron(A_DIM);
    mvprintw(y0 + 3, x0 + 2, " %-4s%-*.*s %12s ", "#", nw, nw, "Nombre", "Canciones");
    attroff(A_DIM);

    if(cantidad == 0){
        attron(A_DIM);
        centrar_texto(ly + 1, x0, ancho, "(Aun no tienes playlists)");
        attroff(A_DIM);
    }

    for(fila = 0; fila < visibles; fila++){
        i = inicio + fila;
        if(i >= cantidad){
            break;
        }

        snprintf(cuenta, sizeof(cuenta), "%d %s", playlists[i].cantidad, playlists[i].cantidad == 1 ? "cancion" : "canciones");

        if(i == seleccion){
            attron(atributo_seleccion());
        }

        mvprintw(ly + fila, x0 + 2, " %2d. %-*.*s %12s ", i + 1, nw, nw, playlists[i].nombre, cuenta);

        if(i == seleccion){
            attroff(atributo_seleccion());
        }
    }

    dibujar_indicadores_scroll(x0, ancho, ly, visibles, inicio, cantidad);

    dibujar_separador(yb - 4, x0, ancho);
    dibujar_acciones(acciones, 3, yb - 3, x0, w, cantidad, seleccion, modo_eliminar);

    dibujar_barra_estado(modo_eliminar ? "Flechas:mover  ENTER:eliminar  ESC:cancelar" : "Flechas:mover  ENTER:elegir  ESC:volver");
}

/* -------------------------------------------------- */
/* Funcion principal del modulo                       */
/* -------------------------------------------------- */

void mostrar_playlists(void){
    Playlist playlists[MAX_PLAYLISTS];

    int cantidad;
    int seleccion = 0;
    int inicio = 0;
    int modo_eliminar = 0;
    int filas, columnas;
    int visibles, total, tecla;
    int i, repetida;

    char nombre[MAX_NOMBRE];
    char pregunta[MAX_NOMBRE + 20];

    cargar_playlists(playlists, &cantidad);

    while(1){

        getmaxyx(stdscr, filas, columnas);

        if(filas < MIN_FILAS || columnas < MIN_COLUMNAS){
            if(aviso_terminal_pequena()){
                return;
            }
            continue;
        }

        visibles = (filas - 4) - 9;

        if(modo_eliminar && cantidad == 0){
            modo_eliminar = 0;
            seleccion = 0;
        }

        total = modo_eliminar ? cantidad : cantidad + 3;

        if(seleccion >= total){
            seleccion = total - 1;
        }
        if(seleccion < 0){
            seleccion = 0;
        }

        inicio = ajustar_scroll(seleccion, inicio, cantidad, visibles);

        dibujar_pantalla(playlists, cantidad, seleccion, inicio, modo_eliminar, filas, columnas);
        refresh();

        tecla = getch();

        switch(tecla){

            case KEY_UP:
                seleccion = (seleccion - 1 + total) % total;
            break;

            case KEY_DOWN:
                seleccion = (seleccion + 1) % total;
            break;

            case '\n':
            case '\r':
            case KEY_ENTER:

                if(modo_eliminar){

                    snprintf(pregunta, sizeof(pregunta), "Eliminar \"%s\"?", playlists[seleccion].nombre);

                    if(confirmar(pregunta)){

                        if(ipc_eliminar_playlist(playlists[seleccion].nombre) == 0){
                            borrar_playlist(playlists, &cantidad, seleccion);

                            modo_eliminar = 0;
                            seleccion = 0;

                            mostrar_aviso("Playlist eliminada.", PAR_EXITO);
                        }
                        else{
                            mostrar_aviso(ipc_ultimo_error(), PAR_ERROR);
                        }
                    }
                }

                else if(seleccion < cantidad){
                    mostrar_detalle(&playlists[seleccion]);
                }

                else if(seleccion == cantidad){

                    if(cantidad >= MAX_PLAYLISTS){
                        mostrar_aviso("Limite de playlists alcanzado.", PAR_ERROR);
                        break;
                    }

                    if(!pedir_texto("Nueva playlist", "Nombre:", nombre, MAX_NOMBRE) || nombre[0] == '\0'){
                        break;
                    }

                    repetida = 0;

                    for(i = 0; i < cantidad; i++){
                        if(strcmp(playlists[i].nombre, nombre) == 0){
                            repetida = 1;
                            break;
                        }
                    }

                    if(repetida){
                        mostrar_aviso("Ese nombre ya existe.", PAR_ERROR);
                    }
                    else if(ipc_crear_playlist(nombre) != 0){
                        mostrar_aviso(ipc_ultimo_error(), PAR_ERROR);
                    }
                    else{
                        /* Se vuelve a pedir la lista: el servidor es quien guarda el nombre final */
                        cargar_playlists(playlists, &cantidad);
                        seleccion = cantidad > 0 ? cantidad - 1 : 0;

                        mostrar_aviso("Playlist creada.", PAR_EXITO);
                    }
                }

                else if(seleccion == cantidad + 1){

                    if(cantidad == 0){
                        mostrar_aviso("No hay playlists para eliminar.", PAR_ERROR);
                    }
                    else{
                        modo_eliminar = 1;
                        seleccion = 0;
                    }
                }

                else{
                    return;
                }

            break;

            case 27:

                if(modo_eliminar){
                    modo_eliminar = 0;
                    seleccion = 0;
                }
                else{
                    return;
                }

            break;

            default:
                break;
        }
    }
}
