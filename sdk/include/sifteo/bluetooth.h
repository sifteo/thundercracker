/*
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>

namespace Sifteo {

/**
 * @defgroup bluetooth Bluetooth
 *
 * @brief Support for communicating with mobile devices via Bluetooth.
 *
 * Some Sifteo Bases include support for Bluetooth 4.0 Low Energy, also known
 * as Bluetooth Smart. This is a standard designed for low-power exchange of
 * small amounts of data between mobile devices.
 *
 * Bluetooth Low Energy is slow, maxing out at about one kilobyte per second.
 * It is designed for exchanging small bits of game state, or synchronizing
 * small amounts of saved game data. It should not be used for transferring
 * large files.
 *
 * This module provides a very high-level interface to the Bluetooth radio.
 * Pairing, connection state, encryption, and non-game-specific operations
 * including filesystem access are implemented by the system. Some apps
 * which synchronize state or unlock features may find this functionality
 * to be sufficient, and very little in-game code may be necessary.
 *
 * For games that want to communicate directly with mobile devices during
 * gameplay, BluetoothPipe can be used to exchange small packets with a
 * peer device.
 *
 * @{
 */


/**
 * @brief Global Bluetooth operations.
 *
 * The system provides a basic level of Bluetooth functionality without any
 * game-specific code. This includes filesystem access, pairing the base with
 * a Sifteo user account, getting information about the running game, and
 * launching a game.
 */

class Bluetooth {
public:

    /**
     * @brief Is Bluetooth support available on this hardware?
     *
     * Returns 'true' if the Base we're running on has firmware and
     * hardware that support Bluetooth. If not, returns 'false'.
     *
     * Other Bluetooth functions must only be called if isAvailable() 
     * returns 'true'.
     */

    static bool isAvailable()
    {
        if ((_SYS_getFeatures() & _SYS_FEATURE_BLUETOOTH) == 0) {
            return false;
        }

        return _SYS_bt_isAvailable();
    }

    /**
     * @brief Set the current advertised game state
     *
     * When a device has connected to the Sifteo Base's system software
     * but hasn't necessarily connected to a game's BluetoothPipe,
     * it can still retrieve basic information about the running game.
     *
     * This basic information is "advertised" to the mobile device, which
     * can use it to determine game state and possibly to determine whether
     * it wants to connect.
     *
     * This basic information that we advertise includes a game's
     * package name and version. Games may optionally specify an additional
     * game-specific binary blob to include in the advertisement data.
     *
     * The provided advertisement data is atomically copied to a system buffer.
     * The application can reuse the buffer memory immediately.
     *
     * To disable this additional advertisement data, call advertiseState()
     * with an empty or NULL buffer.
     */

    static void advertiseState(const uint8_t *bytes, unsigned length) {
        _SYS_bt_advertiseState(bytes, length);
    };
};


/**
 * @brief A container for one Bluetooth packet's data.
 *
 * Each packet consists of a variable-length payload of up to 19 bytes, and
 * a "type" byte of which the low 7 bits are available for games to use.
 */

struct BluetoothPacket {
    _SYSBluetoothPacket sys;

    // Implicit conversions
    operator _SYSBluetoothPacket* () { return &sys; }
    operator const _SYSBluetoothPacket* () const { return &sys; }

    /// Retrieve the capacity of a packet, in bytes. (19)
    static unsigned capacity() {
        return _SYS_BT_PACKET_BYTES;
    }

    /// Set the packet's length and type to zero.
    void clear() {
        resize(0);
        setType(0);
    }

    /// Retrieve the size of this packet, in bytes.
    unsigned size() const {
        return sys.length;
    }

    /// Change the size of this packet, in bytes.
    void resize(unsigned bytes) {
        ASSERT(bytes <= capacity());
        sys.length = bytes;
    }

    /// Is this packet's payload empty?
    bool empty() const {
        return size() == 0;
    }

    /// Return a pointer to this packet's payload bytes.
    uint8_t *bytes() {
        return sys.bytes;
    }
    const uint8_t *bytes() const {
        return sys.bytes;
    }

    /// Return the packet's 7-bit type code
    unsigned type() const {
        return sys.type;
    }

    /// Set a packet's 7-bit type.
    void setType(unsigned type) {
        ASSERT(type <= 127);
        sys.type = type;
    }
};


/**
 * @brief A memory buffer which holds a queue of Bluetooth packets.
 *
 * This is a FIFO buffer which holds packets that the game has created
 * but is waiting to transmit, or packets which have been received but
 * not yet processed by the game.
 *
 * BluetoothQueues are templatized by buffer size. The system supports
 * buffers with between 1 and 256 packets of capacity.
 */

template < unsigned tSize >
struct BluetoothQueue {
    struct SysType {
        _SYSBluetoothQueueHeader header;
        _SYSBluetoothPacket packets[tSize];
    } sys;

    // Implicit conversions
    operator _SYSBluetoothQueue* () { return reinterpret_cast<_SYSBluetoothQueue*>(&sys); }
    operator const _SYSBluetoothQueue* () const { return reinterpret_cast<const _SYSBluetoothQueue*>(&sys); }
};


/**
 * @} endgroup bluetooth
 */

} // namespace Sifteo
