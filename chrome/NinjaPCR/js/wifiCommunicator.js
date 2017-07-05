var Serial = function () {
	this.firmwareVersion = "1.0.0"; //TODO implement version check (Firmware + app)
};
var DeviceResponse = {
		onDeviceFound : null,
		onReceiveCommandResponse : null,
		onReceiveStatus : null
};


/* Handle connection check response */
DeviceResponse.connect = function (obj) {
	console.log("DeviceResponse.connect:" + obj);
	if (obj) {
		// Connected
		$("#ip_status").text("Connected");
		// TODO OK
		DeviceResponse.onDeviceFound("DEVICE");
	}
};
/* Handle command result */
DeviceResponse.command = function (obj) {
	console.log("DeviceResponse.command: " + obj);
};
/* Handle status response */
DeviceResponse.status = function (obj) {
	console.log("DeviceResponse.status:" + obj);
	if (DeviceResponse.onReceiveStatus) {
		DeviceResponse.onReceiveStatus(obj);
	}
};
// Find ports
Serial.prototype.scan = function (callback) {
	// callback(port)
	DeviceResponse.onDeviceFound = callback;
	var ipLabel = document.createTextNode("http://")
	
	var ipText = document.createElement("input");
	ipText.placeholder = "XXX.XXX.XXX.XXX";
	ipText.id = "device_ip";
	ipText.value = "192.168.2.153";
	ipText.size = "24";
	
	var ipButton = document.createElement("input");
	ipButton.value = "Connect";
	ipButton.type = "button";
	
	var scope = this;
	ipButton.addEventListener("click", function(e) {
		console.log("Check IP: " + ipText.value);
		scope.setDeviceIP(ipText.value);
		scope.connect();
	}, true);
	
	var ipStatusLabel = document.createElement("span");
	ipStatusLabel.id = "ip_status";

	document.getElementById("ipInputContainer").appendChild(ipLabel);
	document.getElementById("ipInputContainer").appendChild(ipText);
	document.getElementById("ipInputContainer").appendChild(ipButton);
	document.getElementById("ipInputContainer").appendChild(ipStatusLabel);
};
Serial.prototype.sendRequestToDevice = function (path, param) {
	var URL = "http://" + this.ip + path;
	if (param) {
		if (param.charAt(0)!="?") {
			URL += "?";
		}
		URL += param;
	}
	console.log("sendRequestToDevice URL=" + URL);
	var scriptTag = document.createElement("script");
	scriptTag.type = "text/javascript";
	scriptTag.src = URL;
	document.body.appendChild(scriptTag);
}
Serial.prototype.setDeviceIP = function (ip) {
	this.ip = ip;
}
Serial.prototype.connect = function (ip) {
	this.sendRequestToDevice("/connect");
	$("#ip_status").text("Connecting...");
}

Serial.prototype.scanOngoingExperiment = function () {
	console.log("TODO scanOngoingExperiment");
	// TODO getPorts -> Open -> (callback) -> getList -> sendRequest -> read -> ...
	callback();
};


Serial.prototype.sendStartCommand = function (commandBody) {
	console.log("TODO sendStartCommand");
	this.sendRequestToDevice("/command", commandBody);
};

// * Request Status and Wait for Response
Serial.prototype.requestStatus = function (callback) {
	this.sendRequestToDevice("/status");
	DeviceResponse.onReceiveStatus = callback;
};

// Send "Stop" Command and Wait for Response
Serial.prototype.sendStopCommand = function (command, callback) {
	// self.startListeningStatus(self.port, connectionId, callback);
	
};

var chromeSerial = new Serial(); // TODO rename

(function () {
	console.log("Testing local storage");
	console.log("localStorage=" + window.localStorage);
	console.log("item=" + window.localStorage.getItem("hoge"));
	window.localStorage.setItem("hoge","puni");
})();