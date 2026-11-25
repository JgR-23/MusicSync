#ifndef COMUNICACION_H
#define COMUNICACION_H

#include "protocolo.h"

#define COM_OK            0
#define COM_SIN_SERVIDOR  1
#define COM_RECHAZADO     2

/* Zona de memoria privada: aqui se escribe la peticion y se lee la respuesta */
extern Mensaje *com_msg;

int         com_conectar(void);            /* COM_OK / COM_SIN_SERVIDOR / COM_RECHAZADO */
void        com_desconectar(void);
void        com_preparar(int accion);      /* limpia el mensaje y fija la accion */
int         com_transaccion(void);         /* envia y espera: RESP_OK, RESP_ERROR o -1 (conexion perdida) */
int         com_perdida(void);             /* 1 si se perdio la conexion */
const char *com_error(void);

/* Consulta el saldo del cliente. 0 = ok, 1 = error del servidor, -1 = conexion perdida */
int         com_saldo(double *saldo);

#endif