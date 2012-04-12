import sys, usb.core, usb.util
import mastertests

IN_EP = 0x81
OUT_EP = 0x1

SIFTEO_VID  = 0x22fa    # Sifteo
MASTER_PID  = 0x0105    # Master Cube product ID
TESTJIG_PID = 0x0110    # Test Jig product ID

class UsbDevice(object):
    def __init__(self, pid):
        self._dev = usb.core.find(idVendor = SIFTEO_VID, idProduct = pid)
        if self._dev is None:
            raise ValueError('device not found - product ID: 0x%04x' % pid)
        self._dev.set_configuration()

    def txPacket(self, bytes):
        bytestr = "".join(chr(b) for b in bytes)
        self._dev.write(OUT_EP, bytestr)

    def rxPacket(self, numbytes, timeout):
        self._dev.read(IN_EP, numbytes, timeout = timeout)

# simple manager to lazily access devices
class DeviceManager(object):
    def __init__(self):
        self._master = None
        self._testjig = None

    def testjig(self):
        if self._testjig is None:
            self._testjig = UsbDevice(TESTJIG_PID)
        return self._testjig

    def master(self):
        if self._master is None:
            self._master = UsbDevice(MASTER_PID)
        return self._master

def runAllTests():
    mgr = DeviceManager()

    # mastertests.StmExternalFlashComms(mgr)
    mastertests.StmNeighborRx(mgr)

if __name__ == '__main__':

    if (len(sys.argv) == 1):
        runAllTests()
    else:
        print "TODO: run specific tests"
