import sys, time, inspect
import usb.core, usb.util, serial
import mastertests, jigtests, cubetests

IN_EP = 0x81
OUT_EP = 0x1

SIFTEO_VID  = 0x22fa    # Sifteo
MASTER_PID  = 0x0105    # Master Cube product ID
TESTJIG_PID = 0x0110    # Test Jig product ID

class TestResponse(object):
    def __init__(self, opcode, payload):
        self.opcode = opcode
        self.payload = payload
        self.length = len(payload)

class SerialDevice(object):
    def __init__(self, comport):
        self._dev = serial.Serial(baudrate = 115200, port = comport)
        self._dev.flushInput()
        self._dev.flushOutput()

    # first byte is length, followed by payload
    def writeMsg(self, payload):
        msg = [len(payload) + 1]
        msg.extend(payload)
        bytestr = "".join(chr(b) for b in msg)
        self._dev.write(bytestr)

    # first byte indicates length, then return the payload as an array
    def getResponse(self):
        numBytes = ord(self._dev.read(1))
        payloadStr = self._dev.read(numBytes - 1)
        payload = [ord(b) for b in payloadStr]

        return TestResponse(payload[0], payload[1:])

class UsbDevice(object):

    MAX_PACKET = 64

    def __init__(self, pid):
        self._dev = usb.core.find(idVendor = SIFTEO_VID, idProduct = pid)
        if self._dev is None:
            raise ValueError('device not found - product ID: 0x%04x' % pid)
        self._dev.set_configuration()

    def txPacket(self, bytes):
        self._dev.write(OUT_EP, bytes)

    def rxPacket(self, timeout = 250):
        payload = self._dev.read(IN_EP, UsbDevice.MAX_PACKET, None ,timeout = timeout)
        return TestResponse(payload[0], payload[1:])

# simple manager to lazily access devices
class DeviceManager(object):
    def __init__(self, comport = None):
        self._masterUSB = None
        self._testjig = None
        self._masterUART = None
        self._comport = comport

    def testjig(self):
        if self._testjig is None:
            self._testjig = UsbDevice(TESTJIG_PID)
        return self._testjig

    def masterUSB(self):
        if self._masterUSB is None:
            self._masterUSB = UsbDevice(MASTER_PID)
        return self._masterUSB

    def masterUART(self):
        if self._masterUART is None:
            self._masterUART = SerialDevice(self._comport)
        return self._masterUART

class TestRunner(object):
    def __init__(self, mgr):
        self.mgr = mgr
        self.successCount = 0
        self.failCount = 0

    def numTestsRun(self):
        return self.successCount + self.failCount

    def run(self, t):
        sys.stdout.write("running %s... " % t.func_name)
        success = t(self.mgr)
        if success == False:
            print "FAIL"
            self.failCount = self.failCount + 1
        else:
            print "SUCCESS"
            self.successCount = self.successCount + 1

    @staticmethod
    def runAll(devManager):
        runner = TestRunner(devManager)

        # find all methods with "Test" in them and run them
        tests = [o for o in inspect.getmembers(mastertests) if inspect.isfunction(o[1])]
        for t in tests:
            if "Test" in t[0]:
                runner.run(t[1])

        # template if you want to manually run a single test
        # runner.run(mastertests.VBattCurrentDrawTest)

        print "COMPLETE. %d tests run, %d failures" % (runner.numTestsRun(), runner.failCount)
        if runner.failCount == 0:
            return 0
        else:
            return 1

if __name__ == '__main__':

    if len(sys.argv) < 2:
        print >> sys.stderr, "usage: python factorytest.py <comport>"
        sys.exit(1)

    if sys.argv[1] == "testjig":
        comport = 0
    else:
        comport = sys.argv[1]

    result = TestRunner.runAll(DeviceManager(comport))
    sys.exit(result)
