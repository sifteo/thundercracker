
# test IDs

NrfCommsID = 0
ExternalFlashCommsID = 1
ExternalFlashReadWriteID = 2
LedID = 3
UniqueIdID = 4

def NrfComms(devMgr):
    uart = devMgr.masterUART()
    txpower = 0
    uart.writeMsg([NrfCommsID, txpower])
    resp = uart.getResponse()
    
    if resp.opcode != NrfCommsID:
        return False
    
    success = (resp.payload[0] != 0)
    return success

def ExternalFlashComms(devMgr):
    uart = devMgr.masterUART()
    uart.writeMsg([ExternalFlashCommsID])
    resp = uart.getResponse()

    success = ((resp.opcode == ExternalFlashCommsID) and (resp.payload[0] != 0))
    return success
    
def ExternalFlashReadWrite(devMgr):
    uart = devMgr.masterUART()
    sectorAddr = 0
    offset = 0
    uart.writeMsg([ExternalFlashReadWriteID, sectorAddr, offset])
    resp = uart.getResponse()

    success = ((resp.opcode == ExternalFlashReadWriteID) and (resp.payload[0] != 0))
    return success
    
def LedHelper(uart, color, code):
    uart.writeMsg([LedID, code])
    resp = uart.getResponse()
    
    s = raw_input("Is LED %s? (y/n) " % color)
    if resp.opcode != LedID or not s.startswith("y"):
        return False
    return True
    
def LedTest(devMgr):
    uart = devMgr.masterUART()
    
    combos = { "green" : 1,
                "red" : 2,
                "both on" : 3 }

    for key, val in combos.iteritems():
        if not LedHelper(uart, key, val):
            return False
    
    # make sure off is last
    if not LedHelper(uart, "off", 0):
        return False

    return True

def UniqueIdTest(devMgr):
    uart = devMgr.masterUART()
    uart.writeMsg([UniqueIdID])
    resp = uart.getResponse()
    
    if resp.opcode == UniqueIdID and len(resp.payload) == 12:
        # TODO: capture this, do something with it?
        return True
    else:
        return False
    
