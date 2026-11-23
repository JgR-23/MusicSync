#include <stdio.h>
#include <string.h>
#include <time.h>

#include "reportes.h"
#include "respuesta.h"

#define ARCH_VENTAS "datos/ventas.txt"

/* Agrega texto a un buffer con limite; marca "truncado" si ya no cabe */
static void agregar(char *buf, size_t cap, const char *linea, int *truncado)
{
    size_t usado = strlen(buf);
    size_t largo = strlen(linea);

    if (usado + largo + 1 >= cap) {
        *truncado = 1;
        return;
    }
    memcpy(buf + usado, linea, largo + 1);
}

void srv_reporte(Mensaje *m, const Sesion *s)
{
    static char detalle[LON_DATOS];
    static char texto[LON_DATOS];
    char linea[256];
    char hoy[16], desde[16], mes[8];
    char nombre_archivo[48];
    const char *titulo, *archivo_rep;
    time_t ahora = time(NULL);
    time_t inicio;
    struct tm info;
    FILE *ventas, *salida;
    double total = 0.0;
    int cantidad = 0, truncado = 0;

    if (!s->logueado || !s->es_admin) {
        resp_error(m, "Operacion solo para el administrador");
        return;
    }
    if (m->arg != REPORTE_DIARIO && m->arg != REPORTE_SEMANAL && m->arg != REPORTE_MENSUAL) {
        resp_error(m, "Tipo de reporte invalido");
        return;
    }

    localtime_r(&ahora, &info);
    strftime(hoy, sizeof(hoy), "%Y-%m-%d", &info);
    strftime(mes, sizeof(mes), "%Y-%m", &info);
    inicio = ahora - 6 * 24 * 3600;           /* semanal: hoy y los 6 dias anteriores */
    localtime_r(&inicio, &info);
    strftime(desde, sizeof(desde), "%Y-%m-%d", &info);

    if (m->arg == REPORTE_DIARIO) {
        titulo = "REPORTE DIARIO";
        archivo_rep = "reporte_diario.txt";
        snprintf(desde, sizeof(desde), "%s", hoy);
    } else if (m->arg == REPORTE_SEMANAL) {
        titulo = "REPORTE SEMANAL";
        archivo_rep = "reporte_semanal.txt";
    } else {
        titulo = "REPORTE MENSUAL";
        archivo_rep = "reporte_mensual.txt";
        snprintf(desde, sizeof(desde), "%s-01", mes);
    }

    detalle[0] = '\0';
    ventas = fopen(ARCH_VENTAS, "r");
    if (ventas != NULL) {
        while (fgets(linea, sizeof(linea), ventas)) {
            char fecha[16], usuario[24], nombre[64];
            int id;
            double precio;
            char fila[200];

            if (sscanf(linea, "%10[^;];%20[^;];%d;%63[^;];%lf",
                       fecha, usuario, &id, nombre, &precio) != 5) {
                continue;
            }
            /* Las fechas AAAA-MM-DD se comparan bien como texto */
            if (strcmp(fecha, desde) < 0 || strcmp(fecha, hoy) > 0) {
                continue;
            }
            snprintf(fila, sizeof(fila), "%-10s  %-14.14s  %-28.28s  $%8.2f\n",
                     fecha, usuario, nombre, precio);
            agregar(detalle, sizeof(detalle), fila, &truncado);
            total += precio;
            cantidad++;
        }
        fclose(ventas);
    }

    /* Primero el resumen: asi nunca se pierde aunque el detalle se recorte */
    texto[0] = '\0';
    snprintf(linea, sizeof(linea), "===== %s =====\n", titulo);
    agregar(texto, sizeof(texto), linea, &truncado);
    snprintf(linea, sizeof(linea), "Periodo: %s  a  %s\n", desde, hoy);
    agregar(texto, sizeof(texto), linea, &truncado);
    snprintf(linea, sizeof(linea), "Canciones vendidas: %d\n", cantidad);
    agregar(texto, sizeof(texto), linea, &truncado);
    snprintf(linea, sizeof(linea), "TOTAL VENDIDO: $%.2f\n\n", total);
    agregar(texto, sizeof(texto), linea, &truncado);

    if (cantidad == 0) {
        agregar(texto, sizeof(texto), "No hay ventas en este periodo.\n", &truncado);
    } else {
        agregar(texto, sizeof(texto), "Fecha       Usuario         Cancion                       Precio\n", &truncado);
        agregar(texto, sizeof(texto), "---------------------------------------------------------------\n", &truncado);
        agregar(texto, sizeof(texto), detalle, &truncado);
    }
    if (truncado) {
        agregar(texto, sizeof(texto), "(... lista recortada; el archivo guardado tiene lo mismo)\n", &truncado);
    }

    snprintf(nombre_archivo, sizeof(nombre_archivo), "datos/%s", archivo_rep);
    salida = fopen(nombre_archivo, "w");
    if (salida != NULL) {
        fputs(texto, salida);
        fclose(salida);
    }

    datos_agregar(m, texto);
    resp_ok(m, "Reporte generado");
}
