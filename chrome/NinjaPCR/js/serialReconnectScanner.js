var SerialReconnectScanner = function (ports) {
	this.ports = ports;
	this.currentPortIndex = 0;
	this.foundPort = null;
};

SerialReconnectScanner.prototype.findWorkingPort = function (callback) {
	if (!this.ports || this.ports.length==0) {
		//No port found. return null.
		callback(null);
		return;
	}
	var self = this;
	// Scan port[currentPortIndex]
	if (this.currentPortIndex==this.ports.length) {
		callback(null, 0);
	} else {
		// Scan next port
		this._scan(function(){
			self.currentPortIndex++;
			if (self.foundPort) {
				Log.d("Finish scanning.");
				callback(self.foundPort, self.connectionId);
			} else {
				self.findWorkingPort(callback);
			}
		});
	}
};

//Private
SerialReconnectScanner.prototype._scan = function(callback) {
	var port = this.ports[this.currentPortIndex];
	var self = this;
	var options = {
			bitrate:BAUD_RATE
	};
	Log.v("Opening port " + port);
	chrome.serial.open(port, options, function (openInfo) {
		var connectionId = openInfo.connectionId;
		if (connectionId<0) {
			Log.e("Connection error. ID=" + connectionId);
			callback(null);
		} else {
			self.scanStartTimestamp = new Date().getTime();
			self.readMessage = "";
			//var data = getFullCommand(commandBody, SEND_CMD);
			var data = str2bin("FFFFFF");
			chrome.serial.write(connectionId, data, function (sendInfo) {
				console.log("Message sent to port " + port);
				self._read(connectionId, callback);
			});
		}
	});
};

function str2bin(str) {
	var length = str.length;
	var buff = new ArrayBuffer(length);
	var arr = new Uint8Array(buff);
	
	for (var i=0; i<commandBody.length; i++) {
		arr[i] = str.charCodeAt(i);
	}
	return buff;
}

SerialReconnectScanner.DURATION_MSEC = 3000;
SerialReconnectScanner.BYTES_TO_READ = 64;

SerialReconnectScanner.INITIAL_MESSAGE = "";
for (var i=0; i<512; i++) {
	SerialReconnectScanner.INITIAL_MESSAGE += "a";
}
// Called from _scan"
SerialReconnectScanner.prototype._read = function (connectionId, callback) {
	var port = this.ports[this.currentPortIndex];
	var self = this;
	chrome.serial.read(connectionId, SerialReconnectScanner.BYTES_TO_READ, function (readInfo) {
		if (readInfo.bytesRead>0) {
			var message = String.fromCharCode.apply(null, new Uint8Array(readInfo.data));
			self.readMessage += message;
		}
		
		if (new Date().getTime()-self.scanStartTimestamp<SerialReconnectScanner.DURATION_MSEC) {
			// Continue reading until time limit
			self._read(connectionId, callback);
		} else {
			// Finish reading and check message
			Log.d("Message=" + self.readMessage);
			if (self.readMessage.match(MESSAGE_FROM_DEVICE)) {
				self.foundPort = port;
				self.firmwareVersion = version;
				self.connectionId = connectionId;
				var data = getArrayBufferForString(SerialReconnectScanner.INITIAL_MESSAGE);
				chrome.serial.write(connectionId, data, function (sendInfo){
					chrome.serial.read(connectionId, 1024, function(){
						callback();
					});
				});
			} else {
				chrome.serial.close(connectionId, function () {
					Log.d("Device was not found on port " + port);
					callback();
				});
			}
		}
	});
};
