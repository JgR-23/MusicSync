#ifndef IPC_CLIENTE_H_
#define IPC_CLIENTE_H_

#include "comun.h"

/*
 * API de comunicacion con el servidor (memoria compartida + semaforos).
 * Las funciones devuelven 0 (o una cantidad >= 0) si salio bien y -1 si hubo
 * error; el texto del error se obtiene con ipc_ultimo_error().
 * Despues de ipc_login() la sesion queda guardada aqui, asi que las demas
 * funciones no necesitan recibir el usuario.
 */

int         ipc_conectar(void);
void        ipc_desconectar(void);
const char *ipc_ultimo_error(void);

int  ipc_login(const char *usuario, const char *clave);
int  ipc_registrar(const char *usuario, const char *correo, const char *clave);
void ipc_logout(void);

int ipc_listar_catalogo(RegCancion *salida, int max);
int ipc_listar_playlists(RegPlaylist *salida, int max);
int ipc_crear_playlist(const char *nombre);
int ipc_eliminar_playlist(const char *nombre);
int ipc_listar_canciones_playlist(const char *nombre, RegCancion *salida, int max);
int ipc_agregar_cancion(const char *nombre, int id_cancion);
int ipc_quitar_cancion(const char *nombre, int id_cancion);

#endif // IPC_CLIENTE_H_
