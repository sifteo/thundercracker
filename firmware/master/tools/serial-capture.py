#!/usr/bin/env python
#
# simple script to log timestamped messages from a serial connection.
#
# would normally just use RealTerm (or similar), but this logs
# messages to the console as they come in, and can timestamp with
# millisecond precision, where RealTerm only provides 1 second precision
#

import sys, serial, datetime

comport = sys.argv[1]
filepath = sys.argv[2]
port = serial.Serial(baudrate = 115200, port = comport)
port.flush()

with open(filepath, "w") as f:
    while True:
        line = port.readline().strip()
        time = str(datetime.datetime.now())
        log = "%s: %s\n" % (time, line)
        print log.strip()
        f.write(log)
        f.flush()
