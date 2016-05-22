#include <ncurses.h>

int main(int argc, char* argv[]) {
    initscr();
    printw(argv[0]);
    refresh();
    getch();
    endwin();
    return 0;
}
