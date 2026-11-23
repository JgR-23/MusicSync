#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "interfaz.h"
#include "ui_form.h"
#include "protocolo.h"
#include "sesion_ui.h"

/* ------------------------------------------------------------------ */
/*  Validaciones (reglas de tu modulo + las de las notas del equipo)  */
/* ------------------------------------------------------------------ */

int validarCorreo(const char *correo)
{
    const char *arroba = strchr(correo, '@');
    const char *punto;
    int len = (int)strlen(correo);

    if (len < 5 || arroba == NULL || arroba == correo) {
        return 0;
    }
    if (strchr(correo, ' ') != NULL || strchr(correo, ';') != NULL) {
        return 0;
    }
    if (strchr(arroba + 1, '@') != NULL) {
        return 0;
    }
    punto = strchr(arroba + 1, '.');
    if (punto == NULL || punto == arroba + 1) {
        return 0;
    }
    if (*(punto + 1) == '\0' || correo[len - 1] == '.') {
        return 0;
    }
    return 1;
}

/* 8+ caracteres, mayuscula, minuscula, numero y simbolo */
int validarContrasenaSegura(const char *pass)
{
    int mayus = 0, minus = 0, numero = 0, simbolo = 0;
    int len = (int)strlen(pass);
    int i;

    if (len < 8) {
        return 0;
    }
    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)pass[i];

        if (isupper(c)) {
            mayus = 1;
        } else if (islower(c)) {
            minus = 1;
        } else if (isdigit(c)) {
            numero = 1;
        } else if (!isspace(c)) {
            simbolo = 1;
        }
    }
    return mayus && minus && numero && simbolo;
}

static int usuario_ok(const char *u)
{
    size_t n = strlen(u);
    size_t i;

    if (n < 3 || n >= LON_USUARIO) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)u[i];
        if (!isalnum(c) && c != '_' && c != '-') {
            return 0;
        }
    }
    return 1;
}

static int validar_login(const Campo *c, char *error, size_t tam)
{
    if (c[0].valor[0] == '\0' || c[1].valor[0] == '\0') {
        snprintf(error, tam, "Completa usuario y contrasena");
        return 0;
    }
    return 1;
}

static int validar_registro(const Campo *c, char *error, size_t tam)
{
    if (!usuario_ok(c[0].valor)) {
        snprintf(error, tam, "Usuario: 3-20 letras, numeros, _ o -");
        return 0;
    }
    if (!validarCorreo(c[1].valor)) {
        snprintf(error, tam, "Correo invalido (debe llevar @ y .algo)");
        return 0;
    }
    if (!validarContrasenaSegura(c[2].valor)) {
        snprintf(error, tam, "Contrasena: 8+, Mayus, minus, numero y simbolo");
        return 0;
    }
    if (strcmp(c[2].valor, c[3].valor) != 0) {
        snprintf(error, tam, "Las contrasenas no coinciden");
        return 0;
    }
    return 1;
}

int pantalla_login(char *usuario, char *contrasena)
{
    Campo campos[2] = {
        {"Nombre de usuario:", usuario,    LON_USUARIO - 1, 0},
        {"Contrasena:",        contrasena, MAX_CAMPO,       1}
    };

    /* "usuario" conserva lo que traiga (por si se reintenta); la contrasena no */
    contrasena[0] = '\0';
    return ejecutar_formulario("INICIO DE SESION", "Ingresa tus credenciales",
                      campos, 2, "Iniciar sesion", validar_login,
                      usuario[0] != '\0' ? 1 : 0);
}

int pantalla_registro(char *usuario, char *correo, char *contrasena)
{
    char confirmar_pass[MAX_CAMPO + 1] = "";
    Campo campos[4] = {
        {"Nombre de usuario:",    usuario,         LON_USUARIO - 1, 0},
        {"Correo electronico:",   correo,          LON_CORREO - 1,  0},
        {"Contrasena:",           contrasena,      MAX_CAMPO,       1},
        {"Confirmar contrasena:", confirmar_pass,  MAX_CAMPO,       1}
    };
    int r;

    /* usuario y correo se conservan si se reintenta; las contrasenas no */
    contrasena[0] = '\0';
    r = ejecutar_formulario("REGISTRO DE USUARIO",
                   "Contrasena: 8+ caracteres, Mayus, minus, numero y simbolo",
                   campos, 4, "Registrar", validar_registro, 0);
    memset(confirmar_pass, 0, sizeof(confirmar_pass));
    return r;
}

static int validar_login_admin(const Campo *c, char *error, size_t tam)
{
    if (c[0].valor[0] == '\0') {
        snprintf(error, tam, "Escribe la contrasena");
        return 0;
    }
    return 1;
}

int pantalla_login_admin(char *contrasena)
{
    Campo campos[1] = {
        {"Contrasena:", contrasena, MAX_CAMPO, 1}
    };

    contrasena[0] = '\0';
    return ejecutar_formulario("ADMINISTRADOR", "Bienvenido administrador",
                               campos, 1, "Entrar", validar_login_admin, 0);
}
