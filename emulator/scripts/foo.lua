
sys = System()
c = Cube(0)

sys:setOptions{numCubes=1, noThrottle=true}

sys:init()
sys:start()
sys:vsleep(0.25)

for addr = 0, 18*2-1, 1 do
   c:xwPoke(addr, addr)
end

c:xbPoke(1023, 0x08)

sys:vsleep(0.1)
print(string.format("t=%.2f wr=%d", sys:vclock(), c:lcdWriteCount()))

x, y, lcd, ref = c:testScreenshot("foo.png")
if x then
    print(string.format("Mismatch at (%d,%d), got %d, expected %d", x, y, lcd, ref))
end

sys:exit()
