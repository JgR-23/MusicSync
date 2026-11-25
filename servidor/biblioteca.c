#include <stdio.h>
#include <string.h>

#include "biblioteca.h"
#include "respuesta.h"

/* Formato: id;titulo;artista;album;genero;duracion;precio */
#define ARCH_CANCIONES "datos/canciones.txt"

static int leer_linea(const char *linea, CancionSrv *c)
{
    memset(c, 0, sizeof(*c));
    return sscanf(linea, "%d;%63[^;];%63[^;];%63[^;];%63[^;];%9[^;];%lf",
                  &c->id, c->titulo, c->artista, c->album, c->genero,
                  c->duracion, &c->precio) == 7;
}

int biblioteca_buscar(int id, CancionSrv *salida)
{
    FILE *archivo = fopen(ARCH_CANCIONES, "r");
    char linea[512];
    CancionSrv c;
    int encontrada = 0;

    if (archivo == NULL) {
        return 0;
    }
    while (fgets(linea, sizeof(linea), archivo)) {
        if (leer_linea(linea, &c) && c.id == id) {
            *salida = c;
            encontrada = 1;
            break;
        }
    }
    fclose(archivo);
    return encontrada;
}

void srv_biblioteca(Mensaje *m, const Sesion *s)
{
    FILE *archivo;
    char linea[512];
    char fila[512];
    CancionSrv c;
    int total = 0;

    if (!s->logueado) {
        resp_error(m, "Inicia sesion primero");
        return;
    }

    archivo = fopen(ARCH_CANCIONES, "r");
    if (archivo == NULL) {
        resp_error(m, "No se pudo abrir la biblioteca");
        return;
    }

    while (total < MAX_BIBLIOTECA && fgets(linea, sizeof(linea), archivo)) {
        if (!leer_linea(linea, &c)) {
            continue;
        }
        snprintf(fila, sizeof(fila), "%d;%.63s;%.63s;%.63s;%.63s;%.9s;%.2f\n",
                 c.id, c.titulo, c.artista, c.album, c.genero,
                 c.duracion, c.precio);
        if (datos_agregar(m, fila) != 0) {
            break;
        }
        total++;
    }
    fclose(archivo);

    resp_ok(m, "OK");
}

int biblioteca_cargar(CancionSrv *lista, int max)
{
    FILE *archivo = fopen(ARCH_CANCIONES, "r");
    char linea[512];
    CancionSrv c;
    int n = 0;

    if (archivo == NULL) {
        return 0;
    }
    while (n < max && fgets(linea, sizeof(linea), archivo)) {
        if (leer_linea(linea, &c)) {
            lista[n++] = c;
        }
    }
    fclose(archivo);
    return n;
}

int biblioteca_guardar(const CancionSrv *lista, int n)
{
    FILE *archivo = fopen(ARCH_CANCIONES, "w");
    int i;

    if (archivo == NULL) {
        return -1;
    }
    for (i = 0; i < n; i++) {
        fprintf(archivo, "%d;%s;%s;%s;%s;%s;%.2f\n",
                lista[i].id, lista[i].titulo, lista[i].artista,
                lista[i].album, lista[i].genero, lista[i].duracion,
                lista[i].precio);
    }
    fclose(archivo);
    return 0;
}