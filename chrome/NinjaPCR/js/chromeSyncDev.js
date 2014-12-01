/**
	Dev code for chrome sync implementation
*/

function execSync () 
{
	console.log("execSync");
}
function execUpdateForSync ()
{
	console.log("execUpdateForSync");
}

function initChromeSyncDev () 
{
	console.log("initChromeSyncDev start.");
	var syncButton = document.getElementById("devSyncButton");
	var updateForSyncButton = document.getElementById("devUpdateForSyncButton");
	console.log(syncButton);
	console.log(updateForSyncButton);
	syncButton.addEventListener("click", execSync);
	updateForSyncButton.addEventListener("click", execUpdateForSync);
}

initChromeSyncDev ();
