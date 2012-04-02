import sys, usb.core, usb.util

IN_EP = 0x81
OUT_EP = 0x1

def getByte(dev, _timeout):
    return dev.read(IN_EP, 1, timeout = _timeout)[0]

if __name__ == '__main__':
    
    if (len(sys.argv) < 2):
        print "usage: python asset_install.py /path/to/my/assetfile"
        sys.exit(1)
    
    filepath = sys.argv[1]
    dev = usb.core.find(idVendor = 0x22fa, idProduct = 0x0105)
    if dev is None:
        raise ValueError('Device not found')
    
    try:
        blob = open(filepath, 'rb').read()
        size = len(blob)
        sz = ""
        for c in [size & 0xFF, (size >> 8) & 0xFF, (size >> 16) & 0xFF, (size >> 24) & 0xFF]:
            sz = sz + chr(c)

        dev.write(OUT_EP, sz)
        sys.stderr.write("erasing (please wait)...")
        # once we hear 0 back, erase is complete
        count = 0
        while True:
            if getByte(dev, 5000) == 0:
                break
            count = count + 1
            if count % 10 == 0:
                sys.stderr.write(".")
        sys.stderr.write("\n")

        sys.stderr.write("loading %s, %d bytes" % (filepath, size))
        count = 0
        while blob:
            blockSize = min(64, len(blob))
            count = count + 1
            if count % 100 == 0:
                sys.stderr.write(".")
            dev.write(OUT_EP, blob[:blockSize])
            blob = blob[blockSize:]

        sys.stderr.write("\n")
        
        sys.stderr.write("verifying...")
        status = getByte(dev, 100 * 1000)
        if status == 0:
            print "success!"
        else:
            print "failed:", status

    except KeyboardInterrupt:
        print "keyboard interrupt"
