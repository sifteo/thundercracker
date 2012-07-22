import time
from factorytest_common import rangeHelper, AckPacket

# cube test IDs
SimulatedBatteryVoltage = 1
WriteToVramID           = 6
SensorsReportEnabledID  = 7

# cube event IDs
AckPacketRxedID         = 100

# VRAM constants
VRAM_ADDRESS_MODE       = 0x3fe
VRAM_ADDRESS_FLAGS      = 0x3ff

VRAM_FLAG_CONTINUOUS    = 0x08

VRAM_MODE_BG0ROM        = 0x04

# Used for setting battery
cube_voltage = [[0x07,0x45]]

################################################################
# NOTE: All functions in this module with 'Test' in the name
#       will be discovered and run by factorytest.py
################################################################

# enable the virtual battery
def setSimulatedBatteryVoltage(devMgr):

    usb = devMgr.testjig();
    
    
    for value in cube_voltage:
        
        usb.txPacket([SimulatedBatteryVoltage, value[1] , value[0]])
        # Ideally it would return a unique byte for each iteration
        resp = usb.rxPacket()
        print "Setting simulatred batery voltage: %s" % resp.opcode
        # from time import sleep
        # sleep(5);
    
    
    if resp.opcode != SimulatedBatteryVoltage:
        success = False
    else:
        success = True

    return success

# enable the virtual battery
def setSimulatedBatteryVoltageOff(devMgr):

    usb = devMgr.testjig();


    for value in cube_voltage:

        usb.txPacket([SimulatedBatteryVoltage, 0x00 , 0x00])
        # Ideally it would return a unique byte for each iteration
        resp = usb.rxPacket()
        print "Setting simulatred batery voltage: %s" % resp.opcode
        # from time import sleep
        # sleep(5);


    if resp.opcode != SimulatedBatteryVoltage:
        success = False
    else:
        success = True

    return success

# helper to enable/disable sensor reporting
def SetSensorReportingEnabled(devMgr, enabled):
    jig = devMgr.testjig()
    
    jig.txPacket([SensorsReportEnabledID, enabled])

    resp = jig.rxPacket()

    # print "Sensors reporing response %s" % resp.opcode
    return resp.opcode == SensorsReportEnabledID

def AccelerometerTest(devMgr):

    setSimulatedBatteryVoltage(devMgr)
    SetSensorReportingEnabled(devMgr, 1)
    jig = devMgr.testjig()

    count = 0
    while count < 1000:
        resp = jig.rxPacket(timeout = -1)
        
        print "%d : Payload: %s" % (count, resp.payload)
        # print "%d Accelerometer response %s" % (count,resp.opcode)
        
        if resp.opcode == AckPacketRxedID:
            pkt = AckPacket(resp.payload)
            # print "x:y:z - %d:%d:%d" % (pkt.accelX, pkt.accelY, pkt.accelZ)
            count = count + 1

    setSimulatedBatteryVoltageOff(devMgr)
    SetSensorReportingEnabled(devMgr, 0)
    
    return True

# a WriteToVram packet consists of the a little endian 16-bit address and byte of data
def _writeToVram(jig, address, data):
    addrLow = address & 0xff
    addrHigh = (address >> 8) & 0xff
    jig.txPacket([WriteToVramID, addrLow, addrHigh, data])
    resp = jig.rxPacket(timeout = 500)
    return resp

def Bg0RomTest(devMgr):
    jig = devMgr.testjig()

    # set continuous render mode
    _writeToVram(jig, VRAM_ADDRESS_FLAGS, VRAM_FLAG_CONTINUOUS)

    # set bg0 rom mode
    _writeToVram(jig, VRAM_ADDRESS_MODE, VRAM_MODE_BG0ROM)

    # write 7:7 (left aligned) 16-bit words along the beginning of bgo rom
    for a in xrange(500):
        valueLow = (a << 1) & 0xfe
        valueHigh = (a >> 6) & 0xfe
        _writeToVram(jig, a, valueLow)
        _writeToVram(jig, a, valueHigh)
