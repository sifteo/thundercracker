#include <sifteo.h>
using namespace Sifteo;

static Metadata M = Metadata()
    .title("Accelerometer Chart")
    .package("com.sifteo.extras.accelchart", "0.1")
    .cubeRange(1);

void main()
{
    static VideoBuffer vid;
    static _SYSMotionBuffer buf;

    vid.attach(0);
    vid.initMode(FB64);
    vid.colormap[0] = 0xdddddd;
    vid.colormap[1] = 0x000088;

    buf.header.last = 0xFF;
    _SYS_setMotionBuffer(0, &buf);

    int y = 0;
    int prevX = 32;
    uint8_t head = 0;
    unsigned ticks = 0;
    const unsigned ticksPerPixel = 50;

    while (1) {
        while (head != buf.header.tail) {
            _SYSByte4 sample = buf.buf[head++];
            ticks += uint8_t(sample.w);

            while (ticks >= ticksPerPixel) {
                ticks -= ticksPerPixel;

                int x = clamp(32 + sample.x / 2, 0, 63);

                // Draw a line from old X to new X
                int x1 = MIN(x, prevX+1);
                int x2 = MAX(x, prevX-1);
                vid.fb64.span(vec(x1,y), x2-x1+1, 1);
                prevX = x;

                // Erase one row ahead
                if (++y == 64) y = 0;
                vid.fb64.span(vec(0,y), 64, 0);
            }
        }

        System::paint();
    }
}
