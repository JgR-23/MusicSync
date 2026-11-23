#include "ui_auth.h"
#include "ipc_cliente.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

void colores(void)
{
    if(has_colors())
    {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLACK);    /* Fondo general */
        init_pair(2, COLOR_BLACK, COLOR_CYAN);     /* Campo / boton activo */
        init_pair(3, COLOR_WHITE, COLOR_RED);      /* Mensaje de error */
        init_pair(4, COLOR_BLACK, COLOR_GREEN);    /* Mensaje de exito */
        init_pair(5, COLOR_BLUE, COLOR_BLACK);     /* Encabezados y bordes */
        init_pair(6, COLOR_WHITE, COLOR_BLACK);    /* Caja de texto inactiva */
    }
}

void iniciarncurses(void)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    colores();
    bkgd(COLOR_PAIR(1));
}

void cerrarncurses(void)
{
    endwin();
}

void imprimirTextoCentrado(int fila, const char *texto)
{
    int filas, columnas, x;
    getmaxyx(stdscr, filas, columnas);
    (void)filas;
    x = (columnas - (int)strlen(texto)) / 2;
    if(x < 0)
    {
        x = 0;
    }
    mvprintw(fila, x, "%s", texto);
}

void dibujaAyuda(int fila)
{
    attron(COLOR_PAIR(5));
    imprimirTextoCentrado(fila, "Flechas: Mover | ENTER: Seleccionar | ESC: Salir");
    attroff(COLOR_PAIR(5));
}

void dibujarMarcoPrincipal(const char *titulo, const char *subtitulo)
{
    int filas, columnas;
    int anchoMarco = 68;
    int altoMarco = 20;
    int inicioY, inicioX, i;

    getmaxyx(stdscr, filas, columnas);
    inicioY = (filas - altoMarco) / 2;
    inicioX = (columnas - anchoMarco) / 2;

    if(inicioY < 1)
    {
        inicioY = 1;
    }
    if(inicioX < 1)
    {
        inicioX = 1;
    }

    attron(COLOR_PAIR(5));
    mvaddch(inicioY, inicioX, ACS_ULCORNER);
    mvaddch(inicioY, inicioX + anchoMarco - 1, ACS_URCORNER);
    mvaddch(inicioY + altoMarco - 1, inicioX, ACS_LLCORNER);
    mvaddch(inicioY + altoMarco - 1, inicioX + anchoMarco - 1, ACS_LRCORNER);

    for(i = 1; i < anchoMarco - 1; i++)
    {
        mvaddch(inicioY, inicioX + i, ACS_HLINE);
        mvaddch(inicioY + 3, inicioX + i, ACS_HLINE);
        mvaddch(inicioY + altoMarco - 1, inicioX + i, ACS_HLINE);
    }

    for(i = 1; i < altoMarco - 1; i++)
    {
        mvaddch(inicioY + i, inicioX, ACS_VLINE);
        mvaddch(inicioY + i, inicioX + anchoMarco - 1, ACS_VLINE);
    }

    mvaddch(inicioY + 3, inicioX, ACS_LTEE);
    mvaddch(inicioY + 3, inicioX + anchoMarco - 1, ACS_RTEE);

    attron(A_BOLD);
    imprimirTextoCentrado(inicioY + 1, titulo);
    attroff(A_BOLD);
    imprimirTextoCentrado(inicioY + 2, subtitulo);
    attroff(COLOR_PAIR(5));
}

void dibujarCampo(int fila, int columna, const char *etiqueta, const char *valor, int activo, int ocultar)
{
    int i, len;
    int colCaja = columna + 23;
    int anchoCaja = 26;

    if(activo)
    {
        attron(COLOR_PAIR(5) | A_BOLD);
        mvprintw(fila, columna - 2, "> %s", etiqueta);
        attroff(COLOR_PAIR(5) | A_BOLD);
        attron(COLOR_PAIR(2) | A_BOLD);
    }
    else
    {
        attron(COLOR_PAIR(1));
        mvprintw(fila, columna, "%s", etiqueta);
        attroff(COLOR_PAIR(1));
        attron(COLOR_PAIR(6));
    }

    for(i = 0; i < anchoCaja; i++)
    {
        mvprintw(fila, colCaja + i, " ");
    }

    len = (int)strlen(valor);
    if(len > anchoCaja - 2)
    {
        len = anchoCaja - 2;
    }

    if(ocultar)
    {
        for(i = 0; i < len; i++)
        {
            mvprintw(fila, colCaja + 1 + i, "*");
        }
    }
    else
    {
        for(i = 0; i < len; i++)
        {
            mvprintw(fila, colCaja + 1 + i, "%c", valor[i]);
        }
    }

    if(activo)
    {
        attroff(COLOR_PAIR(2) | A_BOLD);
    }
    else
    {
        attroff(COLOR_PAIR(6));
    }
}

void dibujarBoton(int fila, const char *texto, int activo)
{
    int filas, columnas, x;
    char etiquetaBoton[64];

    getmaxyx(stdscr, filas, columnas);
    (void)filas;

    snprintf(etiquetaBoton, sizeof(etiquetaBoton), "  [ %s ]  ", texto);
    x = (columnas - (int)strlen(etiquetaBoton)) / 2;
    if(x < 0)
    {
        x = 0;
    }

    if(activo)
    {
        attron(COLOR_PAIR(2) | A_BOLD);
    }
    else
    {
        attron(COLOR_PAIR(6));
    }

    mvprintw(fila, x, "%s", etiquetaBoton);

    if(activo)
    {
        attroff(COLOR_PAIR(2) | A_BOLD);
    }
    else
    {
        attroff(COLOR_PAIR(6));
    }
}

void dibujarBotonesLogin(int fila, int activoIniciar, int activoRegistrar)
{
    int filas, columnas, xIzq, xDer;
    char botonIzq[40];
    char botonDer[40];

    getmaxyx(stdscr, filas, columnas);
    (void)filas;

    snprintf(botonIzq, sizeof(botonIzq), "  [ Iniciar sesion ]  ");
    snprintf(botonDer, sizeof(botonDer), "  [ Registrarse ]  ");

    xIzq = (columnas / 2) - (int)strlen(botonIzq) - 3;
    xDer = (columnas / 2) + 3;

    if(xIzq < 1)
    {
        xIzq = 1;
    }
    if(xDer < 1)
    {
        xDer = 1;
    }

    if(activoIniciar)
    {
        attron(COLOR_PAIR(2) | A_BOLD);
    }
    else
    {
        attron(COLOR_PAIR(6));
    }
    mvprintw(fila, xIzq, "%s", botonIzq);
    if(activoIniciar)
    {
        attroff(COLOR_PAIR(2) | A_BOLD);
    }
    else
    {
        attroff(COLOR_PAIR(6));
    }

    if(activoRegistrar)
    {
        attron(COLOR_PAIR(2) | A_BOLD);
    }
    else
    {
        attron(COLOR_PAIR(6));
    }
    mvprintw(fila, xDer, "%s", botonDer);
    if(activoRegistrar)
    {
        attroff(COLOR_PAIR(2) | A_BOLD);
    }
    else
    {
        attroff(COLOR_PAIR(6));
    }
}

void dibujarMensaje(int fila, const char *mensaje, int esError)
{
    char barra[64];
    if(mensaje[0] != '\0')
    {
        snprintf(barra, sizeof(barra), " %s ", mensaje);
        if(esError)
        {
            attron(COLOR_PAIR(3) | A_BOLD);
        }
        else
        {
            attron(COLOR_PAIR(4) | A_BOLD);
        }
        imprimirTextoCentrado(fila, barra);
        if(esError)
        {
            attroff(COLOR_PAIR(3) | A_BOLD);
        }
        else
        {
            attroff(COLOR_PAIR(4) | A_BOLD);
        }
    }
}

void inicializarLogin(EstadoLogin *estado)
{
    estado->usuario[0] = '\0';
    estado->lenUsuario = 0;
    estado->contrasena[0] = '\0';
    estado->lenContrasena = 0;
    estado->campoActivo = 0;
    estado->mensaje[0] = '\0';
    estado->esError = 0;
    estado->autenticado = 0;
}

void inicializarRegistro(EstadoRegistro *estado)
{
    estado->usuario[0] = '\0';
    estado->lenUsuario = 0;
    estado->correo[0] = '\0';
    estado->lenCorreo = 0;
    estado->contrasena[0] = '\0';
    estado->lenContrasena = 0;
    estado->confirmar[0] = '\0';
    estado->lenConfirmar = 0;
    estado->campoActivo = 0;
    estado->mensaje[0] = '\0';
    estado->esError = 0;
    estado->registrado = 0;
}

void dibujarLogin(const EstadoLogin *estado)
{
    int filas, columnas, inicioY, inicioX;
    getmaxyx(stdscr, filas, columnas);
    inicioY = (filas - 20) / 2;
    inicioX = (columnas - 68) / 2;
    if(inicioY < 1)
    {
        inicioY = 1;
    }
    if(inicioX < 1)
    {
        inicioX = 1;
    }

    erase();
    dibujarMarcoPrincipal("INICIO DE SESION", "Ingrese sus credenciales");

    dibujarCampo(inicioY + 6, inicioX + 8, "Nombre de usuario:", estado->usuario, estado->campoActivo == 0, 0);
    dibujarCampo(inicioY + 8, inicioX + 8, "Contrasena:", estado->contrasena, estado->campoActivo == 1, 1);
    dibujarBotonesLogin(inicioY + 12, estado->campoActivo == 2, estado->campoActivo == 3);
    dibujarMensaje(inicioY + 15, estado->mensaje, estado->esError);
    dibujaAyuda(filas - 2);
    refresh();
}

void dibujarRegistro(const EstadoRegistro *estado)
{
    int filas, columnas, inicioY, inicioX;
    getmaxyx(stdscr, filas, columnas);
    inicioY = (filas - 20) / 2;
    inicioX = (columnas - 68) / 2;
    if(inicioY < 1)
    {
        inicioY = 1;
    }
    if(inicioX < 1)
    {
        inicioX = 1;
    }

    erase();
    dibujarMarcoPrincipal("REGISTRO DE USUARIO", "Complete los datos de su cuenta");

    dibujarCampo(inicioY + 5, inicioX + 8, "Nombre de usuario:", estado->usuario, estado->campoActivo == 0, 0);
    dibujarCampo(inicioY + 7, inicioX + 8, "Correo electronico:", estado->correo, estado->campoActivo == 1, 0);
    dibujarCampo(inicioY + 9, inicioX + 8, "Contrasena:", estado->contrasena, estado->campoActivo == 2, 1);
    dibujarCampo(inicioY + 11, inicioX + 8, "Confirmar contrasena:", estado->confirmar, estado->campoActivo == 3, 1);
    dibujarBoton(inicioY + 14, "Registrar", estado->campoActivo == 4);
    dibujarMensaje(inicioY + 17, estado->mensaje, estado->esError);
    dibujaAyuda(filas - 2);
    refresh();
}

void moverFocoLogin(EstadoLogin *estado, int delta)
{
    estado->campoActivo += delta;
    if(estado->campoActivo < 0)
    {
        estado->campoActivo = 3;
    }
    else if(estado->campoActivo > 3)
    {
        estado->campoActivo = 0;
    }
}

void moverFocoRegistro(EstadoRegistro *estado, int delta)
{
    estado->campoActivo += delta;
    if(estado->campoActivo < 0)
    {
        estado->campoActivo = 4;
    }
    else if(estado->campoActivo > 4)
    {
        estado->campoActivo = 0;
    }
}

void agregarCaracter(char *buffer, int *len, int tecla)
{
    if(tecla >= 32 && tecla <= 126 && *len < MAX_CAMPO)
    {
        buffer[*len] = (char)tecla;
        (*len)++;
        buffer[*len] = '\0';
    }
}

void borrarCaracter(char *buffer, int *len)
{
    if(*len > 0)
    {
        (*len)--;
        buffer[*len] = '\0';
    }
}

int validarCorreo(const char *correo)
{
    const char *arroba = strchr(correo, '@');
    const char *punto;
    int len = (int)strlen(correo);

    if(len < 5 || arroba == NULL || arroba == correo)
    {
        return 0;
    }

    if(strchr(correo, ' ') != NULL)
    {
        return 0;
    }

    if(strchr(arroba + 1, '@') != NULL)
    {
        return 0;
    }

    punto = strchr(arroba + 1, '.');
    if(punto == NULL || punto == arroba + 1)
    {
        return 0;
    }

    if(*(punto + 1) == '\0' || correo[len - 1] == '.')
    {
        return 0;
    }

    return 1;
}

int validarContrasenaSegura(const char *pass)
{
    int tieneMayus = 0;
    int tieneMinus = 0;
    int tieneSimbolo = 0;
    int len = (int)strlen(pass);
    int i;

    if(len < 8)
    {
        return 0;
    }

    for(i = 0; i < len; i++)
    {
        unsigned char c = (unsigned char)pass[i];
        if(isupper(c))
        {
            tieneMayus = 1;
        }
        else if(islower(c))
        {
            tieneMinus = 1;
        }
        else if(!isdigit(c) && !isspace(c))
        {
            tieneSimbolo = 1;
        }
    }

    return (tieneMayus && tieneMinus && tieneSimbolo);
}

void validarYProcesarLogin(EstadoLogin *estado)
{
    estado->autenticado = 0;

    if(estado->lenUsuario == 0 || estado->lenContrasena == 0)
    {
        estado->esError = 1;
        snprintf(estado->mensaje, sizeof(estado->mensaje), "Error: Completa usuario y contrasena");
        return;
    }

    /* Se envia al servidor por memoria compartida */
    if(ipc_login(estado->usuario, estado->contrasena) != 0)
    {
        estado->esError = 1;
        snprintf(estado->mensaje, sizeof(estado->mensaje), "Error: %.50s", ipc_ultimo_error());
        return;
    }

    estado->esError = 0;
    estado->autenticado = 1;
    snprintf(estado->mensaje, sizeof(estado->mensaje), "Bienvenido, %.40s", estado->usuario);
}

void validarYProcesarRegistro(EstadoRegistro *estado)
{
    estado->registrado = 0;

    if(estado->lenUsuario < 3)
    {
        estado->esError = 1;
        snprintf(estado->mensaje, sizeof(estado->mensaje), "Error: El usuario debe tener al menos 3 caracteres");
        return;
    }

    if(!validarCorreo(estado->correo))
    {
        estado->esError = 1;
        snprintf(estado->mensaje, sizeof(estado->mensaje), "Error: Correo invalido (debe llevar @ y .algo)");
        return;
    }

    if(!validarContrasenaSegura(estado->contrasena))
    {
        estado->esError = 1;
        snprintf(estado->mensaje, sizeof(estado->mensaje), "Error: Contrasena min 8 chars, 1 Mayus, 1 minus y 1 simbolo");
        return;
    }

    if(strcmp(estado->contrasena, estado->confirmar) != 0)
    {
        estado->esError = 1;
        snprintf(estado->mensaje, sizeof(estado->mensaje), "Error: Las contrasenas no coinciden");
        return;
    }

    /* Se envia al servidor por memoria compartida */
    if(ipc_registrar(estado->usuario, estado->correo, estado->contrasena) != 0)
    {
        estado->esError = 1;
        snprintf(estado->mensaje, sizeof(estado->mensaje), "Error: %.50s", ipc_ultimo_error());
        return;
    }

    estado->esError = 0;
    estado->registrado = 1;
    snprintf(estado->mensaje, sizeof(estado->mensaje), "Registro exitoso");
}

/* Regresa 1 si el usuario eligio ir a Registrarse, 2 si inicio sesion correctamente, 0 en otro caso */
int teclaLogin(EstadoLogin *estado, int tecla)
{
    if(tecla == KEY_UP || tecla == KEY_LEFT)
    {
        moverFocoLogin(estado, -1);
    }
    else if(tecla == KEY_DOWN || tecla == KEY_RIGHT)
    {
        moverFocoLogin(estado, 1);
    }
    else if(tecla == KEY_BACKSPACE || tecla == 127 || tecla == 8)
    {
        if(estado->campoActivo == 0)
        {
            borrarCaracter(estado->usuario, &estado->lenUsuario);
        }
        else if(estado->campoActivo == 1)
        {
            borrarCaracter(estado->contrasena, &estado->lenContrasena);
        }
    }
    else if(tecla == 10 || tecla == 13 || tecla == KEY_ENTER)
    {
        if(estado->campoActivo == 0 || estado->campoActivo == 1)
        {
            moverFocoLogin(estado, 1);
        }
        else if(estado->campoActivo == 2)
        {
            validarYProcesarLogin(estado);

            if(estado->autenticado)
            {
                return 2;
            }
        }
        else if(estado->campoActivo == 3)
        {
            return 1;
        }
    }
    else
    {
        if(estado->campoActivo == 0)
        {
            agregarCaracter(estado->usuario, &estado->lenUsuario, tecla);
        }
        else if(estado->campoActivo == 1)
        {
            agregarCaracter(estado->contrasena, &estado->lenContrasena, tecla);
        }
    }

    return 0;
}

void teclaRegistro(EstadoRegistro *estado, int tecla)
{
    if(tecla == KEY_UP)
    {
        moverFocoRegistro(estado, -1);
    }
    else if(tecla == KEY_DOWN)
    {
        moverFocoRegistro(estado, 1);
    }
    else if(tecla == KEY_BACKSPACE || tecla == 127 || tecla == 8)
    {
        if(estado->campoActivo == 0)
        {
            borrarCaracter(estado->usuario, &estado->lenUsuario);
        }
        else if(estado->campoActivo == 1)
        {
            borrarCaracter(estado->correo, &estado->lenCorreo);
        }
        else if(estado->campoActivo == 2)
        {
            borrarCaracter(estado->contrasena, &estado->lenContrasena);
        }
        else if(estado->campoActivo == 3)
        {
            borrarCaracter(estado->confirmar, &estado->lenConfirmar);
        }
    }
    else if(tecla == 10 || tecla == 13 || tecla == KEY_ENTER)
    {
        if(estado->campoActivo >= 0 && estado->campoActivo <= 3)
        {
            moverFocoRegistro(estado, 1);
        }
        else if(estado->campoActivo == 4)
        {
            validarYProcesarRegistro(estado);
        }
    }
    else
    {
        if(estado->campoActivo == 0)
        {
            agregarCaracter(estado->usuario, &estado->lenUsuario, tecla);
        }
        else if(estado->campoActivo == 1)
        {
            agregarCaracter(estado->correo, &estado->lenCorreo, tecla);
        }
        else if(estado->campoActivo == 2)
        {
            agregarCaracter(estado->contrasena, &estado->lenContrasena, tecla);
        }
        else if(estado->campoActivo == 3)
        {
            agregarCaracter(estado->confirmar, &estado->lenConfirmar, tecla);
        }
    }
}
