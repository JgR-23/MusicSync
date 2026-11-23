#ifndef USUARIOS_H
#define USUARIOS_H

#include "protocolo.h"
#include "sesion.h"

int  usuario_valido(const char *usuario);
void srv_registro(Mensaje *m);
void srv_login(Mensaje *m, Sesion *s);
void srv_cerrar_sesion(Mensaje *m, Sesion *s);

/* Perfil del usuario */
void srv_ver_perfil(Mensaje *m, const Sesion *s);
void srv_modificar_perfil(Mensaje *m, const Sesion *s);

/* Administrador */
void srv_login_admin(Mensaje *m, Sesion *s);
void srv_adm_ver_usuarios(Mensaje *m, const Sesion *s);
void srv_adm_crear_usuario(Mensaje *m, const Sesion *s);
void srv_adm_modificar_usuario(Mensaje *m, const Sesion *s);
void srv_adm_eliminar_usuario(Mensaje *m, const Sesion *s);

#endif
