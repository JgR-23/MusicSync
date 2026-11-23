#include <stdio.h>
#include <string.h>

#include "cifrado.h"
#include "comunicacion.h"
#include "interfaz.h"
#include "ipc_util.h"
#include "menus.h"
#include "protocolo.h"
#include "sesion_ui.h"
#include "ui_form.h"
#include "ui_perfil.h"

/* Pide al servidor usuario y correo. 0 = ok, -1 = conexion perdida, 1 = error */
static int leer_perfil(char *usuario, size_t tu, char *correo, size_t tc)
{
    char *separador;
    int r;

    com_preparar(ACC_VER_PERFIL);
    r = com_transaccion();
    if (r < 0) {
        return -1;
    }
    if (r != RESP_OK) {
        mostrar_aviso(com_msg->mensaje, PAR_ERROR);
        return 1;
    }
    com_msg->datos[strcspn(com_msg->datos, "\n")] = '\0';
    separador = strchr(com_msg->datos, ';');
    if (separador == NULL) {
        return 1;
    }
    *separador = '\0';
    copiar_cadena(usuario, tu, com_msg->datos);
    copiar_cadena(correo, tc, separador + 1);
    return 0;
}

static int validar_correo_form(const Campo *c, char *error, size_t tam)
{
    if (!validarCorreo(c[0].valor)) {
        snprintf(error, tam, "Correo invalido (debe llevar @ y .algo)");
        return 0;
    }
    if (c[1].valor[0] == '\0') {
        snprintf(error, tam, "Escribe tu contrasena actual");
        return 0;
    }
    return 1;
}

static int validar_pass_form(const Campo *c, char *error, size_t tam)
{
    if (c[0].valor[0] == '\0') {
        snprintf(error, tam, "Escribe tu contrasena actual");
        return 0;
    }
    if (!validarContrasenaSegura(c[1].valor)) {
        snprintf(error, tam, "Nueva: 8+, Mayus, minus, numero y simbolo");
        return 0;
    }
    if (strcmp(c[1].valor, c[2].valor) != 0) {
        snprintf(error, tam, "Las contrasenas nuevas no coinciden");
        return 0;
    }
    return 1;
}

/* 1 = cambio hecho, 0 = el usuario volvio, -1 = conexion perdida.
 * Si el servidor rechaza, se queda en el formulario. */
static int cambiar_correo(const char *correo_actual)
{
    char correo[LON_CORREO];
    char actual[LON_HASH + 8] = "";
    int foco = 0;   /* tras un error se queda en la contrasena */

    copiar_cadena(correo, sizeof(correo), correo_actual);

    for (;;) {
        Campo campos[2] = {
            {"Nuevo correo:",      correo, LON_CORREO - 1, 0},
            {"Contrasena actual:", actual, 35,             1}
        };
        int r;

        actual[0] = '\0';
        if (!ejecutar_formulario("MODIFICAR CORREO", "Confirma con tu contrasena actual",
                                 campos, 2, "Guardar", validar_correo_form, foco)) {
            return 0;
        }

        com_preparar(ACC_MODIFICAR_PERFIL);
        copiar_cadena(com_msg->correo, sizeof(com_msg->correo), correo);
        contrasenacif(actual, com_msg->hash);
        memset(actual, 0, sizeof(actual));

        r = com_transaccion();
        if (r < 0) {
            return -1;
        }
        erase();
        mostrar_aviso(com_msg->mensaje, r == RESP_OK ? PAR_EXITO : PAR_ERROR);
        if (r == RESP_OK) {
            return 1;
        }
        foco = 1;
    }
}

static int cambiar_contrasena(void)
{
    char actual[40] = "", nueva[40] = "", confirma[40] = "";

    for (;;) {
        Campo campos[3] = {
            {"Contrasena actual:",  actual,   35, 1},
            {"Nueva contrasena:",   nueva,    35, 1},
            {"Confirmar nueva:",    confirma, 35, 1}
        };
        int r;

        actual[0] = nueva[0] = confirma[0] = '\0';
        if (!ejecutar_formulario("CAMBIAR CONTRASENA",
                                 "Nueva: 8+ caracteres, Mayus, minus, numero y simbolo",
                                 campos, 3, "Guardar", validar_pass_form, 0)) {
            return 0;
        }

        com_preparar(ACC_MODIFICAR_PERFIL);
        contrasenacif(actual, com_msg->hash);
        contrasenacif(nueva, com_msg->hash_nuevo);
        memset(actual, 0, sizeof(actual));
        memset(nueva, 0, sizeof(nueva));
        memset(confirma, 0, sizeof(confirma));

        r = com_transaccion();
        if (r < 0) {
            return -1;
        }
        erase();
        mostrar_aviso(com_msg->mensaje, r == RESP_OK ? PAR_EXITO : PAR_ERROR);
        if (r == RESP_OK) {
            return 1;
        }
    }
}

void mostrar_perfil(void)
{
    static const char *const opciones[] = {
        "Modificar correo",
        "Cambiar contrasena",
        "Regresar"
    };

    for (;;) {
        char usuario[LON_USUARIO];
        char correo[LON_CORREO];
        char titulo[LON_USUARIO + 16];
        int eleccion, r;

        if (leer_perfil(usuario, sizeof(usuario), correo, sizeof(correo)) != 0) {
            return;
        }
        snprintf(titulo, sizeof(titulo), "Mi perfil: %s", usuario);

        eleccion = menu_opciones(titulo, correo, opciones, 3);
        if (eleccion == 0) {
            r = cambiar_correo(correo);
        } else if (eleccion == 1) {
            r = cambiar_contrasena();
        } else {
            return;
        }
        if (r < 0) {
            return;
        }
    }
}
