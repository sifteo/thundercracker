
# test IDs

UsbPower = 0
SimulatedBatteryVoltage = 1

# Cube Power Iteration - cycles from 1.5V down to .8 and then back up
# [ 1.5V, 1.2V, 1.0V, 0.8V ]

# cube_voltage = [[0x07,0x45], [0x05,0xd1], [0x04,0xd9], [0x03,0xe0] ]
cube_voltage = [[0x07,0x45],]

# Master Power Iteration - cycles from 3.2V down to 2.0V and then back up
# [ 3.2V, 2.9V, 2.6V, 2.3, 2.0V ]

master_highbyte = [0x0f, 0x0e, 0x0c, 0x0b, 0x09 ]
master_lowbyte = [0x83, 0x0f, 0x9b, 0x26, 0xb2 ]

def setUsbPower(devMgr):
    
    success = True

    return success

def setSimulatedBatteryVoltage(devMgr):

    usb = devMgr.testjig();
    
    
    for value in cube_voltage:
        
        print value[0] , value[1]
        
        usb.txPacket([SimulatedBatteryVoltage, value[1] , value[0]])
        # Ideally it would return a unique byte for each iteration
        resp = usb.rxPacket()
        print resp.opcode
        from time import sleep
        sleep(5);
    
    
    if resp.opcode != SimulatedBatteryVoltage:
        success = False
    else:
        success = True

    return success
