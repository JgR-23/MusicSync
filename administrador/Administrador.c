#include <stdio.h>
#include <string.h>

#include "admin_modulos.h"
#include "cifrado.h"
#include "comunicacion.h"
#include "interfaz.h"
#include "menus.h"
#include "sesion_ui.h"

/* Login del administrador. 1 = entro, 0 = decidio salir, -1 = conexion perdida.
 * Si la contrasena es incorrecta se queda en la pantalla de login. */
static int login_admin(void)
{
    char contrasena[MAX_CAMPO + 1];

    for (;;) {
        int r;

        if (!pantalla_login_admin(contrasena)) {
            return 0;
        }
        com_preparar(ACC_LOGIN_ADMIN);
        contrasenacif(contrasena, com_msg->hash);
        memset(contrasena, 0, sizeof(contrasena));

        r = com_transaccion();
        if (r < 0) {
            return -1;
        }
        if (r == RESP_OK) {
            return 1;
        }
        erase();
        mostrar_aviso(com_msg->mensaje, PAR_ERROR);
    }
}

static void panel(void)
{
    static const char *const opciones[] = {
        "Administrar usuarios",
        "Administrar canciones (productos)",
        "Reportes de ventas",
        "Cerrar sesion"
    };
    int ultima = 0;

    while (!com_perdida()) {
        int opcion = menu_opciones_desde("Panel de administracion",
                                         "MusicSync - Administrador", opciones, 4, ultima);

        if (opcion >= 0) {
            ultima = opcion;
        }
        if (opcion == 0) {
            admin_usuarios();
        } else if (opcion == 1) {
            admin_canciones();
        } else if (opcion == 2) {
            admin_reportes();
        } else if (opcion == 3 || opcion == -1) {
            if (confirmar("Cerrar sesion de administrador?")) {
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
        if (login_admin() != 1) {
            break;
        }
        panel();
    }

    com_desconectar();
    finalizar_interfaz();
    return 0;
}
