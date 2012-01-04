import serial
import struct

# helpful links:
# http://stackoverflow.com/questions/6076300/python-pack-4-byte-integer-with-bytes-in-bytearray-struct-pack

if __name__ == '__main__':
	
	#port = serial.Serial(port = "COM17", baudrate = 115200, timeout = 2)
	#port = serial.Serial('/dev/tty.usbmodem621', 115200, timeout=1)
	port = serial.Serial('/dev/tty.usbmodem411', 115200, timeout=1)
	print port
	
	
	try:
		cntr = 0
		
		#f = open('asegment.bin', 'rb')
		f = open('test.txt', 'r')
		data = f.read()
		print "length: ", len(data)
		
		val = long(len(data)) 
		s = bytearray(struct.pack("i", val))
		bwrit = port.write(s)
		#print "wrote: ", bwrit
		
		for c in data:
			#print "c", c
			b = port.write(c)
			#print "wrote: ", b
			response = port.read()
			#print "response", response
			
	except KeyboardInterrupt:
		print "keyboard interrupt"
	except Exception:
		print "exception"
