#include <stdint.h>

void _SYS_paint(void);
void _SYS_ticks_ns(int64_t *nanosec);
void _SYS_strlcpy(char *dest, const char *src, uint32_t destSize);
void _SYS_strlcat_int(char *dest, int src, uint32_t destSize);

static char buf[128];

void siftmain() {
    int64_t now = 0, then = 0;
    
	while (1) {
        _SYS_ticks_ns(&now);
        int64_t delta = now - then;

        if (delta > 100000) {
            then = now;
            _SYS_strlcpy(buf, "Ticks: ", sizeof buf);
            _SYS_strlcat_int(buf, (int)delta, sizeof buf);
        }

        _SYS_paint();
	}
}
