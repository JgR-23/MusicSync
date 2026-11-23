#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <sys/types.h>

/* ------------------------------------------------------------------ */
/*  Llaves y permisos de IPC                                          */
/*  El archivo "archivo" debe existir en el directorio de trabajo     */
/*  (el Servidor lo crea si no existe; "make" tambien).               */
/* ------------------------------------------------------------------ */
#define RUTA_LLAVE        "archivo"
#define ID_LLAVE_GENERAL  'k'
#define PERMISOS          0666

/* Semaforos del canal GENERAL (conexion inicial de los clientes) */
#define SEM_G_SOLICITUD   0   /* el cliente avisa al servidor            */
#define SEM_G_RESPUESTA   1   /* el servidor avisa que ya asigno canal   */
#define SEM_G_MUTEX       2   /* un solo cliente a la vez en el saludo   */
#define NUM_SEM_GENERAL   3

/* Semaforos del canal PRIVADO (un cliente <-> su hilo) */
#define SEM_P_PETICION    0   /* cliente -> hilo */
#define SEM_P_RESPUESTA   1   /* hilo -> cliente */
#define NUM_SEM_PRIVADO   2

/* Tamanos */
#define LON_USUARIO   21
#define LON_CORREO    64
#define LON_HASH      32
#define LON_NOMBRE    50
#define LON_MENSAJE   128
#define LON_DATOS     16384
#define LON_ENTRADA   320     /* datos compuestos que envia el cliente (ej. una cancion) */

/* Limites del sistema */
#define MAX_CLIENTES        20
#define MAX_PLAYLISTS_USR   20
#define MAX_CANC_PLAYLIST   20

/* Tipos de reporte de ventas (campo "arg" de ACC_ADM_REPORTE) */
#define REPORTE_DIARIO   1
#define REPORTE_SEMANAL  2
#define REPORTE_MENSUAL  3

/* Codigos de respuesta */
#define RESP_OK     0
#define RESP_ERROR  1

/* Acciones que puede pedir un cliente */
enum Accion {
    ACC_NINGUNA = 0,
    ACC_LOGIN,
    ACC_REGISTRO,
    ACC_CERRAR_SESION,
    ACC_VER_BIBLIOTECA,
    ACC_VER_PLAYLISTS,
    ACC_CREAR_PLAYLIST,
    ACC_ELIMINAR_PLAYLIST,
    ACC_VER_CANCIONES_PL,
    ACC_AGREGAR_A_PL,
    ACC_QUITAR_DE_PL,
    ACC_DESCONECTAR,
    ACC_LOGIN_ADMIN,
    ACC_VER_PERFIL,
    ACC_MODIFICAR_PERFIL,
    ACC_VER_CARRITO,
    ACC_AGREGAR_CARRITO,
    ACC_QUITAR_CARRITO,
    ACC_VACIAR_CARRITO,
    ACC_COMPRAR,
    ACC_ADM_VER_USUARIOS,
    ACC_ADM_CREAR_USUARIO,
    ACC_ADM_MODIFICAR_USUARIO,
    ACC_ADM_ELIMINAR_USUARIO,
    ACC_ADM_AGREGAR_CANCION,
    ACC_ADM_MODIFICAR_CANCION,
    ACC_ADM_ELIMINAR_CANCION,
    ACC_ADM_REPORTE,
    ACC_TOTAL
};

static inline const char *nombre_accion(int a)
{
    static const char *nombres[ACC_TOTAL] = {
        "NINGUNA", "LOGIN", "REGISTRO", "CERRAR_SESION", "VER_BIBLIOTECA",
        "VER_PLAYLISTS", "CREAR_PLAYLIST", "ELIMINAR_PLAYLIST",
        "VER_CANCIONES_PLAYLIST", "AGREGAR_A_PLAYLIST", "QUITAR_DE_PLAYLIST",
        "DESCONECTAR", "LOGIN_ADMIN", "VER_PERFIL", "MODIFICAR_PERFIL",
        "VER_CARRITO", "AGREGAR_AL_CARRITO", "QUITAR_DEL_CARRITO",
        "VACIAR_CARRITO", "COMPRAR", "ADMIN_VER_USUARIOS",
        "ADMIN_CREAR_USUARIO", "ADMIN_MODIFICAR_USUARIO",
        "ADMIN_ELIMINAR_USUARIO", "ADMIN_AGREGAR_CANCION",
        "ADMIN_MODIFICAR_CANCION", "ADMIN_ELIMINAR_CANCION", "ADMIN_REPORTE"
    };

    if (a < 0 || a >= ACC_TOTAL) {
        return "DESCONOCIDA";
    }
    return nombres[a];
}

/* Memoria compartida GENERAL: solo sirve para el "saludo" inicial */
typedef struct {
    pid_t pid_servidor;
    pid_t pid_cliente;
    int   estado;                 /* 0 = aceptado, -1 = rechazado */
    int   shmid_priv;             /* id de la memoria privada asignada */
    int   semid_priv;             /* id del conjunto de semaforos privado */
    char  mensaje[LON_MENSAJE];
} Conexion;

/* Memoria compartida PRIVADA: peticion y respuesta cliente <-> hilo.
 * Los datos de varias filas viajan en "datos": una fila por linea
 * (separada con '\n') y los campos de la fila separados con ';'.     */
typedef struct {
    int  accion;
    int  codigo;                  /* RESP_OK / RESP_ERROR (respuesta) */
    int  arg;                     /* argumento numerico (id, indice)  */
    char usuario[LON_USUARIO];
    char correo[LON_CORREO];
    char hash[LON_HASH];          /* contrasena ya cifrada por el cliente */
    char hash_nuevo[LON_HASH];    /* contrasena nueva (cifrada), si se cambia */
    char texto[LON_NOMBRE];       /* argumento de texto (nombre de playlist, usuario objetivo) */
    char entrada[LON_ENTRADA];    /* datos compuestos separados con ';' */
    char mensaje[LON_MENSAJE];    /* mensaje de respuesta para el usuario */
    char datos[LON_DATOS];        /* respuesta con varias filas */
} Mensaje;

#endif
