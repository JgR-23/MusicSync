#include <stdio.h>
#include <string.h>

#include "cifrado.h"
#include "comunicacion.h"
#include "interfaz.h"
#include "ipc_util.h"
#include "menus.h"
#include "playlists.h"
#include "ui_compras.h"
#include "ui_carrito.h"
#include "ui_perfil.h"
#include "sesion_ui.h"
#include "ui_biblioteca.h"

/* ------------------------------------------------------------------ */
/*  Biblioteca                                                        */
/* ------------------------------------------------------------------ */

/* Convierte las filas "id;titulo;artista;album;genero;duracion;precio" */
static int leer_canciones(char *datos, CancionUI *lista, int max)
{
    char *guardar = NULL;
    char *linea;
    int n = 0;

    for (linea = strtok_r(datos, "\n", &guardar);
         linea != NULL && n < max;
         linea = strtok_r(NULL, "\n", &guardar)) {

        CancionUI *c = &lista[n];

        memset(c, 0, sizeof(*c));
        if (sscanf(linea, "%d;%63[^;];%63[^;];%63[^;];%63[^;];%9[^;];%lf",
                   &c->id, c->titulo, c->artista, c->album, c->genero,
                   c->duracion, &c->precio) == 7) {
            n++;
        }
    }
    return n;
}

/* ENTER sobre una cancion de la biblioteca: se agrega al carrito.
 * (Para meterla en una playlist primero hay que comprarla: "Mis compras") */
static void accion_cancion(const CancionUI *cancion)
{
    agregar_al_carrito(cancion->id);
}

static void abrir_biblioteca(void)
{
    CancionUI catalogo[MAX_CANCIONES];
    char info[200];
    double saldo;
    int total, r;

    r = com_saldo(&saldo);
    if (r < 0) {
        return;
    }
    snprintf(info, sizeof(info), "Saldo: $%.2f", saldo);

    com_preparar(ACC_VER_BIBLIOTECA);
    r = com_transaccion();
    if (r < 0) {
        return;
    }
    if (r != RESP_OK) {
        mostrar_aviso(com_msg->mensaje, PAR_ERROR);
        return;
    }

    total = leer_canciones(com_msg->datos, catalogo, MAX_CANCIONES);
    if (total == 0) {
        mostrar_aviso("La biblioteca esta vacia.", PAR_ERROR);
        return;
    }
    pantalla_biblioteca(catalogo, total, accion_cancion, info);
}

/* ------------------------------------------------------------------ */
/*  Sesion                                                            */
/* ------------------------------------------------------------------ */

/* Aviso sobre pantalla limpia (para que no se vea el formulario detras) */
static void aviso_limpio(const char *mensaje, int tipo)
{
    erase();
    mostrar_aviso(mensaje, tipo);
}

/* 1 = sesion iniciada, 0 = el usuario volvio atras, -1 = conexion perdida.
 * Si los datos son incorrectos se queda en la pantalla de login. */
static int iniciar_sesion(char *usuario_salida)
{
    char usuario[LON_USUARIO] = "";
    char contrasena[MAX_CAMPO + 1];
    int r;

    for (;;) {
        if (!pantalla_login(usuario, contrasena)) {
            return 0;
        }

        com_preparar(ACC_LOGIN);
        copiar_cadena(com_msg->usuario, sizeof(com_msg->usuario), usuario);
        contrasenacif(contrasena, com_msg->hash);   /* la contrasena viaja cifrada */
        memset(contrasena, 0, sizeof(contrasena));

        r = com_transaccion();
        if (r < 0) {
            return -1;
        }
        if (r == RESP_OK) {
            copiar_cadena(usuario_salida, LON_USUARIO, usuario);
            return 1;
        }
        aviso_limpio(com_msg->mensaje, PAR_ERROR);
    }
}

static void registrar_usuario(void)
{
    char usuario[LON_USUARIO] = "";
    char correo[LON_CORREO] = "";
    char contrasena[MAX_CAMPO + 1];
    int r;

    for (;;) {
        if (!pantalla_registro(usuario, correo, contrasena)) {
            return;
        }

        com_preparar(ACC_REGISTRO);
        copiar_cadena(com_msg->usuario, sizeof(com_msg->usuario), usuario);
        copiar_cadena(com_msg->correo, sizeof(com_msg->correo), correo);
        contrasenacif(contrasena, com_msg->hash);
        memset(contrasena, 0, sizeof(contrasena));

        r = com_transaccion();
        if (r < 0) {
            return;
        }
        if (r == RESP_OK) {
            aviso_limpio("Cuenta creada. Ya puedes iniciar sesion.", PAR_EXITO);
            return;
        }
        aviso_limpio(com_msg->mensaje, PAR_ERROR);
    }
}

static void menu_principal(const char *usuario)
{
    static const char *const opciones[] = {
        "Biblioteca",
        "Playlists",
        "Carrito de compras",
        "Mis compras",
        "Mi perfil",
        "Cerrar sesion"
    };
    int ultima = 0;

    while (!com_perdida()) {
        char subtitulo[300];
        double saldo;
        int opcion;

        if (com_saldo(&saldo) < 0) {
            return;
        }
        snprintf(subtitulo, sizeof(subtitulo), "Usuario: %.20s    Saldo: $%.2f", usuario, saldo);

        opcion = menu_opciones_desde("Menu principal", subtitulo, opciones, 6, ultima);
        if (opcion >= 0) {
            ultima = opcion;
        }

        if (com_perdida()) {
            return;
        }
        if (opcion == 0) {
            abrir_biblioteca();
        } else if (opcion == 1) {
            mostrar_playlists();
        } else if (opcion == 2) {
            mostrar_carrito();
        } else if (opcion == 3) {
            mostrar_compras();
        } else if (opcion == 4) {
            mostrar_perfil();
        } else if (opcion == 5 || opcion == -1) {
            if (confirmar("Cerrar sesion?")) {
                com_preparar(ACC_CERRAR_SESION);
                com_transaccion();
                return;
            }
        }
    }
}

int main(void)
{
    int estado;

    inicializar_interfaz();

    estado = com_conectar();
    if (estado == COM_SIN_SERVIDOR) {
        mostrar_aviso("El servidor no esta activo. Inicialo e intenta de nuevo.", PAR_ERROR);
        finalizar_interfaz();
        return 1;
    }
    if (estado == COM_RECHAZADO) {
        mostrar_aviso(com_error(), PAR_ERROR);
        finalizar_interfaz();
        return 1;
    }

    while (!com_perdida()) {
        int opcion = pantalla_inicio();

        if (opcion == 0) {
            char usuario[LON_USUARIO];
            int r = iniciar_sesion(usuario);

            if (r == 1) {
                menu_principal(usuario);
            }
        } else if (opcion == 1) {
            registrar_usuario();
        } else if (opcion == 2 || (opcion == -1 && confirmar("Salir del sistema?"))) {
            break;
        }
    }

    com_desconectar();
    finalizar_interfaz();
    return 0;
}