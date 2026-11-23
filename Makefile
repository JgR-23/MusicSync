# ----------------------------------------------------------------
# MusicSync - prototipo inicial: Servidor, Cliente y Administrador
# (comunicacion con semaforos + memoria compartida, sin sockets)
# ----------------------------------------------------------------
CC     = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -D_GNU_SOURCE -Icommon

COMUN = common/ipc_util.c common/cifrado.c

SRV_SRC = servidor/Servidor.c servidor/usuarios.c servidor/biblioteca.c \
          servidor/playlists_srv.c servidor/carrito_srv.c \
          servidor/admin_srv.c servidor/reportes.c \
          servidor/respuesta.c servidor/log.c \
          $(COMUN)
SRV_INC = -Iservidor

# Piezas de interfaz que usan el Cliente y el Administrador
UI_COMUN = cliente/comunicacion.c cliente/menus.c \
           cliente/interfaz/interfaz.c cliente/interfaz/ui_form.c \
           cliente/interfaz/ui_texto.c cliente/auth/sesion_ui.c

CLI_SRC = cliente/Cliente.c $(UI_COMUN) \
          cliente/biblioteca/ui_comun.c cliente/biblioteca/ui_buscar.c \
          cliente/biblioteca/ui_biblioteca.c cliente/playlists/playlists.c \
          cliente/carrito/ui_carrito.c cliente/perfil/ui_perfil.c \
          $(COMUN)

ADM_SRC = administrador/Administrador.c administrador/admin_modulos.c \
          $(UI_COMUN) $(COMUN)

UI_INC  = -Icliente -Icliente/interfaz -Icliente/auth
CLI_INC = $(UI_INC) -Icliente/biblioteca -Icliente/playlists \
          -Icliente/carrito -Icliente/perfil
ADM_INC = $(UI_INC) -Iadministrador

HEADERS = $(wildcard common/*.h servidor/*.h cliente/*.h cliente/*/*.h administrador/*.h)

all: archivo Cliente Administrador Servidor

# "archivo" es solo para que ftok() genere las llaves de IPC
archivo:
	touch archivo

Cliente: $(CLI_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(CLI_INC) $(CLI_SRC) -o Cliente -lncurses

Administrador: $(ADM_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(ADM_INC) $(ADM_SRC) -o Administrador -lncurses

Servidor: $(SRV_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(SRV_INC) $(SRV_SRC) -o Servidor -lpthread

clean:
	rm -f Cliente Administrador Servidor

.PHONY: all clean
