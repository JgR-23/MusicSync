# cambios a realizar
- me deja agregar canciones a playlist sin antes comprarla
- en bibloteca que quite la opcion de agregar una playlist , solo el de agregar al carrito 
- mostrar el dinero que tiene el cliente
- al comprar canciones no las agrega a alguna playlist
                - agregar un modulo de "compras" donde aparescan todas las compras hechas y desde se agregen a alguna playlist   
### QUITAR WARNINGS
                from administrador/admin_modulos.c:1:
In function ‘snprintf’,
    inlined from ‘elegir_usuario’ at administrador/admin_modulos.c:101:9:
/nix/store/2f69qaci676cq5kr8svry3gbf6i6iqzi-glibc-2.44-57-dev/include/bits/stdio2.h:68:10: note: ‘__builtin___snprintf_chk’ output between 22 and 8561 bytes into a destination of size 100
   68 |   return __builtin___snprintf_chk (__s, __n, __USE_FORTIFY_LEVEL - 1,
      |          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   69 |                                    __glibc_objsize (__s), __fmt,
      |                                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   70 |                                    __va_arg_pack ());
      |                                    ~~~~~~~~~~~~~~~~~
administrador/admin_modulos.c: In function ‘ver_usuarios’:
administrador/admin_modulos.c:126:39: warning: ‘%-22s’ directive output may be truncated writing between 22 and 8499 bytes into a region of size 160 [-Wformat-truncation=]
  126 |         snprintf(fila, sizeof(fila), "%-22s %s\n", lista[i].usuario, lista[i].correo);
      |                                       ^~~~~
In function ‘snprintf’,
    inlined from ‘ver_usuarios’ at administrador/admin_modulos.c:126:9:
/nix/store/2f69qaci676cq5kr8svry3gbf6i6iqzi-glibc-2.44-57-dev/include/bits/stdio2.h:68:10: note: ‘__builtin___snprintf_chk’ output 25 or more bytes (assuming 8502) into a destination of size 160
   68 |   return __builtin___snprintf_chk (__s, __n, __USE_FORTIFY_LEVEL - 1,
      |          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   69 |                                    __glibc_objsize (__s), __fmt,
      |                                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   70 |                                    __va_arg_pack ());
      |                                    ~~~~~~~~~~~~~~~~~
gcc -Wall -Wextra -Wpedantic -std=c11 -D_GNU_SOURCE -Icommon -Iservidor servidor/Servidor.c servidor/usuarios.c servidor/biblioteca.c servidor/playlists_srv.c servidor/carrito_srv.c servidor/admin_srv.c servidor/reportes.c servidor/respuesta.c servidor/log.c common/ipc_util.c common/cifrado.c -o Servidor -lpthread
servidor/usuarios.c: In function ‘srv_adm_ver_usuarios’:
servidor/usuarios.c:369:39: warning: ‘%s’ directive output may be truncated writing up to 35099 bytes into a region of size 128 [-Wformat-truncation=]
  369 |         snprintf(fila, sizeof(fila), "%s;%s\n", lista[i].usuario, lista[i].correo);
      |                                       ^~
In file included from /nix/store/2f69qaci676cq5kr8svry3gbf6i6iqzi-glibc-2.44-57-dev/include/stdio.h:974,
                 from servidor/usuarios.c:2:
In function ‘snprintf’,
    inlined from ‘srv_adm_ver_usuarios’ at servidor/usuarios.c:369:9:
/nix/store/2f69qaci676cq5kr8svry3gbf6i6iqzi-glibc-2.44-57-dev/include/bits/stdio2.h:68:10: note: ‘__builtin___snprintf_chk’ output 3 or more bytes (assuming 35102) into a destination of size 128
   68 |   return __builtin___snprintf_chk (__s, __n, __USE_FORTIFY_LEVEL - 1,
      |          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   69 |                                    __glibc_objsize (__s), __fmt,
      |                                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   70 |                                    __va_arg_pack ());
      |                                    ~~~~~~~~~~~~~~~~~
servidor/playlists_srv.c: In function ‘srv_ver_playlists’:
servidor/playlists_srv.c:139:39: warning: ‘%s’ directive output may be truncated writing up to 2719 bytes into a region of size 128 [-Wformat-truncation=]
  139 |         snprintf(fila, sizeof(fila), "%s;%d\n", pl[i].nombre, pl[i].n);
      |                                       ^~
In file included from /nix/store/2f69qaci676cq5kr8svry3gbf6i6iqzi-glibc-2.44-57-dev/include/stdio.h:974,
                 from servidor/playlists_srv.c:1:
In function ‘snprintf’,
    inlined from ‘srv_ver_playlists’ at servidor/playlists_srv.c:139:9:
/nix/store/2f69qaci676cq5kr8svry3gbf6i6iqzi-glibc-2.44-57-dev/include/bits/stdio2.h:68:10: note: ‘__builtin___snprintf_chk’ output between 4 and 2733 bytes into a destination of size 128
   68 |   return __builtin___snprintf_chk (__s, __n, __USE_FORTIFY_LEVEL - 1,
      |          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   69 |                                    __glibc_objsize (__s), __fmt,
      |                                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   70 |                                    __va_arg_pack ());
      |                                    ~~~~~~~~~~~~~~~~~
