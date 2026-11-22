#include "ui_biblioteca.h"

#include <ncurses.h>

int main(void) {
    const CancionUI base[] = {
        {1, "Bohemian Rhapsody", "Queen", "A Night at the Opera", "Rock", 19.99},
        {2, "Yesterday", "The Beatles", "Help!", "Pop", 12.50},
        {3, "No Quarter", "Led Zeppelin", "Houses of the Holy", "Rock", 15.00},
        {4, "Breathe", "Pink Floyd", "Dark Side of the Moon", "Rock", 14.00}
    };

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    pantalla_biblioteca(base, (int)(sizeof(base) / sizeof(base[0])));

    endwin();
    return 0;
}
