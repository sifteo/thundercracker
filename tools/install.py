import sys, serial

TIMEOUT = 2
LONG_TIMEOUT = 100

def waitForStatus(port):
    port.timeout = LONG_TIMEOUT # this might take a while
    status = port.read()
    port.timeout = TIMEOUT
    return ord(status)

if __name__ == '__main__':
    
    if (len(sys.argv) < 3):
        print "usage: python asset_install.py COM# /path/to/my/assetfile"
        sys.exit(1)
    
    portName = sys.argv[1]
    filepath = sys.argv[2]
    port = serial.Serial(port = portName, baudrate = 115200, timeout = TIMEOUT)
    port.flushInput()
    
    try:
        blob = open(filepath, 'rb').read()
        size = len(blob)
        sz = ""
        for c in [size & 0xFF, (size >> 8) & 0xFF, (size >> 16) & 0xFF, (size >> 24) & 0xFF]:
            sz = sz + chr(c)

        print "erasing (please wait)..."
        port.write(sz)
        status = waitForStatus(port)
        if status != 0:
            print "sad status", status

        sys.stderr.write("loading %s, %d bytes" % (filepath, size))
        while blob:
            blockSize = min(16384, len(blob))
            sys.stderr.write(".")
            port.write(blob[:blockSize])
            blob = blob[blockSize:]

        sys.stderr.write("\n")
        
        sys.stderr.write("verifying...")
        status = waitForStatus(port)
        if status == 0:
            print "success!"
        else:
            print "verification failed", status

    except KeyboardInterrupt:
        print "keyboard interrupt"

    port.close()
