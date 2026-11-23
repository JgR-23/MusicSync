#include "ipc_cliente.h"

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static MemoriaCompartida *mem = NULL;
static sem_t *sem_mutex     = SEM_FAILED;
static sem_t *sem_solicitud = SEM_FAILED;
static sem_t *sem_respuesta = SEM_FAILED;

/* Copia privada de la ultima respuesta (la de la memoria compartida se reutiliza) */
static Respuesta respuesta_local;

static char usuario_actual[IPC_MAX_USUARIO] = "";
static char ultimo_error[IPC_MAX_MENSAJE]   = "";

/* -------------------------------------------------- */
/* Utilidades                                         */
/* -------------------------------------------------- */

static void poner_error(const char *mensaje){
    snprintf(ultimo_error, sizeof(ultimo_error), "%s", mensaje);
}

const char *ipc_ultimo_error(void){
    return ultimo_error;
}

static void copiar(char *destino, size_t tam, const char *origen){
    snprintf(destino, tam, "%s", origen != NULL ? origen : "");
}

/* sem_wait con limite de tiempo, para no quedar colgado si el servidor muere */
static int esperar_semaforo(sem_t *sem, int segundos){
    struct timespec limite;

    clock_gettime(CLOCK_REALTIME, &limite);
    limite.tv_sec += segundos;

    while(sem_timedwait(sem, &limite) < 0){
        if(errno == EINTR){
            continue;
        }
        return -1;
    }

    return 0;
}

/* -------------------------------------------------- */
/* Conexion                                           */
/* -------------------------------------------------- */

void ipc_desconectar(void){
    if(sem_mutex != SEM_FAILED){
        sem_close(sem_mutex);
        sem_mutex = SEM_FAILED;
    }
    if(sem_solicitud != SEM_FAILED){
        sem_close(sem_solicitud);
        sem_solicitud = SEM_FAILED;
    }
    if(sem_respuesta != SEM_FAILED){
        sem_close(sem_respuesta);
        sem_respuesta = SEM_FAILED;
    }
    if(mem != NULL){
        munmap(mem, sizeof(MemoriaCompartida));
        mem = NULL;
    }

    usuario_actual[0] = '\0';
}

int ipc_conectar(void){
    struct stat st;
    int fd;

    if(mem != NULL){
        return 0;
    }

    fd = shm_open(IPC_NOMBRE_SHM, O_RDWR, 0);
    if(fd < 0){
        poner_error("El servidor no esta en ejecucion");
        return -1;
    }

    if(fstat(fd, &st) < 0 || (size_t)st.st_size < sizeof(MemoriaCompartida)){
        close(fd);
        poner_error("El servidor se esta iniciando, intenta de nuevo");
        return -1;
    }

    mem = mmap(NULL, sizeof(MemoriaCompartida), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if(mem == MAP_FAILED){
        mem = NULL;
        poner_error("No se pudo abrir la memoria compartida");
        return -1;
    }

    /* La memoria puede ser de un servidor que ya murio */
    if(mem->pid_servidor <= 0 || (kill(mem->pid_servidor, 0) < 0 && errno == ESRCH)){
        ipc_desconectar();
        poner_error("El servidor no esta en ejecucion");
        return -1;
    }

    sem_mutex     = sem_open(IPC_SEM_MUTEX, 0);
    sem_solicitud = sem_open(IPC_SEM_SOLICITUD, 0);
    sem_respuesta = sem_open(IPC_SEM_RESPUESTA, 0);

    if(sem_mutex == SEM_FAILED || sem_solicitud == SEM_FAILED || sem_respuesta == SEM_FAILED){
        ipc_desconectar();
        poner_error("No se pudieron abrir los semaforos del servidor");
        return -1;
    }

    return 0;
}

/* -------------------------------------------------- */
/* Envio de una solicitud y espera de su respuesta    */
/* -------------------------------------------------- */

static int ejecutar(Solicitud *s){
    if(mem == NULL){
        poner_error("Sin conexion con el servidor");
        return -1;
    }

    /* Exclusion mutua: solo un cliente usa el buzon a la vez */
    if(esperar_semaforo(sem_mutex, 2 * IPC_TIMEOUT_SEG) < 0){
        poner_error("El servidor esta ocupado");
        return -1;
    }

    s->seq = ++mem->contador_seq;
    mem->solicitud = *s;
    sem_post(sem_solicitud);

    while(1){
        if(esperar_semaforo(sem_respuesta, IPC_TIMEOUT_SEG) < 0){
            sem_post(sem_mutex);
            poner_error("El servidor no responde");
            return -1;
        }

        /* Si un cliente anterior se rindio por tiempo, su respuesta tardia se descarta */
        __sync_synchronize();
        if(mem->respuesta.seq == s->seq){
            break;
        }
    }

    memcpy(&respuesta_local, &mem->respuesta, sizeof(respuesta_local));
    sem_post(sem_mutex);

    if(respuesta_local.estado != RESP_OK){
        poner_error(respuesta_local.mensaje[0] != '\0' ? respuesta_local.mensaje : "Error desconocido");
        return -1;
    }

    return 0;
}

static void preparar(Solicitud *s, int operacion){
    memset(s, 0, sizeof(*s));
    s->operacion = operacion;
    copiar(s->usuario, sizeof(s->usuario), usuario_actual);
}

/* -------------------------------------------------- */
/* Sesion                                             */
/* -------------------------------------------------- */

int ipc_login(const char *usuario, const char *clave){
    Solicitud s;

    preparar(&s, OP_LOGIN);
    copiar(s.usuario, sizeof(s.usuario), usuario);
    copiar(s.clave, sizeof(s.clave), clave);

    if(ejecutar(&s) < 0){
        return -1;
    }

    copiar(usuario_actual, sizeof(usuario_actual), usuario);
    return 0;
}

int ipc_registrar(const char *usuario, const char *correo, const char *clave){
    Solicitud s;

    preparar(&s, OP_REGISTRO);
    copiar(s.usuario, sizeof(s.usuario), usuario);
    copiar(s.correo, sizeof(s.correo), correo);
    copiar(s.clave, sizeof(s.clave), clave);

    return ejecutar(&s);
}

void ipc_logout(void){
    Solicitud s;

    if(usuario_actual[0] != '\0'){
        preparar(&s, OP_LOGOUT);
        ejecutar(&s);   /* solo para que el servidor lo anote en su bitacora */
    }

    usuario_actual[0] = '\0';
}

/* -------------------------------------------------- */
/* Biblioteca y playlists                             */
/* -------------------------------------------------- */

int ipc_listar_catalogo(RegCancion *salida, int max){
    Solicitud s;
    int n;

    preparar(&s, OP_LISTAR_CATALOGO);
    if(ejecutar(&s) < 0){
        return -1;
    }

    n = respuesta_local.cantidad;
    if(n > max){
        n = max;
    }
    if(n > IPC_MAX_CATALOGO){
        n = IPC_MAX_CATALOGO;
    }

    memcpy(salida, respuesta_local.canciones, (size_t)n * sizeof(RegCancion));
    return n;
}

int ipc_listar_playlists(RegPlaylist *salida, int max){
    Solicitud s;
    int n;

    preparar(&s, OP_LISTAR_PLAYLISTS);
    if(ejecutar(&s) < 0){
        return -1;
    }

    n = respuesta_local.cantidad;
    if(n > max){
        n = max;
    }
    if(n > IPC_MAX_PLAYLISTS){
        n = IPC_MAX_PLAYLISTS;
    }

    memcpy(salida, respuesta_local.playlists, (size_t)n * sizeof(RegPlaylist));
    return n;
}

int ipc_crear_playlist(const char *nombre){
    Solicitud s;

    preparar(&s, OP_CREAR_PLAYLIST);
    copiar(s.playlist, sizeof(s.playlist), nombre);

    return ejecutar(&s);
}

int ipc_eliminar_playlist(const char *nombre){
    Solicitud s;

    preparar(&s, OP_ELIMINAR_PLAYLIST);
    copiar(s.playlist, sizeof(s.playlist), nombre);

    return ejecutar(&s);
}

int ipc_listar_canciones_playlist(const char *nombre, RegCancion *salida, int max){
    Solicitud s;
    int n;

    preparar(&s, OP_LISTAR_CANCIONES);
    copiar(s.playlist, sizeof(s.playlist), nombre);

    if(ejecutar(&s) < 0){
        return -1;
    }

    n = respuesta_local.cantidad;
    if(n > max){
        n = max;
    }
    if(n > IPC_MAX_CATALOGO){
        n = IPC_MAX_CATALOGO;
    }

    memcpy(salida, respuesta_local.canciones, (size_t)n * sizeof(RegCancion));
    return n;
}

int ipc_agregar_cancion(const char *nombre, int id_cancion){
    Solicitud s;

    preparar(&s, OP_AGREGAR_CANCION);
    copiar(s.playlist, sizeof(s.playlist), nombre);
    s.id_cancion = id_cancion;

    return ejecutar(&s);
}

int ipc_quitar_cancion(const char *nombre, int id_cancion){
    Solicitud s;

    preparar(&s, OP_QUITAR_CANCION);
    copiar(s.playlist, sizeof(s.playlist), nombre);
    s.id_cancion = id_cancion;

    return ejecutar(&s);
}
