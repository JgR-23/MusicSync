#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "cifrado.h"
#include "ipc_util.h"
#include "respuesta.h"
#include "usuarios.h"

/* Formato de cada linea: usuario;correo;hash */
#define ARCH_USUARIOS "datos/usuarios.txt"

/* Cuenta del administrador: esta definida en el sistema (no esta en el archivo) */
#define ADMIN_USUARIO  "admin"
#define ADMIN_PASSWORD "Admin#2026"

#define MAX_USUARIOS 300

int usuario_valido(const char *usuario)
{
    size_t n = strlen(usuario);
    size_t i;

    if (n < 3 || n >= LON_USUARIO) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)usuario[i];
        if (!isalnum(c) && c != '_' && c != '-') {
            return 0;
        }
    }
    return 1;
}

static int correo_valido(const char *correo)
{
    size_t n = strlen(correo);
    const char *arroba = strchr(correo, '@');
    const char *punto;

    if (n < 5 || n >= LON_CORREO || arroba == NULL || arroba == correo) {
        return 0;
    }
    if (strchr(correo, ' ') != NULL || strchr(correo, ';') != NULL) {
        return 0;
    }
    if (strchr(arroba + 1, '@') != NULL) {
        return 0;
    }
    punto = strchr(arroba + 1, '.');
    if (punto == NULL || punto == arroba + 1 || correo[n - 1] == '.') {
        return 0;
    }
    return 1;
}

static int hash_valido(const char *hash)
{
    size_t n = strlen(hash);
    size_t i;

    if (n == 0 || n > 20) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        if (!isdigit((unsigned char)hash[i])) {
            return 0;
        }
    }
    return 1;
}

/* Parte "usuario;correo;hash" en sus tres campos (modifica la linea) */
static int separar(char *linea, char **usuario, char **correo, char **hash)
{
    char *p;

    *usuario = linea;
    p = strchr(linea, ';');
    if (p == NULL) {
        return 0;
    }
    *p++ = '\0';
    *correo = p;
    p = strchr(p, ';');
    if (p == NULL) {
        return 0;
    }
    *p++ = '\0';
    *hash = p;
    p[strcspn(p, "\r\n")] = '\0';
    return 1;
}

void srv_registro(Mensaje *m)
{
    FILE *archivo;
    char linea[256];
    char *u, *c, *h;
    int existe = 0;

    if (!usuario_valido(m->usuario)) {
        resp_error(m, "Usuario invalido (3-20 letras, numeros, _ o -)");
        return;
    }
    if (!correo_valido(m->correo)) {
        resp_error(m, "Correo invalido");
        return;
    }
    if (!hash_valido(m->hash)) {
        resp_error(m, "Contrasena invalida");
        return;
    }

    if (strcasecmp(m->usuario, ADMIN_USUARIO) == 0) {
        resp_error(m, "Ese nombre de usuario esta reservado");
        return;
    }

    archivo = fopen(ARCH_USUARIOS, "r");
    if (archivo != NULL) {
        while (fgets(linea, sizeof(linea), archivo)) {
            if (separar(linea, &u, &c, &h) && strcasecmp(u, m->usuario) == 0) {
                existe = 1;
                break;
            }
        }
        fclose(archivo);
    }

    if (existe) {
        resp_error(m, "Ese nombre de usuario ya esta registrado");
        return;
    }

    archivo = fopen(ARCH_USUARIOS, "a");
    if (archivo == NULL) {
        resp_error(m, "No se pudo abrir el archivo de usuarios");
        return;
    }
    fprintf(archivo, "%s;%s;%s\n", m->usuario, m->correo, m->hash);
    fclose(archivo);

    resp_ok(m, "Cuenta creada correctamente");
}

void srv_login(Mensaje *m, Sesion *s)
{
    FILE *archivo;
    char linea[256];
    char *u, *c, *h;
    int correcto = 0;

    if (s->logueado) {
        resp_error(m, "Ya hay una sesion iniciada");
        return;
    }
    if (!usuario_valido(m->usuario) || !hash_valido(m->hash)) {
        resp_error(m, "Usuario o contrasena incorrectos");
        return;
    }

    archivo = fopen(ARCH_USUARIOS, "r");
    if (archivo == NULL) {
        resp_error(m, "No se pudo abrir el archivo de usuarios");
        return;
    }
    while (fgets(linea, sizeof(linea), archivo)) {
        if (separar(linea, &u, &c, &h) &&
            strcmp(u, m->usuario) == 0 && strcmp(h, m->hash) == 0) {
            correcto = 1;
            break;
        }
    }
    fclose(archivo);

    if (!correcto) {
        resp_error(m, "Usuario o contrasena incorrectos");
        return;
    }

    s->logueado = 1;
    copiar_cadena(s->usuario, sizeof(s->usuario), m->usuario);
    resp_ok(m, "Bienvenido");
}

void srv_cerrar_sesion(Mensaje *m, Sesion *s)
{
    s->logueado = 0;
    s->es_admin = 0;
    s->usuario[0] = '\0';
    resp_ok(m, "Sesion cerrada");
}

/* ------------------------------------------------------------------ */
/*  Lectura / escritura completa del archivo de usuarios              */
/* ------------------------------------------------------------------ */

typedef struct {
    char usuario[LON_USUARIO];
    char correo[LON_CORREO];
    char hash[LON_HASH];
} UsuarioReg;

static int cargar_usuarios(UsuarioReg *lista, int max)
{
    FILE *archivo = fopen(ARCH_USUARIOS, "r");
    char linea[256];
    char *u, *c, *h;
    int n = 0;

    if (archivo == NULL) {
        return 0;
    }
    while (n < max && fgets(linea, sizeof(linea), archivo)) {
        if (separar(linea, &u, &c, &h)) {
            copiar_cadena(lista[n].usuario, sizeof(lista[n].usuario), u);
            copiar_cadena(lista[n].correo, sizeof(lista[n].correo), c);
            copiar_cadena(lista[n].hash, sizeof(lista[n].hash), h);
            n++;
        }
    }
    fclose(archivo);
    return n;
}

static int guardar_usuarios(const UsuarioReg *lista, int n)
{
    FILE *archivo = fopen(ARCH_USUARIOS, "w");
    int i;

    if (archivo == NULL) {
        return -1;
    }
    for (i = 0; i < n; i++) {
        fprintf(archivo, "%s;%s;%s\n", lista[i].usuario, lista[i].correo, lista[i].hash);
    }
    fclose(archivo);
    return 0;
}

static int buscar_usuario(const UsuarioReg *lista, int n, const char *usuario)
{
    int i;

    for (i = 0; i < n; i++) {
        if (strcmp(lista[i].usuario, usuario) == 0) {
            return i;
        }
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/*  Perfil                                                            */
/* ------------------------------------------------------------------ */

void srv_ver_perfil(Mensaje *m, const Sesion *s)
{
    UsuarioReg lista[MAX_USUARIOS];
    char fila[128];
    int n, idx;

    if (!s->logueado || s->es_admin) {
        resp_error(m, "Inicia sesion como usuario");
        return;
    }
    n = cargar_usuarios(lista, MAX_USUARIOS);
    idx = buscar_usuario(lista, n, s->usuario);
    if (idx < 0) {
        resp_error(m, "No se encontro tu cuenta");
        return;
    }
    snprintf(fila, sizeof(fila), "%s;%s\n", lista[idx].usuario, lista[idx].correo);
    datos_agregar(m, fila);
    resp_ok(m, "OK");
}

/* Cambia correo y/o contrasena. Siempre exige la contrasena actual (m->hash) */
void srv_modificar_perfil(Mensaje *m, const Sesion *s)
{
    UsuarioReg lista[MAX_USUARIOS];
    int n, idx;

    if (!s->logueado || s->es_admin) {
        resp_error(m, "Inicia sesion como usuario");
        return;
    }
    n = cargar_usuarios(lista, MAX_USUARIOS);
    idx = buscar_usuario(lista, n, s->usuario);
    if (idx < 0) {
        resp_error(m, "No se encontro tu cuenta");
        return;
    }
    if (!hash_valido(m->hash) || strcmp(lista[idx].hash, m->hash) != 0) {
        resp_error(m, "La contrasena actual es incorrecta");
        return;
    }
    if (m->correo[0] == '\0' && m->hash_nuevo[0] == '\0') {
        resp_error(m, "No hay cambios que guardar");
        return;
    }
    if (m->correo[0] != '\0') {
        if (!correo_valido(m->correo)) {
            resp_error(m, "Correo invalido");
            return;
        }
        copiar_cadena(lista[idx].correo, sizeof(lista[idx].correo), m->correo);
    }
    if (m->hash_nuevo[0] != '\0') {
        if (!hash_valido(m->hash_nuevo)) {
            resp_error(m, "Contrasena nueva invalida");
            return;
        }
        copiar_cadena(lista[idx].hash, sizeof(lista[idx].hash), m->hash_nuevo);
    }
    if (guardar_usuarios(lista, n) != 0) {
        resp_error(m, "No se pudo guardar el cambio");
        return;
    }
    resp_ok(m, "Datos actualizados correctamente");
}

/* ------------------------------------------------------------------ */
/*  Administrador                                                     */
/* ------------------------------------------------------------------ */

static int requiere_admin(Mensaje *m, const Sesion *s)
{
    if (!s->logueado || !s->es_admin) {
        resp_error(m, "Operacion solo para el administrador");
        return 0;
    }
    return 1;
}

void srv_login_admin(Mensaje *m, Sesion *s)
{
    char esperado[24];

    if (s->logueado) {
        resp_error(m, "Ya hay una sesion iniciada");
        return;
    }
    contrasenacif(ADMIN_PASSWORD, esperado);
    if (strcmp(esperado, m->hash) != 0) {
        resp_error(m, "Contrasena de administrador incorrecta");
        return;
    }
    s->logueado = 1;
    s->es_admin = 1;
    copiar_cadena(s->usuario, sizeof(s->usuario), ADMIN_USUARIO);
    resp_ok(m, "Bienvenido administrador");
}

void srv_adm_ver_usuarios(Mensaje *m, const Sesion *s)
{
    UsuarioReg lista[MAX_USUARIOS];
    char fila[128];
    int n, i;

    if (!requiere_admin(m, s)) {
        return;
    }
    n = cargar_usuarios(lista, MAX_USUARIOS);
    for (i = 0; i < n; i++) {
        snprintf(fila, sizeof(fila), "%s;%s\n", lista[i].usuario, lista[i].correo);
        if (datos_agregar(m, fila) != 0) {
            break;
        }
    }
    resp_ok(m, "OK");
}

void srv_adm_crear_usuario(Mensaje *m, const Sesion *s)
{
    if (!requiere_admin(m, s)) {
        return;
    }
    srv_registro(m);   /* mismas validaciones que un registro normal */
}

/* m->texto = usuario a modificar; m->correo / m->hash_nuevo = cambios (opcionales) */
void srv_adm_modificar_usuario(Mensaje *m, const Sesion *s)
{
    UsuarioReg lista[MAX_USUARIOS];
    int n, idx;

    if (!requiere_admin(m, s)) {
        return;
    }
    n = cargar_usuarios(lista, MAX_USUARIOS);
    idx = buscar_usuario(lista, n, m->texto);
    if (idx < 0) {
        resp_error(m, "Usuario no encontrado");
        return;
    }
    if (m->correo[0] == '\0' && m->hash_nuevo[0] == '\0') {
        resp_error(m, "No hay cambios que guardar");
        return;
    }
    if (m->correo[0] != '\0') {
        if (!correo_valido(m->correo)) {
            resp_error(m, "Correo invalido");
            return;
        }
        copiar_cadena(lista[idx].correo, sizeof(lista[idx].correo), m->correo);
    }
    if (m->hash_nuevo[0] != '\0') {
        if (!hash_valido(m->hash_nuevo)) {
            resp_error(m, "Contrasena invalida");
            return;
        }
        copiar_cadena(lista[idx].hash, sizeof(lista[idx].hash), m->hash_nuevo);
    }
    if (guardar_usuarios(lista, n) != 0) {
        resp_error(m, "No se pudo guardar el cambio");
        return;
    }
    resp_ok(m, "Usuario modificado correctamente");
}

/* Elimina la cuenta y sus archivos personales (playlists y carrito) */
void srv_adm_eliminar_usuario(Mensaje *m, const Sesion *s)
{
    UsuarioReg lista[MAX_USUARIOS];
    char ruta[96];
    int n, idx, i;

    if (!requiere_admin(m, s)) {
        return;
    }
    n = cargar_usuarios(lista, MAX_USUARIOS);
    idx = buscar_usuario(lista, n, m->texto);
    if (idx < 0 || !usuario_valido(m->texto)) {
        resp_error(m, "Usuario no encontrado");
        return;
    }
    for (i = idx; i < n - 1; i++) {
        lista[i] = lista[i + 1];
    }
    n--;
    if (guardar_usuarios(lista, n) != 0) {
        resp_error(m, "No se pudo guardar el cambio");
        return;
    }

    snprintf(ruta, sizeof(ruta), "datos/playlists_%s.txt", m->texto);
    remove(ruta);
    snprintf(ruta, sizeof(ruta), "datos/carrito_%s.txt", m->texto);
    remove(ruta);

    resp_ok(m, "Usuario eliminado correctamente");
}
