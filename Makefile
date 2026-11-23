##
#  MusicSync - reproductor de musica (C + ncurses)
#  Cliente y servidor se comunican con memoria compartida y semaforos POSIX.
#
#  make            compila ./servidor y ./cliente
#  ./servidor      (en una terminal)
#  ./cliente       (en otra terminal)
#  make clean      borra objetos y ejecutables
#  make limpiar-ipc  borra restos de memoria compartida/semaforos si el servidor murio mal
#
# @file
# @version 0.1

CC      = gcc
CFLAGS  = -std=gnu11 -Wall -Wextra -g -MMD -MP -I. -Ivistas

LDLIBS_CLIENTE  = -lncurses -lrt -pthread
LDLIBS_SERVIDOR = -lrt -pthread

OBJ_VISTAS = vistas/interfaz.o \
             vistas/ui_auth.o \
             vistas/ui_comun.o \
             vistas/ui_buscar.o \
             vistas/ui_biblioteca.o \
             vistas/vista_menu_inicio.o \
             vistas/playlists.o

OBJ_CLIENTE  = cliente.o ipc_cliente.o colores.o $(OBJ_VISTAS)
OBJ_SERVIDOR = servidor.o almacen.o

all: servidor cliente

servidor: $(OBJ_SERVIDOR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS_SERVIDOR)

cliente: $(OBJ_CLIENTE)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS_CLIENTE)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.d vistas/*.o vistas/*.d servidor cliente

limpiar-ipc:
	rm -f /dev/shm/musicsync_shm /dev/shm/sem.musicsync_mutex /dev/shm/sem.musicsync_solicitud /dev/shm/sem.musicsync_respuesta

.PHONY: all clean limpiar-ipc

-include $(OBJ_CLIENTE:.o=.d) $(OBJ_SERVIDOR:.o=.d)

# end
