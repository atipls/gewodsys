#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <cpu/intel.h>
#include "serial.h"

void ComPutChar(char c) {
    while (!(IoIn(0x3F8 + 5) & 0x20))
        ;
    IoOut(0x3F8, c);
}

void ComPrint(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char *string = (char *) fmt;
    while (*string) {
        switch (*string) {
            case '%':
                string++;
                switch (*string) {
                    case 's':
                        string++;
                        for (char *s = va_arg(args, char *); *s; s++)
                            ComPutChar(*s);
                        break;
                    case 'c':
                        string++;
                        ComPutChar(va_arg(args, int));
                        break;
                    case 'd': {
                        string++;
                        int num = va_arg(args, int);
                        if (num < 0) {
                            ComPutChar('-');
                            num = -num;
                        }

                        int div = 1;
                        while (num / div >= 10)
                            div *= 10;

                        while (div) {
                            ComPutChar('0' + num / div);
                            num %= div;
                            div /= 10;
                        }
                    } break;
                    case 'x': {
                        string++;
                        int num = va_arg(args, int);
                        for (int i = 28; i >= 0; i -= 4)
                            ComPutChar("0123456789ABCDEF"[(num >> i) & 0xF]);
                    } break;
                    case 'X': {
                        string++;
                        uint64_t num = va_arg(args, uint64_t);
                        for (int i = 60; i >= 0; i -= 4)
                            ComPutChar("0123456789ABCDEF"[(num >> i) & 0xF]);
                    } break;
                    case '%':
                        ComPutChar('%');
                        break;
                }
                break;
            default:
                ComPutChar(*string++);
                break;
        }
    }
}
