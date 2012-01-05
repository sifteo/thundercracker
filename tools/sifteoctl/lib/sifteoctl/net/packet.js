var binary = require('binary');

var SYSTEM_INFO_TYPE = 1;
var START_APP_TYPE = 2;
var END_APP_TYPE = 4;
var ASSET_DATA_TYPE = 3;

function Packet(type, data) {
  this._type = type;
  this._data = data;
}

Packet.prototype.toBuffer =
Packet.prototype.toData = function() {
  if (!this._data) {
    return binary.put()
      .word16be(this._type)
      .word16be(0)
      .buffer();
  }
  
  return binary.put()
    .word16be(this._type)
    .word16be(this._data.length)
    .put(this._data)
    .buffer();
}


Packet.createSystemInfoPacket = function() {
  return new Packet(ASSET_DATA_TYPE);
}

Packet.createStartAppPacket = function(info) {
  var data = binary.put()
    .word32be(info.size)
    .buffer();
  
  return new Packet(START_APP_TYPE, data);
}

Packet.createEndAppPacket = function() {
  return new Packet(END_APP_TYPE);
}

Packet.createDataPacket = function(data) {
  return new Packet(ASSET_DATA_TYPE, data);
}

module.exports = Packet;
