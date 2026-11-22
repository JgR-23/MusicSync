#ifndef UI_AUTH_H
#define UI_AUTH_H

#include <ncurses.h>

#define MAX_CAMPO 35

typedef struct
{
    char usuario[MAX_CAMPO + 1];
    int lenUsuario;
    char contrasena[MAX_CAMPO + 1];
    int lenContrasena;
    int campoActivo; /* 0: usuario, 1: contrasena, 2: boton iniciar, 3: boton registrarse */
    char mensaje[120];
    int esError;
} EstadoLogin;

typedef struct
{
    char usuario[MAX_CAMPO + 1];
    int lenUsuario;
    char correo[MAX_CAMPO + 1];
    int lenCorreo;
    char contrasena[MAX_CAMPO + 1];
    int lenContrasena;
    char confirmar[MAX_CAMPO + 1];
    int lenConfirmar;
    int campoActivo; /* 0: usuario, 1: correo, 2: contrasena, 3: confirmar, 4: boton */
    char mensaje[120];
    int esError;
} EstadoRegistro;

void colores(void);
void iniciarncurses(void);
void cerrarncurses(void);
void imprimirTextoCentrado(int fila, const char *texto);
void dibujaAyuda(int fila);
void dibujarMarcoPrincipal(const char *titulo, const char *subtitulo);
void dibujarCampo(int fila, int columna, const char *etiqueta, const char *valor, int activo, int ocultar);
void dibujarBoton(int fila, const char *texto, int activo);
void dibujarBotonesLogin(int fila, int activoIniciar, int activoRegistrar);
void dibujarMensaje(int fila, const char *mensaje, int esError);

void inicializarLogin(EstadoLogin *estado);
void inicializarRegistro(EstadoRegistro *estado);
void dibujarLogin(const EstadoLogin *estado);
void dibujarRegistro(const EstadoRegistro *estado);

void moverFocoLogin(EstadoLogin *estado, int delta);
void moverFocoRegistro(EstadoRegistro *estado, int delta);
void agregarCaracter(char *buffer, int *len, int tecla);
void borrarCaracter(char *buffer, int *len);

int validarCorreo(const char *correo);
int validarContrasenaSegura(const char *pass);
void validarYProcesarLogin(EstadoLogin *estado);
void validarYProcesarRegistro(EstadoRegistro *estado);

int teclaLogin(EstadoLogin *estado, int tecla);
void teclaRegistro(EstadoRegistro *estado, int tecla);

#endif