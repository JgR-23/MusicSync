#include "ui_auth.h"

int main(void)
{
    EstadoLogin login;
    EstadoRegistro registro;
    int pantalla = 0; /* 0: Inicio de sesion, 1: Registro */
    int tecla;

    iniciarncurses();
    inicializarLogin(&login);
    inicializarRegistro(&registro);

    while(1)
    {
        if(pantalla == 0)
        {
            dibujarLogin(&login);
        }
        else
        {
            dibujarRegistro(&registro);
        }

        tecla = getch();
        if(tecla == 27)
        {
            break;
        }

        if(pantalla == 0)
        {
            if(teclaLogin(&login, tecla) == 1)
            {
                inicializarRegistro(&registro);
                pantalla = 1;
            }
        }
        else
        {
            teclaRegistro(&registro, tecla);
        }
    }

    cerrarncurses();
    return 0;
}