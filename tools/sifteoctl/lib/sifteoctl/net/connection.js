var util = require('util')
  , EventEmitter = require('events').EventEmitter;


function Connection(stream) {
  var self = this;
  EventEmitter.call(this);
  
  this._stream = stream;
}

util.inherits(Connection, EventEmitter);

Connection.prototype.send = function(packet) {
  var data = packet.toData();
  this._stream.write(data);
}


module.exports = Connection;
