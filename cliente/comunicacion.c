#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "comunicacion.h"
#include "interfaz.h"
#include "ipc_util.h"

Mensaje *com_msg = NULL;

static Conexion *conexion = NULL;
static int sem_general = -1;
static int sem_privado = -1;
static int perdida = 0;
static char ultimo_error[LON_MENSAJE] = "";

static int servidor_vivo(void)
{
    pid_t pid;

    if (conexion == NULL) {
        return 0;
    }
    pid = conexion->pid_servidor;
    if (pid <= 0) {
        return 0;
    }
    return kill(pid, 0) == 0 || errno == EPERM;
}

/* Espera un semaforo vigilando que el servidor siga vivo.
 * 0 = ok, -1 = el servidor ya no existe */
static int esperar_semaforo(int semid, int num)
{
    for (;;) {
        int r = sem_down_timeout(semid, num, 1);

        if (r == 0) {
            return 0;
        }
        if (r < 0 || !servidor_vivo()) {
            return -1;
        }
    }
}

int com_conectar(void)
{
    key_t llave = ftok(RUTA_LLAVE, ID_LLAVE_GENERAL);
    int shm_general;
    int estado, shm_priv, sem_priv;

    if (llave == -1) {
        return COM_SIN_SERVIDOR;
    }
    shm_general = shmget(llave, sizeof(Conexion), 0);
    sem_general = semget(llave, NUM_SEM_GENERAL, 0);
    if (shm_general == -1 || sem_general == -1) {
        return COM_SIN_SERVIDOR;
    }
    conexion = (Conexion *)shmat(shm_general, NULL, 0);
    if (conexion == (Conexion *)-1) {
        conexion = NULL;
        return COM_SIN_SERVIDOR;
    }
    if (!servidor_vivo()) {
        shmdt(conexion);
        conexion = NULL;
        return COM_SIN_SERVIDOR;
    }

    /* Saludo: solo un cliente a la vez usa el canal general */
    if (esperar_semaforo(sem_general, SEM_G_MUTEX) < 0) {
        return COM_SIN_SERVIDOR;
    }
    conexion->pid_cliente = getpid();
    sem_up(sem_general, SEM_G_SOLICITUD);

    if (esperar_semaforo(sem_general, SEM_G_RESPUESTA) < 0) {
        return COM_SIN_SERVIDOR;
    }
    estado = conexion->estado;
    shm_priv = conexion->shmid_priv;
    sem_priv = conexion->semid_priv;
    copiar_cadena(ultimo_error, sizeof(ultimo_error), conexion->mensaje);
    sem_up(sem_general, SEM_G_MUTEX);

    if (estado != 0) {
        return COM_RECHAZADO;
    }

    com_msg = (Mensaje *)shmat(shm_priv, NULL, 0);
    if (com_msg == (Mensaje *)-1) {
        com_msg = NULL;
        copiar_cadena(ultimo_error, sizeof(ultimo_error),
                      "No se pudo abrir el canal privado");
        return COM_RECHAZADO;
    }
    sem_privado = sem_priv;
    return COM_OK;
}

void com_preparar(int accion)
{
    memset(com_msg, 0, sizeof(*com_msg));
    com_msg->accion = accion;
}

int com_transaccion(void)
{
    if (perdida || com_msg == NULL) {
        return -1;
    }

    sem_up(sem_privado, SEM_P_PETICION);

    if (esperar_semaforo(sem_privado, SEM_P_RESPUESTA) < 0) {
        perdida = 1;
        mostrar_aviso("Se perdio la conexion con el servidor.", PAR_ERROR);
        return -1;
    }
    return com_msg->codigo;
}

void com_desconectar(void)
{
    if (com_msg != NULL && !perdida) {
        com_preparar(ACC_DESCONECTAR);
        sem_up(sem_privado, SEM_P_PETICION);
    }
    if (com_msg != NULL) {
        shmdt(com_msg);
        com_msg = NULL;
    }
    if (conexion != NULL) {
        shmdt(conexion);
        conexion = NULL;
    }
}

int com_perdida(void)
{
    return perdida;
}

const char *com_error(void)
{
    return ultimo_error;
}

int com_saldo(double *saldo)
{
    int r;

    *saldo = 0.0;
    com_preparar(ACC_VER_SALDO);
    r = com_transaccion();
    if (r < 0) {
        return -1;
    }
    if (r != RESP_OK) {
        return 1;
    }
    *saldo = atof(com_msg->datos);
    return 0;
}