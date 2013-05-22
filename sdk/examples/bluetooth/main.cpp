/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

Metadata M = Metadata()
    .title("Bluetooth SDK Example")
    .package("com.sifteo.sdk.bluetooth", "1.0")
    .icon(Icon)
    .cubeRange(1);

/*
 * When you allocate a BluetoothPipe you can optionally set the size of its transmit and receive queues.
 * We use a minimum-size transmit queue to keep latency low, since we're transmitting continuously.
 */
BluetoothPipe <1,8> btPipe;

// Bluetooth packet counters, available for debugging
BluetoothCounters btCounters;

VideoBuffer vid;

void onCubeTouch(void *, unsigned);
void onConnect();
void onDisconnect();
void onReadAvailable();
void onWriteAvailable();
void updatePacketCounts(int tx, int rx);


void main()
{
    /*
     * Display text in BG0_ROM mode on Cube 0
     */

    CubeID cube = 0;
    vid.initMode(BG0_ROM);
    vid.attach(cube);
    vid.bg0rom.text(vec(0,0), " Bluetooth Demo ", vid.bg0rom.WHITE_ON_TEAL);

    /*
     * If Bluetooth isn't supported, don't go on.
     */

    if (!Bluetooth::isAvailable()) {
        vid.bg0rom.text(vec(1,2), "Not supported");
        vid.bg0rom.text(vec(1,3), "on this Base.");
        vid.bg0rom.plot(vec(7,5), vid.bg0rom.FROWN);
        while (1)
            System::paint();
    }

    // Zero out our counters
    btCounters.reset();

    /*
     * Advertise some "game state" to the peer. Mobile apps can read this
     * "advertisement" buffer without interrupting the game. This may have
     * information that's useful on its own, or it may be information that
     * tells a mobile app whether or not we're in a game state where Bluetooth
     * interaction makes sense.
     *
     * In this demo, we'll report the number of times the cube has been touched,
     * and its current touch state. Advertisement data is totally optional, and
     * if your game doesn't have a use for this feature it's fine not to use it.
     */

    Events::cubeTouch.set(onCubeTouch);
    onCubeTouch(0, cube);

    /*
     * Handle sending and receiving Bluetooth data entirely with Events.
     * Our BluetoothPipe is a buffer that holds packets that have been
     * received and are waiting to be processed, and packets we're waiting
     * for the system to send.
     *
     * To demonstrate the system's performance, we'll be trying to send
     * and receive packets as fast as possible. Every time there's buffer
     * space available in the transmit queue, we'll fill it with a packet.
     * Likewise, every received packet will be dealt with as soon as possible.
     *
     * When we transmit packets in this example, we'll fill them with our
     * cube's accelerometer and touch state. When we receive packets, they'll
     * be hex-dumped to the screen. We also keep counters that show how many
     * packets have been processed.
     *
     * If possible applications are encouraged to use event handlers so that
     * they only try to read packets when packets are available, and they only
     * write packets when buffer space is available. In this example, we always
     * want to read packets when they arrive, so we keep an onRead() handler
     * registered at all times. We also want to write as long as there's buffer
     * space, but only when a peer is connected. So we'll register and unregister
     * our onWrite() handler in onConnect() and onDisconnect(), respectively.
     *
     * Note that attach() will empty our transmit and receive queues. If we want
     * to enqueue write packets in onConnct(), we need to be sure the pipe is
     * attached before we set up onConnect/onDisconnect.
     */

    Events::bluetoothReadAvailable.set(onReadAvailable);
    btPipe.attach();
    updatePacketCounts(0, 0);

    /*
     * Watch for incoming connections, and display some text on the screen to
     * indicate connection state.
     */

    Events::bluetoothConnect.set(onConnect);
    Events::bluetoothDisconnect.set(onDisconnect);

    if (Bluetooth::isConnected()) {
        onConnect();
    } else {
        onDisconnect();
    }

    /*
     * Everything else happens in event handlers, nothing to do in our main loop.
     */

    while (1) {

        for (unsigned n = 0; n < 60; n++) {
            System::paint();
        }

        /*
         * For debugging, periodically log the Bluetooth packet counters.
         */

        btCounters.capture();
        LOG("BT-Counters: rxPackets=%d txPackets=%d rxBytes=%d txBytes=%d rxUserDropped=%d\n",
            btCounters.receivedPackets(), btCounters.sentPackets(),
            btCounters.receivedBytes(), btCounters.sentBytes(),
            btCounters.userPacketsDropped());
    }
}

void onCubeTouch(void *context, unsigned id)
{
    /*
     * Keep track of how many times this cube has been touched
     */

    CubeID cube = id;
    bool isTouching = cube.isTouching();
    static uint32_t numTouches = 0;

    if (isTouching) {
        numTouches++;
    }

    /*
     * Set our current Bluetooth "advertisement" data to a blob of information about
     * cube touches. In a real game, you'd use this to store any game state that
     * a mobile app may passively want to know without explicitly requesting it
     * from the running game.
     *
     * This packet can be up to 19 bytes long. We'll use a maximum-length packet
     * as an example, but this packet can indeed be shorter.
     */

    struct {
        uint8_t count;
        uint8_t touch;
        uint8_t placeholder[17];
    } packet = {
        numTouches,
        isTouching
    };

    // Fill placeholder with some sequential bytes
    for (unsigned i = 0; i < sizeof packet.placeholder; ++i)
        packet.placeholder[i] = 'A' + i;

    LOG("Advertising state: %d bytes, %19h\n", sizeof packet, &packet);
    Bluetooth::advertiseState(packet);
}

void onConnect()
{
    LOG("onConnect() called\n");
    ASSERT(Bluetooth::isConnected() == true);

    vid.bg0rom.text(vec(0,2), "   Connected!   ");
    vid.bg0rom.text(vec(0,3), "                ");
    vid.bg0rom.text(vec(0,8), " Last received: ");

    // Start trying to write immediately
    Events::bluetoothWriteAvailable.set(onWriteAvailable);
    onWriteAvailable();
}

void onDisconnect()
{
    LOG("onDisconnect() called\n");
    ASSERT(Bluetooth::isConnected() == false);

    vid.bg0rom.text(vec(0,2), " Waiting for a  ");
    vid.bg0rom.text(vec(0,3), " connection...  ");

    // Stop trying to write
    Events::bluetoothWriteAvailable.unset();
}

void updatePacketCounts(int tx, int rx)
{
    // Update and draw packet counters

    static int txCount = 0;
    static int rxCount = 0;

    txCount += tx;
    rxCount += rx;

    String<17> str;
    str << "RX: " << rxCount;
    vid.bg0rom.text(vec(1,6), str);

    str.clear();
    str << "TX: " << txCount;
    vid.bg0rom.text(vec(1,5), str);
}

void packetHexDumpLine(const BluetoothPacket &packet, String<17> &str, unsigned index)
{
    str.clear();

    // Write up to 8 characters
    for (unsigned i = 0; i < 8; ++i, ++index) {
        if (index < packet.size()) {
            str << Hex(packet.bytes()[index], 2);
        } else {
            str << "  ";
        }
    }
}

void onReadAvailable()
{
    LOG("onReadAvailable() called\n");

    /*
     * This is one way to read packets from the BluetoothPipe; using read(),
     * and copying them into our own buffer. A faster but slightly more complex
     * method would use peek() to access the next packet, and pop() to remove it.
     */

    BluetoothPacket packet;
    while (btPipe.read(packet)) {

        /*
         * We received a packet over the Bluetooth link!
         * Dump out its contents in hexadecimal, to the log and the display.
         */

        LOG("Received: %d bytes, type=%02x, data=%19h\n",
            packet.size(), packet.type(), packet.bytes());

        String<17> str;

        str << "len=" << Hex(packet.size(), 2) << " type=" << Hex(packet.type(), 2);
        vid.bg0rom.text(vec(1,10), str);

        packetHexDumpLine(packet, str, 0);
        vid.bg0rom.text(vec(0,12), str);

        packetHexDumpLine(packet, str, 8);
        vid.bg0rom.text(vec(0,13), str);

        packetHexDumpLine(packet, str, 16);
        vid.bg0rom.text(vec(0,14), str);

        // Update our counters
        updatePacketCounts(0, 1);
    }
}

void onWriteAvailable()
{
    LOG("onWriteAvailable() called\n");

    /*
     * This is one way to write packets to the BluetoothPipe; using reserve()
     * and commit(). If you already have a buffer that you want to copy to the
     * BluetoothPipe, you can use write().
     */

    while (Bluetooth::isConnected() && btPipe.writeAvailable()) {

        /*
         * Access some buffer space for writing the next packet. This
         * is the zero-copy API for writing packets. Both reading and writing
         * have a traditional (one copy) API and a zero-copy API.
         */

        BluetoothPacket &packet = btPipe.sendQueue.reserve();

        /*
         * Fill most of the packet with dummy data
         */

        // 7-bit type code, for our own application's use
        packet.setType(0x5A);

        packet.resize(packet.capacity());

        for (unsigned i = 0; i < packet.capacity(); ++i) {
            packet.bytes()[i] = 'a' + i;
        }

        /* 
         * Fill the first 3 bytes with accelerometer data from Cube 0
         */

        Byte3 accel = vid.physicalAccel();

        packet.bytes()[0] = accel.x;
        packet.bytes()[1] = accel.y;
        packet.bytes()[2] = accel.z;

        /*
         * Log the packet for debugging, and commit it to the FIFO.
         * The system will asynchronously send it to our peer.
         */

        LOG("Sending: %d bytes, type=%02x, data=%19h\n",
            packet.size(), packet.type(), packet.bytes());

        btPipe.sendQueue.commit();
        updatePacketCounts(1, 0);
    }
}
