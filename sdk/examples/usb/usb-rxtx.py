#!/usr/bin/env python

#
# Simple PyUSB demo to interact with the Sifteo SDK USB example.
#

import usb # expects PyUSB (http://pyusb.sourceforge.net) to be installed
import sys

SIFTEO_VID  = 0x22fa
BASE_PID    = 0x0105

IN_EP       = 0x81
OUT_EP      = 0x01
INTF        = 0x0
MAX_PACKET  = 64

USER_SUBSYS = 7

def find_and_open():
    dev = usb.core.find(idVendor = SIFTEO_VID, idProduct = BASE_PID)
    if dev is None:
        sys.stderr.write('Device is not connected\n')
        sys.exit(1)

    # set the active configuration. With no arguments, the first
    # configuration will be the active one
    dev.set_configuration()

    return dev

def send(dev, bytes, timeout = 1000):
    """
    Write a byte array to the device.
    """

    # Ensure that our message will be dispatched appropriately by the base.
    # Highest 4 bits specify the subsystem, user subsystem is 7.
    USER_HDR = USER_SUBSYS << 4
    msg = [0, 0, 0, USER_HDR]
    msg.extend(bytes)

    if len(msg) > MAX_PACKET:
        raise ValueError("msg is too long")

    return dev.write(OUT_EP, msg, INTF, timeout)

def receive(dev, timeout=500):
    """
    Read a byte array from the device.
    """
    try:
        # catch timeouts, and just return empty a byte array
        msg = dev.read(IN_EP, MAX_PACKET, INTF, timeout)
    except usb.core.USBError:
        return -1, []

    if len(msg) < 4:
        return -1, []

    subsystem = (msg[3] & 0xff) >> 4
    if subsystem != USER_SUBSYS:
        return -1, []

    return type, msg[4:]

dev = find_and_open()
b = 0
while True:
    msg = [b]
    b = (b + 1) % 100
    send(dev, msg)

    type, payload = receive(dev)
    if len(payload) >= 3:
        print "accel:", payload[0], payload[1], payload[2]

