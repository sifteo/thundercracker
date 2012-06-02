/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
using namespace Sifteo;

static Metadata M = Metadata()
    .title("TinySynth")
    .cubeRange(1);

static const CubeID cube = 0;
static VideoBuffer vid;


void synthesize(float hz, float timbre, float volume)
{
    LOG("hz=%f timbre=%f volume=%f\n", hz, timbre, volume);
    
    const AudioChannel channel(0);
    static int16_t sampleWave[512];
    static const AssetAudio sampleAsset = {{
        /* sampleRate  */  0,
        /* loopStart   */  0,
        /* loopEnd     */  arraysize(sampleWave) - 1,
        /* loopType    */  _SYS_LOOP_REPEAT,
        /* type        */  _SYS_PCM,
        /* volume      */  0,
        /* dataSize    */  sizeof sampleWave,
        /* pData       */  reinterpret_cast<uint32_t>(sampleWave),
    }};

    int dutyCycle = timbre * arraysize(sampleWave);
    for (int i = 0; i != arraysize(sampleWave); i++)
        sampleWave[i] = (dutyCycle < i) ? 10000 : -10000;

    channel.play(sampleAsset);
    channel.setVolume(volume * 255.f);
    channel.setSpeed(hz * arraysize(sampleWave));
}

void main()
{
    unsigned fg = BG0ROMDrawable::SOLID_FG ^ BG0ROMDrawable::LTBLUE;
    unsigned bg = BG0ROMDrawable::SOLID_FG ^ BG0ROMDrawable::BLACK;
    
    vid.initMode(BG0_ROM);
    vid.bg0rom.erase(bg);
    vid.bg0rom.fill(vec(0,0), vec(3,3), fg);
    vid.attach(cube);

    float hz = 0;

    while (1) {
        // Scale to [0, 1]
        auto accel = (cube.accel() + vec(128,128,128)) / 255.f;

        // Glide to the target note (half-steps above or below middle C)
        float note = 261.6f * pow(1.05946f, round(accel.y * 24.f));
        hz += (note - hz) * 0.4f;

        synthesize(hz, accel.x, accel.x);

        vid.bg0rom.setPanning(accel.xy() * -(LCD_size - vec(24,24)));
        System::paint();
    }
}
