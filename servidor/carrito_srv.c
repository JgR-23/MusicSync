#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "biblioteca.h"
#include "carrito_srv.h"
#include "respuesta.h"

/* Carrito por usuario: datos/carrito_<usuario>.txt  (un id de cancion por linea)
 * Ventas: datos/ventas.txt  (fecha;usuario;id;titulo;precio)                   */

#define MAX_CARRITO   50
#define ARCH_VENTAS   "datos/ventas.txt"

static void ruta_carrito(const char *usuario, char *destino, size_t tam)
{
    snprintf(destino, tam, "datos/carrito_%s.txt", usuario);
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

void srv_ver_carrito(Mensaje *m, const Sesion *s)
{
    int ids[MAX_CARRITO];
    CancionSrv c;
    char fila[320];
    int n, i;

    if (!es_cliente(m, s)) {
        return;
    }
    n = cargar(s->usuario, ids, MAX_CARRITO);
    for (i = 0; i < n; i++) {
        if (biblioteca_buscar(ids[i], &c)) {
            snprintf(fila, sizeof(fila), "%d;%s;%s;%.2f\n",
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

/* Registra cada cancion del carrito como venta y vacia el carrito */
void srv_comprar(Mensaje *m, const Sesion *s)
{
    int ids[MAX_CARRITO];
    CancionSrv c;
    FILE *ventas;
    char fecha[16];
    char texto[LON_MENSAJE];
    time_t ahora = time(NULL);
    struct tm info;
    double total = 0.0;
    int n, i, vendidas = 0;

    if (!es_cliente(m, s)) {
        return;
    }
    n = cargar(s->usuario, ids, MAX_CARRITO);
    if (n == 0) {
        resp_error(m, "Tu carrito esta vacio");
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
        if (biblioteca_buscar(ids[i], &c)) {
            fprintf(ventas, "%s;%s;%d;%s;%.2f\n", fecha, s->usuario, c.id, c.titulo, c.precio);
            total += c.precio;
            vendidas++;
        }
    }
    fclose(ventas);

    if (vendidas == 0) {
        guardar(s->usuario, NULL, 0);
        resp_error(m, "Las canciones del carrito ya no estan disponibles");
        return;
    }
    guardar(s->usuario, NULL, 0);

    snprintf(texto, sizeof(texto), "Compra realizada: %d canciones, total $%.2f", vendidas, total);
    resp_ok(m, texto);
}
