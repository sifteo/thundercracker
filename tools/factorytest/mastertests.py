import time

################################################################
# NOTE: All functions in this module with 'Test' in the name
#       will be discovered and run by factorytest.py
################################################################

# master command IDs
NrfCommsID                  = 0
ExternalFlashCommsID        = 1
ExternalFlashReadWriteID    = 2
LedID                       = 3
UniqueIdID                  = 4

# testjig command IDs
SetUsbEnabledID             = 0
SimulatedBatteryVoltageID   = 1
GetBattSupplyCurrentID      = 2

def _rangeHelper(value, min1, max1, min2, max2):
    range1 = (max1 - min1)
    range2 = (max2 - min2)
    return (((value - min1) * range2) / range1) + min2

def _setUsbEnabled(jig, enabled):
    pkt = [SetUsbEnabledID, enabled]
    jig.txPacket(pkt)
    resp = jig.rxPacket()
    return resp.opcode == SetUsbEnabledID

def NrfCommsTest(devMgr):
    uart = devMgr.masterUART()
    txpower = 0
    uart.writeMsg([NrfCommsID, txpower])
    resp = uart.getResponse()

    if resp.opcode != NrfCommsID:
        return False

    success = (resp.payload[0] != 0)
    return success

def ExternalFlashCommsTest(devMgr):
    uart = devMgr.masterUART()
    uart.writeMsg([ExternalFlashCommsID])
    resp = uart.getResponse()

    success = ((resp.opcode == ExternalFlashCommsID) and (resp.payload[0] != 0))
    return success

def ExternalFlashReadWriteTest(devMgr):
    uart = devMgr.masterUART()
    sectorAddr = 0
    offset = 0
    uart.writeMsg([ExternalFlashReadWriteID, sectorAddr, offset])
    resp = uart.getResponse()

    success = ((resp.opcode == ExternalFlashReadWriteID) and (resp.payload[0] != 0))
    return success

def _ledHelper(uart, color, code):
    uart.writeMsg([LedID, code])
    resp = uart.getResponse()

    s = raw_input("Is LED %s? (y/n) " % color)
    if resp.opcode != LedID or not s.startswith("y"):
        return False
    return True

def LedTest(devMgr):
    uart = devMgr.masterUART()

    combos = { "green" : 1,
                "red" : 2,
                "both on" : 3 }

    for key, val in combos.iteritems():
        if not _ledHelper(uart, key, val):
            return False

    # make sure off is last
    if not _ledHelper(uart, "off", 0):
        return False

    return True

def UniqueIdTest(devMgr):
    uart = devMgr.masterUART()
    uart.writeMsg([UniqueIdID])
    resp = uart.getResponse()

    if resp.opcode == UniqueIdID and len(resp.payload) == 12:
        # TODO: capture this, do something with it?
        return True
    else:
        return False

# disconnect USB, set the simulated battery voltage, and ensure
# that the power being drawn by the master is acceptable
def VBattCurrentDrawTest(devMgr):
    jig = devMgr.testjig();
    _setUsbEnabled(jig, False)

    # Master Power Iteration - cycles from 3.2V down to 2.0V and then back up
    # TODO: tune windows for success criteria
    measurements = ( (3.2, 0, 0xfff),
                     (2.9, 0, 0xfff),
                     (2.6, 0, 0xfff),
                     (2.3, 0, 0xfff),
                     (2.0, 0, 0xfff) )
    for m in measurements:
        voltage = m[0]
        minCurrent = m[1]
        maxCurrent = m[2]

        # set voltage
        # scale voltages to the 12-bit DAC output, and send them LSB first
        bit12 = int(_rangeHelper(voltage, 0.0, 3.3, 0, 0xfff))
        pkt = [SimulatedBatteryVoltageID, bit12 & 0xff, (bit12 >> 8) & 0xff]
        jig.txPacket(pkt)
        resp = jig.rxPacket()
        if resp.opcode != SimulatedBatteryVoltageID:
            return False

        time.sleep(1)

        # read current & verify against window
        jig.txPacket([GetBattSupplyCurrentID])
        resp = jig.rxPacket()
        if resp.opcode != GetBattSupplyCurrentID or len(resp.payload) < 2:
            return False
        current = resp.payload[0] | resp.payload[1] << 8
        print "current draw @ %.2fV: 0x%x" % (voltage, current)
        if current < minCurrent or current > maxCurrent:
            return False

    return True
