/*
 * Cliente de MusicSync.
 *
 * Se ejecuta en su propia terminal (con el servidor ya corriendo en otra):
 *     ./cliente
 *
 * Flujo: Login / Registro -> Menu de inicio -> Biblioteca o Gestion de playlist.
 * Todos los datos se piden al servidor por memoria compartida y semaforos.
 */

#include <stdio.h>
#include <string.h>

#include "colores.h"
#include "interfaz.h"
#include "ipc_cliente.h"
#include "playlists.h"
#include "ui_auth.h"
#include "ui_biblioteca.h"
#include "ui_comun.h"
#include "vista_menu_inicio.h"

/* -------------------------------------------------- */
/* Biblioteca                                         */
/* -------------------------------------------------- */

static void abrir_biblioteca(void){
    static RegCancion registros[IPC_MAX_CATALOGO];
    static CancionUI catalogo[MAX_CANCIONES];
    int max = MAX_CANCIONES < IPC_MAX_CATALOGO ? MAX_CANCIONES : IPC_MAX_CATALOGO;
    int total, i;

    total = ipc_listar_catalogo(registros, max);

    if(total < 0){
        mostrar_mensaje_ui(ipc_ultimo_error());
        return;
    }

    if(total == 0){
        mostrar_mensaje_ui("El catalogo esta vacio.");
        return;
    }

    for(i = 0; i < total; i++){
        catalogo[i].id = registros[i].id;
        snprintf(catalogo[i].titulo,  sizeof(catalogo[i].titulo),  "%s", registros[i].titulo);
        snprintf(catalogo[i].artista, sizeof(catalogo[i].artista), "%s", registros[i].artista);
        snprintf(catalogo[i].album,   sizeof(catalogo[i].album),   "%s", registros[i].album);
        snprintf(catalogo[i].genero,  sizeof(catalogo[i].genero),  "%s", registros[i].genero);
        catalogo[i].precio = registros[i].precio;
    }

    pantalla_biblioteca(catalogo, total);
}

/* -------------------------------------------------- */
/* Sesion iniciada: menu principal                    */
/* -------------------------------------------------- */

static void ejecutar_sesion(void){
    int salir = 0;

    while(!salir){
        OpcionMenuInicio opcion = mostrar_menu_inicio();

        switch(opcion){

            case MENU_BIBLIOTECA:
                colores_aplicar(COLORES_INTERFAZ);
                abrir_biblioteca();
            break;

            case MENU_GESTION_PLAYLIST:
                colores_aplicar(COLORES_INTERFAZ);
                mostrar_playlists();
            break;

            default:   /* MENU_SALIR: cerrar sesion */
                salir = 1;
            break;
        }
    }

    ipc_logout();
}

/* -------------------------------------------------- */
/* Programa principal                                 */
/* -------------------------------------------------- */

int main(void)
{
    EstadoLogin login;
    EstadoRegistro registro;
    int pantalla = 0; /* 0: Inicio de sesion, 1: Registro */
    int tecla, resultado;

    if(ipc_conectar() < 0)
    {
        fprintf(stderr, "No se pudo conectar con el servidor: %s.\n", ipc_ultimo_error());
        fprintf(stderr, "Inicia primero el servidor en otra terminal:  ./servidor\n");
        return 1;
    }

    inicializar_interfaz();
    colores_aplicar(COLORES_AUTH);
    inicializarLogin(&login);
    inicializarRegistro(&registro);

    while(1)
    {
        if(pantalla == 0)
        {
            dibujarLogin(&login);
        }
        else
        {
            dibujarRegistro(&registro);
        }

        tecla = getch();

        if(tecla == 27)
        {
            if(pantalla == 1)
            {
                pantalla = 0;   /* ESC en el registro regresa al login */
                continue;
            }
            break;              /* ESC en el login sale del programa */
        }

        if(pantalla == 0)
        {
            resultado = teclaLogin(&login, tecla);

            if(resultado == 1)
            {
                inicializarRegistro(&registro);
                pantalla = 1;
            }
            else if(resultado == 2)
            {
                ejecutar_sesion();

                inicializarLogin(&login);
                colores_aplicar(COLORES_AUTH);
            }
        }
        else
        {
            teclaRegistro(&registro, tecla);

            if(registro.registrado)
            {
                /* Registro correcto: vuelve al login con el usuario ya escrito */
                inicializarLogin(&login);
                snprintf(login.usuario, sizeof(login.usuario), "%s", registro.usuario);
                login.lenUsuario = (int)strlen(login.usuario);
                login.campoActivo = 1;
                login.esError = 0;
                snprintf(login.mensaje, sizeof(login.mensaje), "Registro exitoso. Ahora inicia sesion");

                pantalla = 0;
            }
        }
    }

    finalizar_interfaz();
    ipc_desconectar();
    return 0;
}
