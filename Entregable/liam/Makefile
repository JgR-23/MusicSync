CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -std=c11
LIBS = -lncurses

EJECUTABLE = cliente_app

ARCHIVOS = cliente/main_cliente.c \
           cliente/interfaz/interfaz.c \
           cliente/playlists/playlists.c

all:
	$(CC) $(CFLAGS) $(ARCHIVOS) -Icliente/interfaz -Icliente/playlists -o $(EJECUTABLE) $(LIBS)

run: all
	./$(EJECUTABLE)

clean:
	rm -f $(EJECUTABLE)

.PHONY: all run clean