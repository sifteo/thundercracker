
# test IDs
NrfComms = 0
ExternalFlashComms = 1
EnableTestJigNeighborTransmit = 9

def StmExternalFlashComms(devMgr):
    uart = devMgr.masterUART()
    uart.writeMsg([ExternalFlashComms])
    resp = uart.getResponse()

    success = ((resp.opcode == ExternalFlashComms) and (resp.payload[0] != 0))
    return success

def StmNeighborRx(devMgr):
    msg = [1, EnableTestJigNeighborTransmit]
    devMgr.testjig().txPacket(msg)

