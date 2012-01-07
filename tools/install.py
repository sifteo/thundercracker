import sys, serial

if __name__ == '__main__':
	
	if (len(sys.argv) < 2):
		print "must specify file to install"
		print "usage: python asset_install.py /path/to/my/assetfile"
		sys.exit(1)
	
	filepath = sys.argv[1]
	port = serial.Serial(port = "COM21", baudrate = 115200, timeout = 2)
	port.flushInput()
	
	try:
		blob = open(filepath, 'rb').read()
		size = len(blob)
		sz = ""
		for c in [size & 0xFF, (size >> 8) & 0xFF, (size >> 16) & 0xFF, (size >> 24) & 0xFF]:
			sz = sz + chr(c)
		print "loading %s, %d bytes" % (filepath, size)
		port.write(sz)
		port.write(blob)
		
	except KeyboardInterrupt:
		print "keyboard interrupt"

	port.close()
