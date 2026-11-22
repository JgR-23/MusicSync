#include "ui_comun.h"

#include <ctype.h>
#include <ncurses.h>
#include <string.h>

const char *nombre_criterio(CriterioBusqueda c) {
    switch (c) {
        case CRITERIO_NOMBRE:  return "Nombre";
        case CRITERIO_ARTISTA: return "Artista";
        case CRITERIO_ALBUM:   return "Album";
        case CRITERIO_GENERO:  return "Genero";
        default:               return "Nombre";
    }
}

bool validar_texto(const char *raw, char *out, size_t out_size) {
    if (!raw || !out || out_size < 2) return false;

    size_t len = strlen(raw);
    while (len > 0 && isspace((unsigned char)raw[len - 1])) len--;

    size_t ini = 0;
    while (ini < len && isspace((unsigned char)raw[ini])) ini++;

    size_t util = len - ini;
    if (util == 0 || util >= out_size) return false;

    memcpy(out, raw + ini, util);
    out[util] = '\0';
    return true;
}

bool contiene_texto(const char *haystack, const char *needle) {
    size_t lh = strlen(haystack);
    size_t ln = strlen(needle);
    if (ln == 0 || ln > lh) return false;

    for (size_t i = 0; i <= lh - ln; i++) {
        bool ok = true;
        for (size_t j = 0; j < ln; j++) {
            if (tolower((unsigned char)haystack[i + j]) !=
                tolower((unsigned char)needle[j])) {
                ok = false;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

void mostrar_mensaje_ui(const char *msg) {
    int alto, ancho;
    getmaxyx(stdscr, alto, ancho);
    WINDOW *w = newwin(7, ancho - 10 > 30 ? ancho - 10 : 30, (alto - 7) / 2, 5);
    box(w, 0, 0);
    mvwprintw(w, 2, 2, "%s", msg);
    mvwprintw(w, 4, 2, "Presione una tecla para continuar...");
    wrefresh(w);
    getch();
    delwin(w);
}
