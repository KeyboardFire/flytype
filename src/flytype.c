#include <ncurses.h>
#include <locale.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <stdlib.h>
#include <time.h>

#include "utf8.h"

#define FAIL 1

#define FILE_ERR(f, e) do { \
        fprintf(stderr, "error reading %s\n", (f)); \
        perror(e); \
        return ""; \
    } while (0)

#define ALPHABET "абвгдежзийклмнопрстуфхцчшщъыьэюяё                           "

#define DATA(i, j) (data + dataIdx + (i) + (alphabetSize*(j)))

unsigned int getInt(unsigned char *ptr) {
    unsigned long long val = 0;
    for (int i = 0; i < 4; ++i) {
        val |= ptr[i] << (i*8);
    }
    return val;
}

unsigned long long getLong(unsigned char *ptr) {
    unsigned long long val = 0;
    for (int i = 0; i < 8; ++i) {
        val |= ptr[i] << (i*8);
    }
    return val;
}

unsigned long long longRand(unsigned long long max) {
    // RAND_MAX is guaranteed to be at least 15 bits, so let's be safe
    // ... that comment originally made more sense than it does now
    int reps = sizeof(unsigned long long) * 8;
    unsigned long long val = 0;
    do {
        val = (val << 1) | (rand() & 1);
    } while (--reps);
    return ((double)val / (unsigned long long)-1) * max;
}

void typeStr(WINDOW *win, char *str) {
    mvwprintw(win, 1, 1, str);
    wrefresh(win);

    int pos = 0;

    uchar input[4];
    while (1) {
        input[0] = getch();
        int inputBytes = byteLength(input[0]);
        int i = 1;
        for (uchar *p = input + 1; i < inputBytes; ++i) {
            *p = getch();
        }

        int fail = 0;
        if (byteLength(*str) == inputBytes) {
            for (i = 0; i < inputBytes; ++i) {
                if ((uchar)str[i] != input[i]) {
                    fail = 1;
                    break;
                }
            }
        } else fail = 1;

        if (fail) wattron(win, COLOR_PAIR(FAIL));
        mvwprintw(win, 2, pos+1, "%.*s", inputBytes, input);
        wrefresh(win);
        wattroff(win, COLOR_PAIR(FAIL));
        if (!fail) {
            ++pos;
            str += inputBytes;
        }

        if (input[0] == '\x03') break;
    }
}

char *genStr(char *filename, int len) {
    int fh;
    unsigned char *data;

    if ((fh = open(filename, O_RDONLY)) == -1) FILE_ERR(filename, "open");

    struct stat finfo;
    fstat(fh, &finfo);

    if ((data = mmap(0, finfo.st_size, PROT_READ, MAP_SHARED, fh, 0))
            == MAP_FAILED) FILE_ERR(filename, "mmap");
    if (close(fh) == -1) FILE_ERR(filename, "close");

    int dataIdx = 0, alphabetSize = 1;
    for (; data[dataIdx]; dataIdx += byteLength(data[dataIdx]), ++alphabetSize);
    ++dataIdx;  // we're on the null byte, want to be where data actually starts
    if (alphabetSize*alphabetSize*(8+4*alphabetSize) != finfo.st_size - dataIdx) {
        return "!!! CORRUPT DATA FILE !!!";
    }

    char *str = malloc((len+1) * 4*sizeof *str);
    int a = alphabetSize - 1, b = alphabetSize - 1;
    int strpos = 0;
    for (int i = 0; i < len; ++i) {
        unsigned char *dataPtr = DATA(a, b);
        unsigned long long n = longRand(getLong(dataPtr));
        dataPtr += 8;
        int nextCharPos = 0;
        for (; nextCharPos < alphabetSize; ++nextCharPos) {
            unsigned int m = getInt(dataPtr);
            if (m > n) break;
            n -= m;
            dataPtr += 4;
        }
        a = b;
        b = nextCharPos;
        int alphabetIdx = 0;
        for (; nextCharPos; --nextCharPos) {
            alphabetIdx += byteLength(ALPHABET[alphabetIdx]);
        }
        int nextCharLen = byteLength(ALPHABET[alphabetIdx]);
        for (int i = nextCharLen; i >= 0; --i) {
            str[strpos + i] = ALPHABET[alphabetIdx + i];
        }
        strpos += nextCharLen;
    }
    str[strpos] = '\0';
    //for (int i = 0; i < len*2; ++i) printf("str[%d]: %c\n", i, str[i]);

    if (munmap(data, finfo.st_size) == -1) FILE_ERR(filename, "munmap");

    return str;
}

int main(int argc, char* argv[]) {
    srand(time(NULL));

    setlocale(LC_ALL, "en_US.utf8");
    initscr();
    raw();
    noecho();
    start_color();
    init_pair(FAIL, COLOR_RED, COLOR_BLACK);

    WINDOW *typeWin = newwin(4, COLS, 0, 0);
    box(typeWin, 0, 0);
    refresh();
    typeStr(typeWin, genStr("data-russian", 50));

    endwin();
    return 0;
}
