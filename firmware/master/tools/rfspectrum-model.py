#!/usr/bin/env python

import sys, random, time

NUM_BUCKETS = 83 >> 1
MAX_RETRIES = 128 * 15 # 1920
GAIN = 0.8
WIFI_CH_WIDTH = 20
# K = 1500.0 / MAX_RETRIES
K = float(MAX_RETRIES / WIFI_CH_WIDTH)
ROWS = 30

HIDDEN_SPECTRUM = [ (100, 900)[i >= 4 and i <= 8] for i in xrange(NUM_BUCKETS) ]


class RfSpectrum(object):
    
    def init(self):
        self.buckets = [0.0 for i in xrange(NUM_BUCKETS)]
    
    @staticmethod
    def scalefactor(i, channel):
        distance = abs(i - channel)
        return max(GAIN - float(distance) * K, 0.0)
    
    def update(self, channel, retry_count):
        
        if retry_count > MAX_RETRIES:
            retry_count = MAX_RETRIES
        
        for i, b in enumerate(self.buckets):
            sf = RfSpectrum.scalefactor(i, channel)
            newval = (b * (1.0 - sf)) + (retry_count * sf)
            self.buckets[i] = newval
    
    def ascii_dump(self):
        print "\n********************************************"
        print "                RfSpectrum"
        print "********************************************\n"
        
        for row in reversed(range(ROWS)):
            rowsegment = (MAX_RETRIES / ROWS) * row
            for b in self.buckets:
                if b <= rowsegment:
                    sys.stdout.write(" - |")
                else:
                    sys.stdout.write(" o |")
            sys.stdout.write("\n")


if __name__ == "__main__":
    
    spectrum = RfSpectrum()
    spectrum.init()
    
    while True:
        
        ch = random.randrange(0, NUM_BUCKETS)
        for v in xrange(5):
            retries = random.gauss(HIDDEN_SPECTRUM[ch], 50)
            print ch, retries
            spectrum.update(ch, retries)
        
        spectrum.ascii_dump()
        time.sleep(1.0 / 30.0)