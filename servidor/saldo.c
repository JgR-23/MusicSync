#include <stdio.h>
#include <string.h>
 
#include "ipc_util.h"
#include "protocolo.h"
#include "saldo.h"
 
#define ARCH_SALDOS   "datos/saldos.txt"
#define MAX_SALDOS    300
 
typedef struct {
    char   usuario[LON_USUARIO];
    double saldo;
} RegSaldo;
 
static int cargar(RegSaldo *lista, int max)
{
    FILE *archivo = fopen(ARCH_SALDOS, "r");
    char linea[128];
    int n = 0;
 
    if (archivo == NULL) {
        return 0;
    }
    while (n < max && fgets(linea, sizeof(linea), archivo)) {
        char usuario[LON_USUARIO];
        double saldo;
 
        if (sscanf(linea, "%20[^;];%lf", usuario, &saldo) == 2) {
            copiar_cadena(lista[n].usuario, sizeof(lista[n].usuario), usuario);
            lista[n].saldo = saldo;
            n++;
        }
    }
    fclose(archivo);
    return n;
}
 
static int guardar(const RegSaldo *lista, int n)
{
    FILE *archivo = fopen(ARCH_SALDOS, "w");
    int i;
 
    if (archivo == NULL) {
        return -1;
    }
    for (i = 0; i < n; i++) {
        fprintf(archivo, "%s;%.2f\n", lista[i].usuario, lista[i].saldo);
    }
    fclose(archivo);
    return 0;
}
 
static int buscar(const RegSaldo *lista, int n, const char *usuario)
{
    int i;
 
    for (i = 0; i < n; i++) {
        if (strcmp(lista[i].usuario, usuario) == 0) {
            return i;
        }
    }
    return -1;
}
 
int saldo_fijar(const char *usuario, double saldo)
{
    RegSaldo lista[MAX_SALDOS];
    int n = cargar(lista, MAX_SALDOS);
    int idx = buscar(lista, n, usuario);
 
    if (idx < 0) {
        if (n >= MAX_SALDOS) {
            return -1;
        }
        idx = n++;
        copiar_cadena(lista[idx].usuario, sizeof(lista[idx].usuario), usuario);
    }
    lista[idx].saldo = saldo;
    return guardar(lista, n);
}
 
double saldo_obtener(const char *usuario)
{
    RegSaldo lista[MAX_SALDOS];
    int n = cargar(lista, MAX_SALDOS);
    int idx = buscar(lista, n, usuario);
 
    if (idx >= 0) {
        return lista[idx].saldo;
    }
    saldo_fijar(usuario, SALDO_INICIAL);
    return SALDO_INICIAL;
}
 
void saldo_eliminar(const char *usuario)
{
    RegSaldo lista[MAX_SALDOS];
    int n = cargar(lista, MAX_SALDOS);
    int idx = buscar(lista, n, usuario);
    int i;
 
    if (idx < 0) {
        return;
    }
    for (i = idx; i < n - 1; i++) {
        lista[i] = lista[i + 1];
    }
    guardar(lista, n - 1);
}
 