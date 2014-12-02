/**
	Dev code for chrome sync implementation
*/
var Sync = {};
Sync.fileSystem = null;
Sync.execSync = function () 
{
	console.log("execSync");
}
Sync.execUpdateForSync = function ()
{
	console.log("execUpdateForSync");
}

Sync.initChromeSyncDev = function () 
{
	console.log("initChromeSyncDev start.");
	var syncButton = document.getElementById("devSyncButton");
	var updateForSyncButton = document.getElementById("devUpdateForSyncButton");
	
	syncButton.addEventListener("click", Sync.execSync);
	updateForSyncButton.addEventListener("click", Sync.execUpdateForSync);
	
	//Get filesystem

	chrome.syncFileSystem.requestFileSystem(function (/*DOMFileSystem*/fileSystem) {
		// DOMFileSystem instance
		// http://www.w3.org/TR/file-system-api/
		console.log("fileSystem=" + fileSystem);
		debugObject(fileSystem, 3);
		Sync.fileSystem = fileSystem;
		// Check quota
		chrome.syncFileSystem.getUsageAndQuota(fileSystem, function (result) {
			console.log("quotaBytes="+result.quotaBytes);
			console.log("usageBytes="+result.usageBytes);
			if (result.usageBytes > result.quotaBytes*0.8) {
				console.log("[WARNING] Exceeded 80% of sync quota.");
			}
			
		});
		
	});
}

Sync.initChromeSyncDev ();
