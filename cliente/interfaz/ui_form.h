#ifndef UI_FORM_H
#define UI_FORM_H

#include <stddef.h>

/* Un campo de texto del formulario. "valor" es el buffer (puede venir con
 * texto inicial) y "max" la cantidad maxima de caracteres. */
typedef struct {
    const char *etiqueta;
    char       *valor;
    int         max;
    int         oculto;     /* 1 = se muestra con asteriscos */
} Campo;

/* Al presionar [ boton ] se llama al validador; si devuelve 0 debe dejar el
 * motivo en "error" y el formulario se queda abierto. */
typedef int (*Validador)(const Campo *campos, char *error, size_t tam);

/* Dibuja el formulario con sus botones [ boton ] y [ Volver ].
 * Devuelve 1 si se envio, 0 si se volvio atras (ESC o [ Volver ]). */
int ejecutar_formulario(const char *titulo, const char *subtitulo,
                        Campo *campos, int n, const char *boton,
                        Validador validar, int activo_inicial);

#endif
