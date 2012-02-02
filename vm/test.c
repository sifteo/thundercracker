#include <stdint.h>

#define _SC(n)  __asm__ ("_SYS_" #n)

void _SYS_paint(void) _SC(0x0100);
void _SYS_ticks_ns(int64_t *nanosec) _SC(0x0101);
void _SYS_strlcpy(char *dest, const char *src, uint32_t destSize) _SC(0x0102);
void _SYS_strlcat_int(char *dest, int src, uint32_t destSize) _SC(0x0103);

#if 0
int func(char *buf, int a) {
    _SYS_strlcpy(buf, a << 5, (a << 4) | 3);
    return a+1;
}
#endif

#if 0
void siftmain() {
    int64_t now, then = 0;
    char buf[16];

	while (1) {
        _SYS_ticks_ns(&now);
        int64_t delta = now - then;

        func(buf, 0x12345678);
        func(buf, 0xabcdef00);
        
        /*
        if (delta > 100000) {
            then = now;
            _SYS_strlcat_int(buf, (int)delta, sizeof buf);
        }*/

        _SYS_paint();
	}
}
#endif


void __attribute__ ((noinline)) p3() {
    _SYS_paint();
    _SYS_paint();
    _SYS_paint();
}
    
void main() {
    while (1) {
        _SYS_ticks_ns(0xf00fbabe);
        p3();
    }
}

