#include "utf8.h"

int byteLength(uchar firstByte) {
    if (firstByte < 0xC0) return 1;
    if (firstByte < 0xE0) return 2;
    if (firstByte < 0xF0) return 3;
    return 4;
}
