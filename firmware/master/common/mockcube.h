#ifndef _MOCK_CUBE_H
#define _MOCK_CUBE_H

#include "radio.h"
#include "cubeslots.h"
#include "systime.h"


struct PacketTransmission;

class MockCubeSlot {
 public:
     static CubeSlot &getInstance(_SYSCubeID id) {
         ASSERT(id < _SYS_NUM_CUBE_SLOTS);
         return CubeSlots::instances[id];
     }
     
     _SYSCubeID id() const {
         _SYSCubeID i = this - &CubeSlots::instances[0];
         ASSERT(i < _SYS_NUM_CUBE_SLOTS);
         STATIC_ASSERT(arraysize(CubeSlots::instances) == _SYS_NUM_CUBE_SLOTS);
         return i;
     }
     
     _SYSCubeIDVector bit() const {
         STATIC_ASSERT(_SYS_NUM_CUBE_SLOTS <= 32);
         return Sifteo::Intrinsic::LZ(id());
     }
     
     void setVideoBuffer(_SYSVideoBuffer * /*v*/) {
     }
     
     void loadAssets(_SYSAssetGroup * /*a*/) {
     }
     
     void getAccelState(struct _SYSAccelState * /*state*/) {
         //*state = accelState;
     }
     
     void getRawNeighbors(uint8_t buf[4]) {
     }
     
     uint16_t getRawBatteryV() const {
         return 0;
         //return rawBatteryV;
     }
     
     const _SYSCubeHWID & getHWID() const {
         //return 0;
         return hwid;
     }
     
     bool enabled() const {
         return !!(bit() & CubeSlots::vecEnabled);
         //return enabledFlag;
     }
     
     bool radioProduce(PacketTransmission & /*tx*/) {
         return true;
     }
     
     void radioAcknowledge(const PacketBuffer & /*packet*/) {
         radioAcknowledgeCallCount++;
     }
     
     void radioTimeout() {
         
     }
     
     void waitForPaint() {
         
     }
     void waitForFinish() {
         
     }
     void triggerPaint(SysTime::Ticks /*timestamp*/) {
         
     }
     
     
  public:
      //bool enabledFlag;
      uint32_t radioAcknowledgeCallCount;
     
  private:
      _SYSCubeHWID hwid;
};

#endif _MOCK_CUBE_H