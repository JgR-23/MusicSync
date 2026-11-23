/*
 * Servidor de MusicSync.
 *
 * Se ejecuta en su propia terminal (desde la raiz del proyecto, donde esta la
 * carpeta datos/):   ./servidor
 *
 * Crea la memoria compartida y los semaforos, y atiende las solicitudes de los
 * clientes una por una. Ctrl+C lo detiene y limpia los objetos IPC.
 */

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "comun.h"
#include "almacen.h"

static volatile sig_atomic_t corriendo = 1;

static MemoriaCompartida *mem = NULL;
static sem_t *sem_mutex     = SEM_FAILED;
static sem_t *sem_solicitud = SEM_FAILED;
static sem_t *sem_respuesta = SEM_FAILED;

/* -------------------------------------------------- */
/* Bitacora en la terminal del servidor               */
/* -------------------------------------------------- */

static const char *nombre_operacion(int op){
    switch(op){
        case OP_LOGIN:             return "LOGIN";
        case OP_REGISTRO:          return "REGISTRO";
        case OP_LOGOUT:            return "LOGOUT";
        case OP_LISTAR_CATALOGO:   return "LISTAR_CATALOGO";
        case OP_LISTAR_PLAYLISTS:  return "LISTAR_PLAYLISTS";
        case OP_CREAR_PLAYLIST:    return "CREAR_PLAYLIST";
        case OP_ELIMINAR_PLAYLIST: return "ELIMINAR_PLAYLIST";
        case OP_LISTAR_CANCIONES:  return "LISTAR_CANCIONES";
        case OP_AGREGAR_CANCION:   return "AGREGAR_CANCION";
        case OP_QUITAR_CANCION:    return "QUITAR_CANCION";
        default:                   return "DESCONOCIDA";
    }
}

static void registrar_log(const Solicitud *s, int rc, const char *detalle, const char *err){
    time_t ahora = time(NULL);
    struct tm *tm_local = localtime(&ahora);
    char hora[16];

    strftime(hora, sizeof(hora), "%H:%M:%S", tm_local);

    printf("[%s] %-10s %-18s %s", hora, s->usuario[0] != '\0' ? s->usuario : "-", nombre_operacion(s->operacion), rc == 0 ? "OK" : "ERROR");

    if(detalle[0] != '\0'){
        printf("  %s", detalle);
    }
    if(rc != 0){
        printf("  (%s)", err);
    }

    printf("\n");
    fflush(stdout);
}

/* -------------------------------------------------- */
/* Atencion de una solicitud                          */
/* -------------------------------------------------- */

static void procesar(const Solicitud *s, Respuesta *r){
    char err[IPC_MAX_MENSAJE] = "";
    char detalle[IPC_MAX_MENSAJE] = "";
    int rc = 0;
    int n;

    memset(r, 0, sizeof(*r));

    if(s->operacion != OP_LOGIN && s->operacion != OP_REGISTRO && s->usuario[0] == '\0'){
        snprintf(err, sizeof(err), "Sesion no iniciada");
        rc = -1;
    }
    else{
        switch(s->operacion){

            case OP_LOGIN:
                rc = almacen_validar_usuario(s->usuario, s->clave, err, sizeof(err));
                break;

            case OP_REGISTRO:
                snprintf(detalle, sizeof(detalle), "%s", s->correo);
                rc = almacen_registrar_usuario(s->usuario, s->correo, s->clave, err, sizeof(err));
                break;

            case OP_LOGOUT:
                break;

            case OP_LISTAR_CATALOGO:
                n = almacen_listar_catalogo(r->canciones, IPC_MAX_CATALOGO, err, sizeof(err));
                if(n < 0){
                    rc = -1;
                }
                else{
                    r->cantidad = n;
                    snprintf(detalle, sizeof(detalle), "%d canciones", n);
                }
                break;

            case OP_LISTAR_PLAYLISTS:
                n = almacen_listar_playlists(s->usuario, r->playlists, IPC_MAX_PLAYLISTS);
                r->cantidad = n;
                snprintf(detalle, sizeof(detalle), "%d playlists", n);
                break;

            case OP_CREAR_PLAYLIST:
                snprintf(detalle, sizeof(detalle), "\"%s\"", s->playlist);
                rc = almacen_crear_playlist(s->usuario, s->playlist, err, sizeof(err));
                break;

            case OP_ELIMINAR_PLAYLIST:
                snprintf(detalle, sizeof(detalle), "\"%s\"", s->playlist);
                rc = almacen_eliminar_playlist(s->usuario, s->playlist, err, sizeof(err));
                break;

            case OP_LISTAR_CANCIONES:
                snprintf(detalle, sizeof(detalle), "\"%s\"", s->playlist);
                n = almacen_listar_canciones_playlist(s->usuario, s->playlist, r->canciones, IPC_MAX_CANCIONES_PLAYLIST, err, sizeof(err));
                if(n < 0){
                    rc = -1;
                }
                else{
                    r->cantidad = n;
                }
                break;

            case OP_AGREGAR_CANCION:
                snprintf(detalle, sizeof(detalle), "\"%s\" (cancion %d)", s->playlist, s->id_cancion);
                rc = almacen_agregar_cancion(s->usuario, s->playlist, s->id_cancion, err, sizeof(err));
                break;

            case OP_QUITAR_CANCION:
                snprintf(detalle, sizeof(detalle), "\"%s\" (cancion %d)", s->playlist, s->id_cancion);
                rc = almacen_quitar_cancion(s->usuario, s->playlist, s->id_cancion, err, sizeof(err));
                break;

            default:
                snprintf(err, sizeof(err), "Operacion desconocida");
                rc = -1;
                break;
        }
    }

    if(rc < 0){
        r->estado = RESP_ERROR;
        snprintf(r->mensaje, sizeof(r->mensaje), "%s", err);
    }
    else{
        r->estado = RESP_OK;
    }

    registrar_log(s, rc < 0 ? -1 : 0, detalle, err);

    /* seq va al final: el cliente solo acepta la respuesta cuando ya esta completa */
    __sync_synchronize();
    r->seq = s->seq;
}

/* Las cadenas vienen de memoria compartida: se fuerza el '\0' final */
static void terminar_cadenas(Solicitud *s){
    s->usuario[sizeof(s->usuario) - 1]   = '\0';
    s->correo[sizeof(s->correo) - 1]     = '\0';
    s->clave[sizeof(s->clave) - 1]       = '\0';
    s->playlist[sizeof(s->playlist) - 1] = '\0';
}

/* -------------------------------------------------- */
/* Memoria compartida y semaforos                     */
/* -------------------------------------------------- */

static void manejar_senal(int sig){
    (void)sig;
    corriendo = 0;
}

/* Devuelve 1 si ya hay otro servidor vivo usando la memoria compartida */
static int servidor_ya_activo(void){
    int fd = shm_open(IPC_NOMBRE_SHM, O_RDWR, 0);
    struct stat st;
    pid_t pid = 0;

    if(fd < 0){
        return 0;
    }

    if(fstat(fd, &st) == 0 && (size_t)st.st_size >= sizeof(MemoriaCompartida)){
        MemoriaCompartida *m = mmap(NULL, sizeof(MemoriaCompartida), PROT_READ, MAP_SHARED, fd, 0);

        if(m != MAP_FAILED){
            pid = m->pid_servidor;
            munmap(m, sizeof(MemoriaCompartida));
        }
    }

    close(fd);

    if(pid > 0 && (kill(pid, 0) == 0 || errno == EPERM)){
        fprintf(stderr, "Ya hay un servidor en ejecucion (PID %d).\n", (int)pid);
        return 1;
    }

    return 0;
}

static void cerrar_ipc(void){
    if(sem_mutex != SEM_FAILED){
        sem_close(sem_mutex);
        sem_unlink(IPC_SEM_MUTEX);
    }
    if(sem_solicitud != SEM_FAILED){
        sem_close(sem_solicitud);
        sem_unlink(IPC_SEM_SOLICITUD);
    }
    if(sem_respuesta != SEM_FAILED){
        sem_close(sem_respuesta);
        sem_unlink(IPC_SEM_RESPUESTA);
    }
    if(mem != NULL){
        munmap(mem, sizeof(MemoriaCompartida));
        shm_unlink(IPC_NOMBRE_SHM);
    }
}

static int iniciar_ipc(void){
    int fd;

    /* Limpia restos de una ejecucion anterior que no termino bien */
    sem_unlink(IPC_SEM_MUTEX);
    sem_unlink(IPC_SEM_SOLICITUD);
    sem_unlink(IPC_SEM_RESPUESTA);
    shm_unlink(IPC_NOMBRE_SHM);

    sem_mutex     = sem_open(IPC_SEM_MUTEX,     O_CREAT | O_EXCL, 0600, 1);
    sem_solicitud = sem_open(IPC_SEM_SOLICITUD, O_CREAT | O_EXCL, 0600, 0);
    sem_respuesta = sem_open(IPC_SEM_RESPUESTA, O_CREAT | O_EXCL, 0600, 0);

    if(sem_mutex == SEM_FAILED || sem_solicitud == SEM_FAILED || sem_respuesta == SEM_FAILED){
        perror("sem_open");
        return -1;
    }

    fd = shm_open(IPC_NOMBRE_SHM, O_CREAT | O_EXCL | O_RDWR, 0600);
    if(fd < 0){
        perror("shm_open");
        return -1;
    }

    if(ftruncate(fd, sizeof(MemoriaCompartida)) < 0){
        perror("ftruncate");
        close(fd);
        shm_unlink(IPC_NOMBRE_SHM);
        return -1;
    }

    mem = mmap(NULL, sizeof(MemoriaCompartida), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if(mem == MAP_FAILED){
        perror("mmap");
        mem = NULL;
        shm_unlink(IPC_NOMBRE_SHM);
        return -1;
    }

    memset(mem, 0, sizeof(MemoriaCompartida));
    mem->pid_servidor = getpid();   /* se escribe al final: indica "servidor listo" */

    return 0;
}

int main(void){
    struct sigaction sa;
    Solicitud copia;

    if(servidor_ya_activo()){
        return 1;
    }

    if(access("datos", F_OK) != 0){
        fprintf(stderr, "No se encontro la carpeta datos/. Ejecuta el servidor desde la raiz del proyecto.\n");
        return 1;
    }

    /* Sin SA_RESTART: sem_wait() regresa con EINTR al recibir Ctrl+C */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = manejar_senal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if(iniciar_ipc() < 0){
        cerrar_ipc();
        return 1;
    }

    printf("MusicSync - servidor iniciado (PID %d)\n", (int)getpid());
    printf("Memoria compartida: %s | Semaforos: %s, %s, %s\n", IPC_NOMBRE_SHM, IPC_SEM_MUTEX, IPC_SEM_SOLICITUD, IPC_SEM_RESPUESTA);
    printf("Esperando clientes... (Ctrl+C para detener)\n\n");
    fflush(stdout);

    while(corriendo){
        if(sem_wait(sem_solicitud) < 0){
            if(errno == EINTR){
                continue;
            }
            perror("sem_wait");
            break;
        }

        copia = mem->solicitud;
        terminar_cadenas(&copia);

        procesar(&copia, &mem->respuesta);

        sem_post(sem_respuesta);
    }

    printf("\nServidor detenido.\n");
    cerrar_ipc();

    return 0;
}
