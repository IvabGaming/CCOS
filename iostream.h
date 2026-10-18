#ifndef IOSTREAM_H
#define IOSTREAM_H

#include <stdint.h>

// Forward declarations of kernel VGA drivers
void vga_puts(const char* str);
void vga_putc(char c);

namespace std {

struct Endl {};
constexpr Endl endl;

class ostream {
public:
    ostream& operator<<(const char* str) {
        vga_puts(str);
        return *this;
    }

    ostream& operator<<(char c) {
        vga_putc(c);
        return *this;
    }

    ostream& operator<<(int32_t val) {
        if (val == 0) { vga_putc('0'); return *this; }
        if (val < 0) { vga_putc('-'); val = -val; }
        char buf[16];
        int pos = 0;
        while (val > 0) { buf[pos++] = '0' + (val % 10); val /= 10; }
        for (int i = pos - 1; i >= 0; i--) vga_putc(buf[i]);
        return *this;
    }

    ostream& operator<<(uint32_t val) {
        if (val == 0) { vga_putc('0'); return *this; }
        char buf[16];
        int pos = 0;
        while (val > 0) { buf[pos++] = '0' + (val % 10); val /= 10; }
        for (int i = pos - 1; i >= 0; i--) vga_putc(buf[i]);
        return *this;
    }

    ostream& operator<<(const Endl&) {
        vga_puts("\n");
        return *this;
    }
};

inline ostream kcout;

} // namespace std

#endif