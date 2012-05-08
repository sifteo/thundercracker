
# Routines common to multiple test modules

def rangeHelper(value, min1, max1, min2, max2):
    range1 = (max1 - min1)
    range2 = (max2 - min2)
    return (((value - min1) * range2) / range1) + min2

# helper to convert from uint8 to int8
def _int8(n):
    return (n & 127) - (n & 128)

# simple struct-like object to parse out the contents of an ACK payload
class AckPacket(object):
    def __init__(self, payload):
        self.frameCount = payload[0]
        self.accelX = _int8(payload[1])
        self.accelY = _int8(payload[2])
        self.accelZ = _int8(payload[3])
        self.nbr1 = payload[4]
        self.nbr2 = payload[5]
        self.nbr3 = payload[6]
        self.nbr4 = payload[7]
        self.flashFifoBytes = payload[8]
        self.battery = payload[10] << 8 | payload[9]