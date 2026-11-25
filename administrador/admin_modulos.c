#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "admin_modulos.h"
#include "cifrado.h"
#include "comunicacion.h"
#include "interfaz.h"
#include "ipc_util.h"
#include "menus.h"
#include "sesion_ui.h"
#include "ui_form.h"
#include "ui_texto.h"

#define MAX_LISTA 100

static void aviso_limpio(const char *mensaje, int tipo)
{
    erase();
    mostrar_aviso(mensaje, tipo);
}

/* Muestra el mensaje de la respuesta (verde si fue OK, rojo si no) */
static void mostrar_resultado(int r)
{
    aviso_limpio(com_msg->mensaje, r == RESP_OK ? PAR_EXITO : PAR_ERROR);
}

/* Menu para elegir uno de varios elementos. Devuelve el indice o -1 */
static int elegir_de_lista(const char *titulo, const char *subtitulo,
                           char etiquetas[][100], int n)
{
    const char *punteros[MAX_LISTA];
    int i;

    for (i = 0; i < n; i++) {
        punteros[i] = etiquetas[i];
    }
    return menu_opciones(titulo, subtitulo, punteros, n);
}

/* ================================================================== */
/*  USUARIOS                                                          */
/* ================================================================== */

typedef struct {
    char usuario[LON_USUARIO];
    char correo[LON_CORREO];
} UsuLista;

/* Devuelve cuantos usuarios hay, o -1 si se perdio la conexion */
static int cargar_usuarios(UsuLista *lista, int max)
{
    char *guardar = NULL;
    char *linea;
    int n = 0;
    int r;

    com_preparar(ACC_ADM_VER_USUARIOS);
    r = com_transaccion();
    if (r < 0) {
        return -1;
    }
    if (r != RESP_OK) {
        mostrar_resultado(r);
        return 0;
    }
    for (linea = strtok_r(com_msg->datos, "\n", &guardar);
         linea != NULL && n < max;
         linea = strtok_r(NULL, "\n", &guardar)) {

        char *sep = strchr(linea, ';');

        if (sep == NULL) {
            continue;
        }
        *sep = '\0';
        copiar_cadena(lista[n].usuario, sizeof(lista[n].usuario), linea);
        copiar_cadena(lista[n].correo, sizeof(lista[n].correo), sep + 1);
        n++;
    }
    return n;
}

/* 1 = elegido (en *elegido), 0 = cancelo o no hay usuarios, -1 = conexion perdida */
static int elegir_usuario(const char *titulo, UsuLista *elegido)
{
    UsuLista lista[MAX_LISTA];
    char etiquetas[MAX_LISTA][100];
    int n, i, sel;

    n = cargar_usuarios(lista, MAX_LISTA);
    if (n < 0) {
        return -1;
    }
    if (n == 0) {
        aviso_limpio("No hay usuarios registrados.", PAR_ERROR);
        return 0;
    }
    for (i = 0; i < n; i++) {
        snprintf(etiquetas[i], sizeof(etiquetas[i]), "%-20.20s %.60s",
                 lista[i].usuario, lista[i].correo);
    }
    sel = elegir_de_lista(titulo, "Elige un usuario", etiquetas, n);
    if (sel < 0) {
        return 0;
    }
    *elegido = lista[sel];
    return 1;
}

static void ver_usuarios(void)
{
    static char texto[LON_DATOS];
    UsuLista lista[MAX_LISTA];
    char fila[160];
    int n, i;

    n = cargar_usuarios(lista, MAX_LISTA);
    if (n < 0) {
        return;
    }
    snprintf(texto, sizeof(texto), "Usuarios registrados: %d\n\n%-22s %s\n%s\n",
             n, "Usuario", "Correo", "------------------------------------------------------");
    for (i = 0; i < n; i++) {
        snprintf(fila, sizeof(fila), "%-22.22s %.63s\n", lista[i].usuario, lista[i].correo);
        if (strlen(texto) + strlen(fila) + 1 < sizeof(texto)) {
            strcat(texto, fila);
        }
    }
    mostrar_texto("USUARIOS REGISTRADOS", texto);
}

static void crear_usuario(void)
{
    char usuario[LON_USUARIO] = "";
    char correo[LON_CORREO] = "";
    char contrasena[MAX_CAMPO + 1];

    for (;;) {
        int r;

        if (!pantalla_registro(usuario, correo, contrasena)) {
            return;
        }
        com_preparar(ACC_ADM_CREAR_USUARIO);
        copiar_cadena(com_msg->usuario, sizeof(com_msg->usuario), usuario);
        copiar_cadena(com_msg->correo, sizeof(com_msg->correo), correo);
        contrasenacif(contrasena, com_msg->hash);
        memset(contrasena, 0, sizeof(contrasena));

        r = com_transaccion();
        if (r < 0) {
            return;
        }
        mostrar_resultado(r);
        if (r == RESP_OK) {
            return;
        }
    }
}

static int validar_modif_usuario(const Campo *c, char *error, size_t tam)
{
    if (!validarCorreo(c[0].valor)) {
        snprintf(error, tam, "Correo invalido (debe llevar @ y .algo)");
        return 0;
    }
    if (c[1].valor[0] != '\0' && !validarContrasenaSegura(c[1].valor)) {
        snprintf(error, tam, "Contrasena: 8+, Mayus, minus, numero y simbolo");
        return 0;
    }
    if (strcmp(c[1].valor, c[2].valor) != 0) {
        snprintf(error, tam, "Las contrasenas no coinciden");
        return 0;
    }
    return 1;
}

static void modificar_usuario(void)
{
    UsuLista objetivo;
    char correo[LON_CORREO];
    char nueva[40] = "", confirma[40] = "";
    int r = elegir_usuario("Modificar usuario", &objetivo);

    if (r <= 0) {
        return;
    }
    copiar_cadena(correo, sizeof(correo), objetivo.correo);

    for (;;) {
        char subtitulo[64];
        Campo campos[3] = {
            {"Nuevo correo:",      correo,   LON_CORREO - 1, 0},
            {"Nueva contrasena:",  nueva,    35,             1},
            {"Confirmar:",         confirma, 35,             1}
        };

        nueva[0] = confirma[0] = '\0';
        snprintf(subtitulo, sizeof(subtitulo), "%.20s  (contrasena vacia = no cambiarla)", objetivo.usuario);
        if (!ejecutar_formulario("MODIFICAR USUARIO", subtitulo, campos, 3,
                                 "Guardar", validar_modif_usuario, 0)) {
            return;
        }

        com_preparar(ACC_ADM_MODIFICAR_USUARIO);
        copiar_cadena(com_msg->texto, sizeof(com_msg->texto), objetivo.usuario);
        copiar_cadena(com_msg->correo, sizeof(com_msg->correo), correo);
        if (nueva[0] != '\0') {
            contrasenacif(nueva, com_msg->hash_nuevo);
        }
        memset(nueva, 0, sizeof(nueva));
        memset(confirma, 0, sizeof(confirma));

        r = com_transaccion();
        if (r < 0) {
            return;
        }
        mostrar_resultado(r);
        if (r == RESP_OK) {
            return;
        }
    }
}

static void eliminar_usuario(void)
{
    UsuLista objetivo;
    char pregunta[120];
    int r = elegir_usuario("Eliminar usuario", &objetivo);

    if (r <= 0) {
        return;
    }
    snprintf(pregunta, sizeof(pregunta), "Eliminar a \"%.20s\"?", objetivo.usuario);
    erase();
    if (!confirmar(pregunta)) {
        return;
    }
    com_preparar(ACC_ADM_ELIMINAR_USUARIO);
    copiar_cadena(com_msg->texto, sizeof(com_msg->texto), objetivo.usuario);
    r = com_transaccion();
    if (r < 0) {
        return;
    }
    mostrar_resultado(r);
}

void admin_usuarios(void)
{
    static const char *const opciones[] = {
        "Ver usuarios",
        "Crear usuario",
        "Modificar usuario",
        "Eliminar usuario",
        "Regresar"
    };
    int ultima = 0;

    while (!com_perdida()) {
        int op = menu_opciones_desde("Usuarios", "Administrar usuarios del sistema",
                                     opciones, 5, ultima);

        if (op >= 0) {
            ultima = op;
        }
        if (op == 0) {
            ver_usuarios();
        } else if (op == 1) {
            crear_usuario();
        } else if (op == 2) {
            modificar_usuario();
        } else if (op == 3) {
            eliminar_usuario();
        } else {
            return;
        }
    }
}

/* ================================================================== */
/*  CANCIONES (productos)                                             */
/* ================================================================== */

typedef struct {
    int    id;
    char   titulo[64];
    char   artista[64];
    char   album[64];
    char   genero[64];
    char   duracion[10];
    double precio;
} CancionAdm;

/* Devuelve cuantas canciones hay, o -1 si se perdio la conexion */
static int cargar_canciones(CancionAdm *lista, int max)
{
    char *guardar = NULL;
    char *linea;
    int n = 0;
    int r;

    com_preparar(ACC_VER_BIBLIOTECA);
    r = com_transaccion();
    if (r < 0) {
        return -1;
    }
    if (r != RESP_OK) {
        mostrar_resultado(r);
        return 0;
    }
    for (linea = strtok_r(com_msg->datos, "\n", &guardar);
         linea != NULL && n < max;
         linea = strtok_r(NULL, "\n", &guardar)) {

        CancionAdm *c = &lista[n];

        memset(c, 0, sizeof(*c));
        if (sscanf(linea, "%d;%63[^;];%63[^;];%63[^;];%63[^;];%9[^;];%lf",
                   &c->id, c->titulo, c->artista, c->album, c->genero,
                   c->duracion, &c->precio) == 7) {
            n++;
        }
    }
    return n;
}

static int elegir_cancion(const char *titulo, CancionAdm *elegida)
{
    CancionAdm lista[MAX_LISTA];
    char etiquetas[MAX_LISTA][100];
    int n, i, sel;

    n = cargar_canciones(lista, MAX_LISTA);
    if (n < 0) {
        return -1;
    }
    if (n == 0) {
        aviso_limpio("La biblioteca esta vacia.", PAR_ERROR);
        return 0;
    }
    for (i = 0; i < n; i++) {
        snprintf(etiquetas[i], sizeof(etiquetas[i]), "%3d  %-28.28s %-16.16s $%.2f",
                 lista[i].id, lista[i].titulo, lista[i].artista, lista[i].precio);
    }
    sel = elegir_de_lista(titulo, "Elige una cancion", etiquetas, n);
    if (sel < 0) {
        return 0;
    }
    *elegida = lista[sel];
    return 1;
}

static void ver_canciones(void)
{
    static char texto[LON_DATOS];
    CancionAdm lista[MAX_LISTA];
    char fila[300];
    int n, i;

    n = cargar_canciones(lista, MAX_LISTA);
    if (n < 0) {
        return;
    }
    snprintf(texto, sizeof(texto), "Canciones en la biblioteca: %d\n\n"
             "%-4s %-26s %-16s %-18s %-6s %s\n%s\n", n,
             "ID", "Titulo", "Artista", "Album", "Dur.", "Precio",
             "------------------------------------------------------------------------------");
    for (i = 0; i < n; i++) {
        snprintf(fila, sizeof(fila), "%-4d %-26.26s %-16.16s %-18.18s %-6s $%.2f\n",
                 lista[i].id, lista[i].titulo, lista[i].artista, lista[i].album,
                 lista[i].duracion, lista[i].precio);
        if (strlen(texto) + strlen(fila) + 1 < sizeof(texto)) {
            strcat(texto, fila);
        }
    }
    mostrar_texto("BIBLIOTECA DE CANCIONES", texto);
}

/* Campos: 0 titulo, 1 artista, 2 album, 3 genero, 4 duracion (m:ss), 5 precio */
static int validar_cancion(const Campo *c, char *error, size_t tam)
{
    int minutos, segundos, i;
    char extra;
    char *fin;
    double precio;

    for (i = 0; i < 4; i++) {
        if (strchr(c[i].valor, ';') != NULL) {
            snprintf(error, tam, "No uses ';' en los textos");
            return 0;
        }
    }
    if (c[0].valor[0] == '\0') {
        snprintf(error, tam, "El titulo es obligatorio");
        return 0;
    }
    if (c[1].valor[0] == '\0') {
        snprintf(error, tam, "El artista es obligatorio");
        return 0;
    }
    if (sscanf(c[4].valor, "%d:%d%c", &minutos, &segundos, &extra) != 2 ||
        minutos < 0 || minutos > 99 || segundos < 0 || segundos > 59) {
        snprintf(error, tam, "Duracion invalida (formato m:ss, ej. 3:45)");
        return 0;
    }
    precio = strtod(c[5].valor, &fin);
    if (fin == c[5].valor || *fin != '\0' || precio <= 0.0 || precio > 99999.0) {
        snprintf(error, tam, "Precio invalido (numero mayor a 0)");
        return 0;
    }
    return 1;
}

/* Arma "titulo;artista;album;genero;duracion;precio" (album/genero vacios = "-") */
static void armar_entrada(const CancionAdm *c)
{
    snprintf(com_msg->entrada, sizeof(com_msg->entrada), "%s;%s;%s;%s;%s;%.2f",
             c->titulo, c->artista,
             c->album[0] != '\0' ? c->album : "-",
             c->genero[0] != '\0' ? c->genero : "-",
             c->duracion, c->precio);
}

/* Formulario de cancion. "inicial" puede traer los datos a editar.
 * Si id > 0 modifica esa cancion; si es 0 agrega una nueva. */
static void formulario_cancion(const char *titulo, const CancionAdm *inicial, int id)
{
    CancionAdm c;
    char precio[16] = "";

    memset(&c, 0, sizeof(c));
    if (inicial != NULL) {
        c = *inicial;
        snprintf(precio, sizeof(precio), "%.2f", inicial->precio);
    }

    for (;;) {
        Campo campos[6] = {
            {"Titulo:",          c.titulo,   63, 0},
            {"Artista:",         c.artista,  63, 0},
            {"Album:",           c.album,    63, 0},
            {"Genero:",          c.genero,   63, 0},
            {"Duracion (m:ss):", c.duracion,  9, 0},
            {"Precio:",          precio,     10, 0}
        };
        int r;

        if (!ejecutar_formulario(titulo, "Titulo, artista, duracion y precio son obligatorios",
                                 campos, 6, "Guardar", validar_cancion, 0)) {
            return;
        }
        c.precio = strtod(precio, NULL);

        com_preparar(id > 0 ? ACC_ADM_MODIFICAR_CANCION : ACC_ADM_AGREGAR_CANCION);
        com_msg->arg = id;
        armar_entrada(&c);

        r = com_transaccion();
        if (r < 0) {
            return;
        }
        mostrar_resultado(r);
        if (r == RESP_OK) {
            return;
        }
    }
}

static void modificar_cancion(void)
{
    CancionAdm c;
    int r = elegir_cancion("Modificar cancion", &c);

    if (r > 0) {
        formulario_cancion("MODIFICAR CANCION", &c, c.id);
    }
}

static void eliminar_cancion(void)
{
    CancionAdm c;
    char pregunta[120];
    int r = elegir_cancion("Eliminar cancion", &c);

    if (r <= 0) {
        return;
    }
    snprintf(pregunta, sizeof(pregunta), "Eliminar \"%.35s\"?", c.titulo);
    erase();
    if (!confirmar(pregunta)) {
        return;
    }
    com_preparar(ACC_ADM_ELIMINAR_CANCION);
    com_msg->arg = c.id;
    r = com_transaccion();
    if (r < 0) {
        return;
    }
    mostrar_resultado(r);
}

void admin_canciones(void)
{
    static const char *const opciones[] = {
        "Ver canciones",
        "Agregar cancion",
        "Modificar cancion",
        "Eliminar cancion",
        "Regresar"
    };
    int ultima = 0;

    while (!com_perdida()) {
        int op = menu_opciones_desde("Canciones", "Administrar productos de la tienda",
                                     opciones, 5, ultima);

        if (op >= 0) {
            ultima = op;
        }
        if (op == 0) {
            ver_canciones();
        } else if (op == 1) {
            formulario_cancion("AGREGAR CANCION", NULL, 0);
        } else if (op == 2) {
            modificar_cancion();
        } else if (op == 3) {
            eliminar_cancion();
        } else {
            return;
        }
    }
}

/* ================================================================== */
/*  REPORTES DE VENTAS                                                */
/* ================================================================== */

void admin_reportes(void)
{
    static const char *const opciones[] = {
        "Ventas diarias (hoy)",
        "Ventas semanales (ultimos 7 dias)",
        "Ventas mensuales (mes actual)",
        "Regresar"
    };
    static const char *const titulos[] = {
        "REPORTE DE VENTAS DIARIAS",
        "REPORTE DE VENTAS SEMANALES",
        "REPORTE DE VENTAS MENSUALES"
    };
    int ultima = 0;

    while (!com_perdida()) {
        int op = menu_opciones_desde("Reportes de ventas", "Elige el periodo",
                                     opciones, 4, ultima);

        if (op >= 0) {
            ultima = op;
        }
        if (op >= 0 && op <= 2) {
            int r;

            com_preparar(ACC_ADM_REPORTE);
            com_msg->arg = REPORTE_DIARIO + op;
            r = com_transaccion();
            if (r < 0) {
                return;
            }
            if (r == RESP_OK) {
                mostrar_texto(titulos[op], com_msg->datos);
            } else {
                mostrar_resultado(r);
            }
        } else {
            return;
        }
    }
}