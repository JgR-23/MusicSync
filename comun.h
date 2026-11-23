#ifndef COMUN_H
#define COMUN_H

#include <sys/types.h>

/* -------------------------------------------------- */
/* Objetos IPC (POSIX): memoria compartida y semaforos */
/*                                                    */
/*  Cliente:  mutex -> escribe solicitud -> post(sol) */
/*            -> wait(resp) -> lee respuesta -> post(mutex) */
/*  Servidor: wait(sol) -> procesa -> escribe         */
/*            respuesta -> post(resp)                 */
/* -------------------------------------------------- */

#define IPC_NOMBRE_SHM     "/musicsync_shm"
#define IPC_SEM_MUTEX      "/musicsync_mutex"      /* un cliente a la vez usa el buzon  */
#define IPC_SEM_SOLICITUD  "/musicsync_solicitud"  /* cliente  -> servidor: hay solicitud */
#define IPC_SEM_RESPUESTA  "/musicsync_respuesta"  /* servidor -> cliente:  hay respuesta */

#define IPC_TIMEOUT_SEG    5   /* espera maxima de una respuesta del servidor */

/* -------------------------------------------------- */
/* Limites                                            */
/* -------------------------------------------------- */

#define IPC_MAX_USUARIO    36
#define IPC_MAX_CORREO     36
#define IPC_MAX_CLAVE      36
#define IPC_MAX_TEXTO      64
#define IPC_MAX_DURACION   10
#define IPC_MAX_PLAYLIST   50
#define IPC_MAX_MENSAJE    120

#define IPC_MAX_CATALOGO            100
#define IPC_MAX_PLAYLISTS           20
#define IPC_MAX_CANCIONES_PLAYLIST  20

/* -------------------------------------------------- */
/* Operaciones                                        */
/* -------------------------------------------------- */

typedef enum {
    OP_NINGUNA = 0,
    OP_LOGIN,
    OP_REGISTRO,
    OP_LOGOUT,
    OP_LISTAR_CATALOGO,
    OP_LISTAR_PLAYLISTS,
    OP_CREAR_PLAYLIST,
    OP_ELIMINAR_PLAYLIST,
    OP_LISTAR_CANCIONES,
    OP_AGREGAR_CANCION,
    OP_QUITAR_CANCION
} Operacion;

#define RESP_OK     0
#define RESP_ERROR  1

/* -------------------------------------------------- */
/* Registros que viajan por la memoria compartida     */
/* -------------------------------------------------- */

typedef struct {
    int    id;
    char   titulo[IPC_MAX_TEXTO];
    char   artista[IPC_MAX_TEXTO];
    char   album[IPC_MAX_TEXTO];
    char   genero[IPC_MAX_TEXTO];
    char   duracion[IPC_MAX_DURACION];
    double precio;
} RegCancion;

typedef struct {
    char nombre[IPC_MAX_PLAYLIST];
    int  cantidad;
} RegPlaylist;

typedef struct {
    long seq;                          /* numero de solicitud (lo pone el cliente) */
    int  operacion;
    char usuario[IPC_MAX_USUARIO];     /* usuario de la sesion */
    char correo[IPC_MAX_CORREO];
    char clave[IPC_MAX_CLAVE];
    char playlist[IPC_MAX_PLAYLIST];
    int  id_cancion;
} Solicitud;

typedef struct {
    long        seq;                   /* se escribe AL FINAL: marca la respuesta como completa */
    int         estado;                /* RESP_OK o RESP_ERROR */
    char        mensaje[IPC_MAX_MENSAJE];
    int         cantidad;              /* elementos validos en canciones[] / playlists[] */
    RegCancion  canciones[IPC_MAX_CATALOGO];
    RegPlaylist playlists[IPC_MAX_PLAYLISTS];
} Respuesta;

/* Estructura completa que vive en la memoria compartida */
typedef struct {
    pid_t     pid_servidor;
    long      contador_seq;
    Solicitud solicitud;
    Respuesta respuesta;
} MemoriaCompartida;

#endif
