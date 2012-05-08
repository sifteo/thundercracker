import time
from factorytest_common import rangeHelper, AckPacket

# cube test IDs
SensorsReportEnabledID  = 7

# cube event IDs
AckPacketRxedID         = 6

################################################################
# NOTE: All functions in this module with 'Test' in the name
#       will be discovered and run by factorytest.py
################################################################

# helper to enable/disable sensor reporting
def SetSensorReportingEnabled(devMgr, enabled):
    jig = devMgr.testjig()
    jig.txPacket([SensorsReportEnabledID, enabled])
    
    resp = jig.rxPacket()
    return resp.opcode == SensorsReportEnabledID

def AccelerometerTest(devMgr):

    SetSensorReportingEnabled(devMgr, 1)
    jig = devMgr.testjig();

    count = 0
    while count < 1000:
        resp = jig.rxPacket(timeout = -1)
        if resp.opcode == AckPacketRxedID:
            pkt = AckPacket(resp.payload)
            print "x:y:z - %d:%d:%d" % (pkt.accelX, pkt.accelY, pkt.accelZ)
            count = count + 1

    SetSensorReportingEnabled(devMgr, 0)
