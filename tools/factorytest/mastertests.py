
EnableTestJigNeighborTransmit = 9

def StmExternalFlashComms(devMgr):
    print "StmExternalFlashComms"
    msg = [1, 0]
    devMgr.testjig().txPacket(msg)

def StmNeighborRx(devMgr):
    msg = [1, EnableTestJigNeighborTransmit]
    devMgr.testjig().txPacket(msg)

