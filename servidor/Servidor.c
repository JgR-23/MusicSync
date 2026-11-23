#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>

#include "admin_srv.h"
#include "biblioteca.h"
#include "carrito_srv.h"
#include "ipc_util.h"
#include "log.h"
#include "playlists_srv.h"
#include "reportes.h"
#include "protocolo.h"
#include "sesion.h"
#include "usuarios.h"

static volatile sig_atomic_t activo = 1;

static int semid_general = -1;
static int shmid_general = -1;
static Conexion *conexion = NULL;

/* Un solo hilo a la vez toca los archivos .txt */
static pthread_mutex_t mutex_archivos = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t mutex_clientes = PTHREAD_MUTEX_INITIALIZER;
static int clientes_activos = 0;

typedef struct {
    pid_t pid;
    int   shmid;
    int   semid;
} ArgsHilo;

static void manejar_senal(int senal)
{
    (void)senal;
    activo = 0;
}

/* ------------------------------------------------------------------ */
/*  Archivos de datos                                                 */
/* ------------------------------------------------------------------ */

static void asegurar_archivo(const char *ruta)
{
    FILE *f = fopen(ruta, "a");

    if (f != NULL) {
        fclose(f);
    }
}

static void preparar_datos(void)
{
    mkdir("datos", 0755);
    asegurar_archivo(RUTA_LLAVE);
    asegurar_archivo("datos/usuarios.txt");
    asegurar_archivo("datos/canciones.txt");
    asegurar_archivo("datos/ventas.txt");
}

/* ------------------------------------------------------------------ */
/*  Atencion de peticiones                                            */
/* ------------------------------------------------------------------ */

static void despachar(Mensaje *m, Sesion *s, pid_t pid)
{
    char quien[64];

    /* La memoria es compartida: nunca confiamos en que vengan con '\0' */
    m->usuario[LON_USUARIO - 1] = '\0';
    m->correo[LON_CORREO - 1] = '\0';
    m->hash[LON_HASH - 1] = '\0';
    m->hash_nuevo[LON_HASH - 1] = '\0';
    m->texto[LON_NOMBRE - 1] = '\0';
    m->entrada[LON_ENTRADA - 1] = '\0';

    if (m->accion == ACC_LOGIN || m->accion == ACC_REGISTRO ||
        m->accion == ACC_ADM_CREAR_USUARIO) {
        snprintf(quien, sizeof(quien), "usuario '%s'", m->usuario);
    } else if (s->logueado) {
        snprintf(quien, sizeof(quien), "usuario '%s'", s->usuario);
    } else {
        snprintf(quien, sizeof(quien), "sin sesion");
    }
    log_evento("[PID %d] Solicita %s (%s)", (int)pid, nombre_accion(m->accion), quien);

    m->codigo = RESP_ERROR;
    m->mensaje[0] = '\0';
    m->datos[0] = '\0';

    pthread_mutex_lock(&mutex_archivos);
    switch (m->accion) {
        case ACC_LOGIN:             srv_login(m, s);              break;
        case ACC_REGISTRO:          srv_registro(m);              break;
        case ACC_CERRAR_SESION:     srv_cerrar_sesion(m, s);      break;
        case ACC_VER_BIBLIOTECA:    srv_biblioteca(m, s);         break;
        case ACC_VER_PLAYLISTS:     srv_ver_playlists(m, s);      break;
        case ACC_CREAR_PLAYLIST:    srv_crear_playlist(m, s);     break;
        case ACC_ELIMINAR_PLAYLIST: srv_eliminar_playlist(m, s);  break;
        case ACC_VER_CANCIONES_PL:  srv_ver_canciones_pl(m, s);   break;
        case ACC_AGREGAR_A_PL:      srv_agregar_a_pl(m, s);       break;
        case ACC_QUITAR_DE_PL:      srv_quitar_de_pl(m, s);       break;

        case ACC_LOGIN_ADMIN:       srv_login_admin(m, s);        break;
        case ACC_VER_PERFIL:        srv_ver_perfil(m, s);         break;
        case ACC_MODIFICAR_PERFIL:  srv_modificar_perfil(m, s);   break;

        case ACC_VER_CARRITO:       srv_ver_carrito(m, s);        break;
        case ACC_AGREGAR_CARRITO:   srv_agregar_carrito(m, s);    break;
        case ACC_QUITAR_CARRITO:    srv_quitar_carrito(m, s);     break;
        case ACC_VACIAR_CARRITO:    srv_vaciar_carrito(m, s);     break;
        case ACC_COMPRAR:           srv_comprar(m, s);            break;

        case ACC_ADM_VER_USUARIOS:      srv_adm_ver_usuarios(m, s);      break;
        case ACC_ADM_CREAR_USUARIO:     srv_adm_crear_usuario(m, s);     break;
        case ACC_ADM_MODIFICAR_USUARIO: srv_adm_modificar_usuario(m, s); break;
        case ACC_ADM_ELIMINAR_USUARIO:  srv_adm_eliminar_usuario(m, s);  break;
        case ACC_ADM_AGREGAR_CANCION:   srv_adm_agregar_cancion(m, s);   break;
        case ACC_ADM_MODIFICAR_CANCION: srv_adm_modificar_cancion(m, s); break;
        case ACC_ADM_ELIMINAR_CANCION:  srv_adm_eliminar_cancion(m, s);  break;
        case ACC_ADM_REPORTE:           srv_reporte(m, s);               break;
        default:
            m->codigo = RESP_ERROR;
            copiar_cadena(m->mensaje, sizeof(m->mensaje), "Accion desconocida");
            break;
    }
    pthread_mutex_unlock(&mutex_archivos);

    log_evento("[PID %d] Resultado %s: %s", (int)pid,
               m->codigo == RESP_OK ? "OK" : "ERROR", m->mensaje);
}

/* Hilo dedicado a un cliente: habla con el por su memoria privada */
static void *hilo_cliente(void *argumento)
{
    ArgsHilo args = *(ArgsHilo *)argumento;
    Mensaje *m;
    Sesion sesion;

    free(argumento);
    memset(&sesion, 0, sizeof(sesion));

    m = (Mensaje *)shmat(args.shmid, NULL, 0);
    if (m != (Mensaje *)-1) {
        log_evento("[PID %d] Cliente conectado (hilo atendiendolo)", (int)args.pid);

        while (activo) {
            int r = sem_down_timeout(args.semid, SEM_P_PETICION, 2);

            if (r == 1) {
                /* Sin peticiones: verificamos que el cliente siga vivo */
                if (kill(args.pid, 0) == -1 && errno == ESRCH) {
                    log_evento("[PID %d] El cliente dejo de existir", (int)args.pid);
                    break;
                }
                continue;
            }
            if (r < 0) {
                break;
            }
            if (m->accion == ACC_DESCONECTAR) {
                log_evento("[PID %d] Cliente desconectado", (int)args.pid);
                break;
            }
            despachar(m, &sesion, args.pid);
            sem_up(args.semid, SEM_P_RESPUESTA);
        }
        shmdt(m);
    }

    /* El hilo libera los recursos privados de su cliente */
    shmctl(args.shmid, IPC_RMID, NULL);
    semctl(args.semid, 0, IPC_RMID);

    pthread_mutex_lock(&mutex_clientes);
    clientes_activos--;
    pthread_mutex_unlock(&mutex_clientes);
    return NULL;
}

/* ------------------------------------------------------------------ */
/*  Canal general: alta de nuevos clientes                            */
/* ------------------------------------------------------------------ */

static void rechazar(const char *motivo)
{
    conexion->estado = -1;
    copiar_cadena(conexion->mensaje, sizeof(conexion->mensaje), motivo);
    sem_up(semid_general, SEM_G_RESPUESTA);
}

static void atender_nueva_conexion(void)
{
    pid_t pid = conexion->pid_cliente;
    int shm_priv, sem_priv;
    ArgsHilo *args;
    pthread_t hilo;
    pthread_attr_t atributos;

    pthread_mutex_lock(&mutex_clientes);
    if (clientes_activos >= MAX_CLIENTES) {
        pthread_mutex_unlock(&mutex_clientes);
        log_evento("[PID %d] Conexion rechazada: servidor lleno", (int)pid);
        rechazar("El servidor esta lleno, intenta mas tarde");
        return;
    }
    clientes_activos++;
    pthread_mutex_unlock(&mutex_clientes);

    shm_priv = shmget(IPC_PRIVATE, sizeof(Mensaje), IPC_CREAT | PERMISOS);
    sem_priv = semget(IPC_PRIVATE, NUM_SEM_PRIVADO, IPC_CREAT | PERMISOS);
    args = malloc(sizeof(*args));

    if (shm_priv == -1 || sem_priv == -1 || args == NULL) {
        goto fallo;
    }
    sem_fijar(sem_priv, SEM_P_PETICION, 0);
    sem_fijar(sem_priv, SEM_P_RESPUESTA, 0);

    args->pid = pid;
    args->shmid = shm_priv;
    args->semid = sem_priv;

    pthread_attr_init(&atributos);
    pthread_attr_setdetachstate(&atributos, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&hilo, &atributos, hilo_cliente, args) != 0) {
        pthread_attr_destroy(&atributos);
        goto fallo;
    }
    pthread_attr_destroy(&atributos);

    conexion->estado = 0;
    conexion->shmid_priv = shm_priv;
    conexion->semid_priv = sem_priv;
    conexion->mensaje[0] = '\0';
    sem_up(semid_general, SEM_G_RESPUESTA);
    return;

fallo:
    if (shm_priv != -1) shmctl(shm_priv, IPC_RMID, NULL);
    if (sem_priv != -1) semctl(sem_priv, 0, IPC_RMID);
    free(args);
    pthread_mutex_lock(&mutex_clientes);
    clientes_activos--;
    pthread_mutex_unlock(&mutex_clientes);
    log_evento("[PID %d] No se pudo crear el canal privado", (int)pid);
    rechazar("El servidor no pudo crear el canal de comunicacion");
}

/* Si quedo IPC de una ejecucion anterior que murio, lo borramos */
static void limpiar_residuos(key_t llave)
{
    int shm = shmget(llave, 0, 0);
    int sem;

    if (shm != -1) {
        Conexion *vieja = (Conexion *)shmat(shm, NULL, 0);

        if (vieja != (Conexion *)-1) {
            pid_t pid = vieja->pid_servidor;

            shmdt(vieja);
            if (pid > 0 && kill(pid, 0) == 0) {
                fprintf(stderr, "Ya hay un servidor activo (PID %d).\n", (int)pid);
                exit(1);
            }
        }
        shmctl(shm, IPC_RMID, NULL);
    }
    sem = semget(llave, NUM_SEM_GENERAL, 0);
    if (sem != -1) {
        semctl(sem, 0, IPC_RMID);
    }
}

int main(void)
{
    struct sigaction sa;
    key_t llave;
    int espera;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = manejar_senal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    preparar_datos();

    llave = ftok(RUTA_LLAVE, ID_LLAVE_GENERAL);
    if (llave == -1) {
        perror("ftok");
        return 1;
    }
    limpiar_residuos(llave);

    shmid_general = shmget(llave, sizeof(Conexion), IPC_CREAT | IPC_EXCL | PERMISOS);
    semid_general = semget(llave, NUM_SEM_GENERAL, IPC_CREAT | IPC_EXCL | PERMISOS);
    if (shmid_general == -1 || semid_general == -1) {
        perror("No se pudo crear la memoria o los semaforos");
        if (shmid_general != -1) shmctl(shmid_general, IPC_RMID, NULL);
        if (semid_general != -1) semctl(semid_general, 0, IPC_RMID);
        return 1;
    }

    conexion = (Conexion *)shmat(shmid_general, NULL, 0);
    if (conexion == (Conexion *)-1) {
        perror("shmat");
        shmctl(shmid_general, IPC_RMID, NULL);
        semctl(semid_general, 0, IPC_RMID);
        return 1;
    }
    memset(conexion, 0, sizeof(*conexion));
    conexion->pid_servidor = getpid();

    sem_fijar(semid_general, SEM_G_SOLICITUD, 0);
    sem_fijar(semid_general, SEM_G_RESPUESTA, 0);
    sem_fijar(semid_general, SEM_G_MUTEX, 1);

    log_evento("Servidor listo (PID %d). Esperando clientes... (Ctrl+C para salir)",
               (int)getpid());

    while (activo) {
        int r = sem_down_timeout(semid_general, SEM_G_SOLICITUD, 1);

        if (r < 0) {
            break;
        }
        if (r == 0) {
            atender_nueva_conexion();
        }
    }

    log_evento("Cerrando servidor...");

    /* Damos tiempo a que los hilos cierren a sus clientes */
    for (espera = 0; espera < 50; espera++) {
        pthread_mutex_lock(&mutex_clientes);
        int restantes = clientes_activos;
        pthread_mutex_unlock(&mutex_clientes);
        if (restantes == 0) {
            break;
        }
        usleep(100000);
    }

    shmdt(conexion);
    shmctl(shmid_general, IPC_RMID, NULL);
    semctl(semid_general, 0, IPC_RMID);
    log_evento("Memoria compartida y semaforos eliminados. Adios.");
    return 0;
}
