#ifndef _MOCK_CUBECODEC_H
#define _MOCK_CUBECODEC_H

class MockCubeCodec {
 public:
     void encodeVRAM(PacketBuffer & /*buf*/, _SYSVideoBuffer * /*vb*/) { 
     }
     
     bool flashReset(PacketBuffer & /* buf */) { 
         return true;
     }
     
     bool flashSend(PacketBuffer & /* buf */ , _SYSAssetGroup * /* group */, _SYSAssetGroupCube * /* ac */, bool & /* done */) {
         return true;
     }
     
     void endPacket(PacketBuffer & /* buf */) {
     }
     
     void timeSync(PacketBuffer & /* buf */, uint16_t /* rawTimer */) {
     }
     
     void flashAckBytes(uint8_t /* count */) {
     }
};

#endif _MOCK_CUBECODEC_H
