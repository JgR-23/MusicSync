#ifndef SALDO_H
#define SALDO_H

/* Saldo con el que empieza cada cliente la primera vez que se consulta */
#define SALDO_INICIAL 500.00

/* Archivo datos/saldos.txt: una linea por usuario  ->  usuario;saldo */
double saldo_obtener(const char *usuario);          /* lo crea si no existe */
int    saldo_fijar(const char *usuario, double saldo);   /* 0 = ok, -1 = error */
void   saldo_eliminar(const char *usuario);

#endif