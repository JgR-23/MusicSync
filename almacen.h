#ifndef ALMACEN_H
#define ALMACEN_H

#include <stddef.h>

#include "comun.h"

/*
 * Acceso a los archivos de texto de la carpeta datos/ (solo lo usa el servidor).
 *
 *   datos/usuarios.txt   usuario|correo|hash
 *   datos/canciones.txt  id|titulo|artista|album|genero|duracion|precio
 *   datos/playlists.txt  usuario|nombre_playlist|id1,id2,id3
 *
 * Todas devuelven 0 (o una cantidad >= 0) si todo salio bien y -1 si hubo
 * error; en ese caso el texto del error queda en "err".
 */

int almacen_registrar_usuario(const char *usuario, const char *correo,
                              const char *clave, char *err, size_t n);
int almacen_validar_usuario(const char *usuario, const char *clave,
                            char *err, size_t n);

int almacen_listar_catalogo(RegCancion *salida, int max, char *err, size_t n);

int almacen_listar_playlists(const char *usuario, RegPlaylist *salida, int max);
int almacen_crear_playlist(const char *usuario, const char *nombre,
                           char *err, size_t n);
int almacen_eliminar_playlist(const char *usuario, const char *nombre,
                              char *err, size_t n);
int almacen_listar_canciones_playlist(const char *usuario, const char *nombre,
                                      RegCancion *salida, int max,
                                      char *err, size_t n);
int almacen_agregar_cancion(const char *usuario, const char *nombre,
                            int id_cancion, char *err, size_t n);
int almacen_quitar_cancion(const char *usuario, const char *nombre,
                           int id_cancion, char *err, size_t n);

#endif
