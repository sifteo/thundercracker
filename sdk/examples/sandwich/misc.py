# miscellaneous utility functions

def spiral_into_madness():
	x,y = (4,4)
	yield (x,y)
	y+=1
	yield (x,y)
	x+=1 
	y-=1
	yield (x,y)
	x-=1 
	y-=1
	yield (x,y)
	x-=1 
	y+=1
	yield (x,y)
	y+=1
	yield (x,y)
	x+=2
	yield (x,y)
	y-=2
	yield (x,y)
	x-=2
	yield (x,y)
	x += 1 
	y += 3
	yield (x,y)
	x += 2 
	y -= 2
	yield (x,y)
	x -= 2 
	y -= 2
	yield (x,y)
	x -= 2 
	y += 2
	yield (x,y)
	y +=1
	yield (x,y)
	x+=1 
	y +=1
	yield (x,y)
	x += 2
	yield (x,y)
	x+=1 
	y-=1
	yield (x,y)
	y-=2
	yield (x,y)
	x-=1 
	y-=1
	yield (x,y)
	x -= 2
	yield (x,y)
	x-=1 
	y+=1
	yield (x,y)
	y += 3
	yield (x,y)
	x += 4
	yield (x,y)
	y -= 4
	yield (x,y)
	x -= 4
	yield (x,y)

	#x += 2 
	#y += 5
	#yield (x,y)
	#x+=3 
	#y -= 3
	#yield (x,y)
	#x -= 3 
	#y -= 3
	#yield (x,y)
	#x -= 3 
	#y += 3
	#yield (x,y)
