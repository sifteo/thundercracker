var fs = require('fs');
var net = require('net');
var Connection = require('../').net.Connection;
var Packet = require('../').net.Packet;

var argv = require('optimist')
  .usage('Usage: $0')
  .demand([1])
  .argv;


console.log('Sifteo Loading: ' + argv._[0]);


// TODO: Run connection over serial port when master cube is ready.
// var serialPort = new SerialPort("/dev/tty.usbmodem411", { baudrate: 115200 });

var client = net.connect(8124, function() {
  sendSystemInfo();
  sendFile(argv._[0]);
});

var connection = new Connection(client);


function sendSystemInfo(cb) {
  var packet = Packet.createSystemInfoPacket();
  connection.send(packet);
  cb && cb();
}

function sendStartApp(info, cb) {
  var packet = Packet.createStartAppPacket(info);
  connection.send(packet);
  cb && cb();
}

function sendEndApp(cb) {
  var packet = Packet.createEndAppPacket();
  connection.send(packet);
  cb && cb();
}

function sendData(data, cb) {
  // TODO: Chunk it according to max size;
  
  var packet = Packet.createDataPacket(data);
  connection.send(packet);
  cb && cb();
}



function sendFile(path) {
  fs.stat(path, function(err, stats) {
    console.log('size: ' + stats.size);
      
    sendStartApp({ size: stats.size }, function(err) {
      if (err) { throw err; }
      
      var stream = fs.createReadStream(path);
      stream.on('data', function(chunk) {
        sendData(chunk);
        //client.write(chunk);
      })
      stream.on('close', function() {
        //console.log('done');
        sendEndApp();
      });
    });
  });
}

client.on('data', function(data) {
  console.log('client data :' + data.toString());
});

client.on('end', function() {
  console.log('client disconnected');
});
