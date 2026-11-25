#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "biblioteca.h"
#include "carrito_srv.h"
#include "ipc_util.h"
#include "respuesta.h"
#include "saldo.h"

/* Carrito por usuario: datos/carrito_<usuario>.txt  (un id de cancion por linea)
 * Ventas: datos/ventas.txt  (fecha;usuario;id;titulo;precio)
 * Las "compras" de un usuario son sus lineas de ventas.txt                      */

#define MAX_CARRITO   50
#define MAX_COMPRAS   200
#define ARCH_VENTAS   "datos/ventas.txt"

static void ruta_carrito(const char *usuario, char *destino, size_t tam)
{
    snprintf(destino, tam, "datos/carrito_%.20s.txt", usuario);
}

static int cargar(const char *usuario, int *ids, int max)
{
    char ruta[96];
    char linea[32];
    FILE *archivo;
    int n = 0;

    ruta_carrito(usuario, ruta, sizeof(ruta));
    archivo = fopen(ruta, "r");
    if (archivo == NULL) {
        return 0;
    }
    while (n < max && fgets(linea, sizeof(linea), archivo)) {
        ids[n++] = atoi(linea);
    }
    fclose(archivo);
    return n;
}

static int guardar(const char *usuario, const int *ids, int n)
{
    char ruta[96];
    FILE *archivo;
    int i;

    ruta_carrito(usuario, ruta, sizeof(ruta));
    if (n == 0) {
        remove(ruta);
        return 0;
    }
    archivo = fopen(ruta, "w");
    if (archivo == NULL) {
        return -1;
    }
    for (i = 0; i < n; i++) {
        fprintf(archivo, "%d\n", ids[i]);
    }
    fclose(archivo);
    return 0;
}

static int es_cliente(Mensaje *m, const Sesion *s)
{
    if (!s->logueado || s->es_admin) {
        resp_error(m, "Inicia sesion como usuario");
        return 0;
    }
    return 1;
}

int cancion_comprada(const char *usuario, int id)
{
    FILE *archivo = fopen(ARCH_VENTAS, "r");
    char linea[512];
    char dueno[24];
    int id_venta;
    int comprada = 0;

    if (archivo == NULL) {
        return 0;
    }
    while (fgets(linea, sizeof(linea), archivo)) {
        if (sscanf(linea, "%*[^;];%20[^;];%d", dueno, &id_venta) == 2 &&
            id_venta == id && strcmp(dueno, usuario) == 0) {
            comprada = 1;
            break;
        }
    }
    fclose(archivo);
    return comprada;
}

void srv_ver_carrito(Mensaje *m, const Sesion *s)
{
    int ids[MAX_CARRITO];
    CancionSrv c;
    char fila[400];
    int n, i;

    if (!es_cliente(m, s)) {
        return;
    }
    n = cargar(s->usuario, ids, MAX_CARRITO);
    for (i = 0; i < n; i++) {
        if (biblioteca_buscar(ids[i], &c)) {
            snprintf(fila, sizeof(fila), "%d;%.63s;%.63s;%.2f\n",
                     c.id, c.titulo, c.artista, c.precio);
            datos_agregar(m, fila);
        }
    }
    resp_ok(m, "OK");
}

void srv_agregar_carrito(Mensaje *m, const Sesion *s)
{
    int ids[MAX_CARRITO];
    CancionSrv c;
    int n, i;

    if (!es_cliente(m, s)) {
        return;
    }
    if (!biblioteca_buscar(m->arg, &c)) {
        resp_error(m, "La cancion no existe en la biblioteca");
        return;
    }
    if (cancion_comprada(s->usuario, m->arg)) {
        resp_error(m, "Ya compraste esta cancion (mira 'Mis compras')");
        return;
    }
    n = cargar(s->usuario, ids, MAX_CARRITO);
    for (i = 0; i < n; i++) {
        if (ids[i] == m->arg) {
            resp_error(m, "Esa cancion ya esta en tu carrito");
            return;
        }
    }
    if (n >= MAX_CARRITO) {
        resp_error(m, "Tu carrito esta lleno");
        return;
    }
    ids[n++] = m->arg;
    if (guardar(s->usuario, ids, n) != 0) {
        resp_error(m, "No se pudo guardar el carrito");
        return;
    }
    resp_ok(m, "Cancion agregada al carrito");
}

void srv_quitar_carrito(Mensaje *m, const Sesion *s)
{
    int ids[MAX_CARRITO];
    int n, i, idx = -1;

    if (!es_cliente(m, s)) {
        return;
    }
    n = cargar(s->usuario, ids, MAX_CARRITO);
    for (i = 0; i < n; i++) {
        if (ids[i] == m->arg) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        resp_error(m, "Esa cancion no esta en tu carrito");
        return;
    }
    for (i = idx; i < n - 1; i++) {
        ids[i] = ids[i + 1];
    }
    n--;
    if (guardar(s->usuario, ids, n) != 0) {
        resp_error(m, "No se pudo guardar el carrito");
        return;
    }
    resp_ok(m, "Cancion quitada del carrito");
}

void srv_vaciar_carrito(Mensaje *m, const Sesion *s)
{
    if (!es_cliente(m, s)) {
        return;
    }
    guardar(s->usuario, NULL, 0);
    resp_ok(m, "Carrito vaciado");
}

/* Cobra el carrito del saldo del cliente, registra las ventas y lo vacia */
void srv_comprar(Mensaje *m, const Sesion *s)
{
    int ids[MAX_CARRITO];
    CancionSrv c;
    FILE *ventas;
    char fecha[16];
    char texto[256];
    time_t ahora = time(NULL);
    struct tm info;
    double total = 0.0, saldo;
    int n, i, vendidas = 0;

    if (!es_cliente(m, s)) {
        return;
    }
    n = cargar(s->usuario, ids, MAX_CARRITO);
    if (n == 0) {
        resp_error(m, "Tu carrito esta vacio");
        return;
    }

    /* Primero se calcula el total (sin contar lo que ya compro o ya no existe) */
    for (i = 0; i < n; i++) {
        if (biblioteca_buscar(ids[i], &c) && !cancion_comprada(s->usuario, ids[i])) {
            total += c.precio;
            vendidas++;
        }
    }
    if (vendidas == 0) {
        guardar(s->usuario, NULL, 0);
        resp_error(m, "No hay canciones nuevas para comprar");
        return;
    }

    saldo = saldo_obtener(s->usuario);
    if (total > saldo + 0.001) {
        snprintf(texto, sizeof(texto), "Saldo insuficiente: total $%.2f, tienes $%.2f", total, saldo);
        resp_error(m, texto);
        return;
    }

    ventas = fopen(ARCH_VENTAS, "a");
    if (ventas == NULL) {
        resp_error(m, "No se pudo registrar la venta");
        return;
    }
    localtime_r(&ahora, &info);
    strftime(fecha, sizeof(fecha), "%Y-%m-%d", &info);

    for (i = 0; i < n; i++) {
        if (biblioteca_buscar(ids[i], &c) && !cancion_comprada(s->usuario, ids[i])) {
            fprintf(ventas, "%s;%s;%d;%s;%.2f\n", fecha, s->usuario, c.id, c.titulo, c.precio);
            fflush(ventas);   /* para que cancion_comprada vea la venta de inmediato */
        }
    }
    fclose(ventas);

    saldo_fijar(s->usuario, saldo - total);
    guardar(s->usuario, NULL, 0);

    snprintf(texto, sizeof(texto), "Compra realizada: %d %s, total $%.2f. Saldo: $%.2f",
             vendidas, vendidas == 1 ? "cancion" : "canciones", total, saldo - total);
    resp_ok(m, texto);
}

/* Canciones que el usuario ya compro: id;titulo;artista;fecha;precio */
void srv_ver_compras(Mensaje *m, const Sesion *s)
{
    FILE *archivo;
    char linea[512];
    char fila[400];
    int vistos[MAX_COMPRAS];
    int nvistos = 0;

    if (!es_cliente(m, s)) {
        return;
    }
    archivo = fopen(ARCH_VENTAS, "r");
    if (archivo != NULL) {
        while (fgets(linea, sizeof(linea), archivo)) {
            char fecha[16], dueno[24], titulo[64];
            int id, i, repetida = 0;
            double precio;
            CancionSrv c;
            const char *artista;

            if (sscanf(linea, "%10[^;];%20[^;];%d;%63[^;];%lf",
                       fecha, dueno, &id, titulo, &precio) != 5 ||
                strcmp(dueno, s->usuario) != 0) {
                continue;
            }
            for (i = 0; i < nvistos; i++) {
                if (vistos[i] == id) {
                    repetida = 1;
                    break;
                }
            }
            if (repetida || nvistos >= MAX_COMPRAS) {
                continue;
            }
            vistos[nvistos++] = id;

            artista = biblioteca_buscar(id, &c) ? c.artista : "-";
            snprintf(fila, sizeof(fila), "%d;%.63s;%.63s;%.10s;%.2f\n",
                     id, titulo, artista, fecha, precio);
            if (datos_agregar(m, fila) != 0) {
                break;
            }
        }
        fclose(archivo);
    }
    resp_ok(m, "OK");
}

void srv_ver_saldo(Mensaje *m, const Sesion *s)
{
    char fila[64];

    if (!es_cliente(m, s)) {
        return;
    }
    snprintf(fila, sizeof(fila), "%.2f", saldo_obtener(s->usuario));
    datos_agregar(m, fila);
    resp_ok(m, "OK");
}