import Image
import ImageDraw
import os.path
import re
import sys

batteryResolution = 4
batteryBuddyXY = (8, 24)
batteryBuddySize = (24, 24)
batteryMasterXY = (8, 56)
batteryMasterSize = (48, 24)

def DestPath():
    return os.path.join(os.path.dirname(sys.argv[0]), '..', 'images')

if __name__ == '__main__':
    # Generate buddy-only battery icons
    iconBatteryPath = os.path.join(os.path.dirname(sys.argv[0]), 'icon-battery.png')

    for i in range(batteryResolution + 1):
        iconBattery = Image.open(iconBatteryPath)
        iconBatteryPixels = iconBattery.load()
        
        xInc = float(batteryBuddySize[0]) / float(batteryResolution)
        
        for x in range(int(xInc * i)):
            for y in range(batteryBuddySize[1]):
                iconBatteryPixels[batteryBuddyXY[0] + x, batteryBuddyXY[1] + y] = (0, 0, 0)

        iconBattery.save(os.path.join(DestPath(), 'icon-battery-%d.png' % i))

    # Generate buddy-with-mater battery icons
    iconBatteryMasterPath = os.path.join(os.path.dirname(sys.argv[0]), 'icon-battery-master.png')
    
    for i in range(batteryResolution + 1):
        for j in range(batteryResolution + 1):
            iconBatteryMaster = Image.open(iconBatteryMasterPath)
            iconBatteryMasterPixels = iconBatteryMaster.load()
            
            xIncBuddy = float(batteryBuddySize[0]) / float(batteryResolution)
            xIncMaster = float(batteryMasterSize[0]) / float(batteryResolution)
            
            for x in range(int(xIncBuddy * i)):
                for y in range(batteryBuddySize[1]):
                    iconBatteryMasterPixels[batteryBuddyXY[0] + x, batteryBuddyXY[1] + y] = (0, 0, 0)
            
            for x in range(int(xIncMaster * j)):
                for y in range(batteryMasterSize[1]):
                    iconBatteryMasterPixels[batteryMasterXY[0] + x, batteryMasterXY[1] + y] = (255, 0, 0)

            iconBatteryMaster.save(os.path.join(DestPath(), 'icon-battery-master-%d-%d.png' % (i, j)))