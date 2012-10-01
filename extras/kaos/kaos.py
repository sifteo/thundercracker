import Image
from itertools import *
import random
import sys

num_tiles = 4000

result = Image.new('RGBA', (8,num_tiles*8))
w,h = result.size
p = result.load()

for y,x in product(xrange(h), xrange(w)):
	p[x,y] = (random.randint(0,255), random.randint(0,255), random.randint(0,255), 255)

if len(sys.argv) == 1:
	result.show()
else:
	result.save(sys.argv[1])
