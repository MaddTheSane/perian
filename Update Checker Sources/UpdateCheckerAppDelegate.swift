//
//  UpdateCheckerAppDelegate.swift
//  Perian
//
//  Created by C.W. Betts on 1/3/15.
//  Copyright (c) 2015 Perian. All rights reserved.
//

import Cocoa

private let NEXT_RUN_KEY = "NextRunDate"
private let MANUAL_RUN_KEY = "ManualUpdateCheck"
private let UPDATE_URL_KEY = "UpdateFeedURL"
private let SKIPPED_VERSION_KEY = "SkippedVersion"
private let UPDATE_STATUS_NOTIFICATION = "org.perian.UpdateCheckStatus"
private let TIME_INTERVAL_TIL_NEXT_RUN = NSTimeInterval(7*24*60*60)


class UpdateCheckerAppDelegate: NSObject, NSURLDownloadDelegate, SUAppcastDelegate, SUUpdateAlertDelegate {
	private var updateAlert: SUUpdateAlert? = nil
	private var latest: SUAppcastItem? = nil
	private var statusController: SUStatusController? = nil
	private var appcast = SUAppcast()
	private var downloader: NSURLDownload? = nil
	private var downloadPath = ""
	private var lastRunDate: NSDate! = nil
	private var manualRun = false

	func applicationDidFinishLaunching(aNotification: NSNotification) {
		let defaults = NSUserDefaults.standardUserDefaults()
		if let aVal = defaults.objectForKey(NEXT_RUN_KEY) as? NSDate {
			lastRunDate = aVal
		}
		
		if !lastRunDate.isEqualToDate(NSDate.distantFuture() as NSDate) {
			defaults.setObject(NSDate(timeIntervalSinceNow: TIME_INTERVAL_TIL_NEXT_RUN), forKey: NEXT_RUN_KEY)
		}
		
		manualRun = defaults.boolForKey(MANUAL_RUN_KEY)
		defaults.removeObjectForKey(MANUAL_RUN_KEY)
		defaults.synchronize()
		doUpdateCheck()
	}
	
	func doUpdateCheck() {
		if let updateUrlString = SUInfoValueForKey(UPDATE_URL_KEY) as? String {
			if(manualRun) {
				NSDistributedNotificationCenter.defaultCenter().postNotificationName(UPDATE_STATUS_NOTIFICATION, object: "Starting")
			}
			
			self.appcast = SUAppcast();
			appcast.delegate = self
			appcast.fetchAppcastFromURL(NSURL(string: updateUrlString))
		} else {
			fatalError("No feed URL is specified in the Info.plist!")
		}
	}
	
	func appcastDidFinishLoading(anappcast: SUAppcast!) {
		self.latest = (anappcast.items.first as SUAppcastItem)
		
		if let latestVer = latest?.versionString {
		// OS version (Apple recommends using SystemVersion.plist instead of Gestalt() here, don't ask me why).
		// This code *should* use NSSearchPathForDirectoriesInDomains(NSCoreServiceDirectory, NSSystemDomainMask, YES)
		// but that returns /Library/CoreServices for some reason
		let versionPlistPath = "/System/Library/CoreServices/SystemVersion.plist";
		let currentSystemVersion = NSDictionary(contentsOfFile: versionPlistPath)!["ProductVersion"] as String
		
		var updateAvailable = SUStandardVersionComparison(latest!.minimumSystemVersion, currentSystemVersion) != .OrderedDescending;
		let panePath = NSBundle.mainBundle().bundlePath.stringByDeletingLastPathComponent.stringByDeletingLastPathComponent.stringByDeletingLastPathComponent
		updateAvailable = updateAvailable && ((SUStandardVersionComparison(latest!.versionString, NSBundle(path:panePath)?.objectForInfoDictionaryKey("CFBundleVersion") as String)) == .OrderedAscending)
		
		if panePath.lastPathComponent != "Perian.prefPane" {
			NSLog("The update checker needs to be run from inside the preference pane, quitting...");
			updateAvailable = false;
		}
		
		var skippedVersion = NSUserDefaults.standardUserDefaults().objectForKey(SKIPPED_VERSION_KEY) as? String
		
		if (updateAvailable && (skippedVersion != nil ||
			(skippedVersion != nil && skippedVersion != latest!.versionString ))) {
				if(manualRun) {
					NSDistributedNotificationCenter.defaultCenter().postNotificationName(UPDATE_STATUS_NOTIFICATION, object: "YesUpdates")
				}
				showUpdatePanelForItem(latest!)
		} else {
			if(manualRun) {
				NSDistributedNotificationCenter.defaultCenter().postNotificationName(UPDATE_STATUS_NOTIFICATION, object: "NoUpdates")
			}
			NSApplication.sharedApplication().terminate(self)
		}
		
		//RELEASEOBJ(appcast);
		//self.appcast = nil;
		} else {
			updateFailed()
			fatalError("Can't extract a version string from the appcast feed. The filenames should look like YourApp_1.5.tgz, where 1.5 is the version number.")
		}
	}
	
	func appcast(aappcast: SUAppcast!, failedToLoadWithError error: NSError!) {
		appcastDidFailToLoad(aappcast)
	}
	
	func appcastDidFailToLoad(appcast: SUAppcast!) {
		updateFailed()
		if manualRun {
			NSDistributedNotificationCenter.defaultCenter().postNotificationName(UPDATE_STATUS_NOTIFICATION, object: "Error")
		}
		
		NSApplication.sharedApplication().terminate(self)
	}
	
	func showUpdatePanelForItem(updateItem: SUAppcastItem) {
		let anUpdateAlert = SUUpdateAlert(appcastItem: updateItem)
		updateAlert = anUpdateAlert;
		anUpdateAlert.delegate = self;
		anUpdateAlert.showWindow(self)
	}
	
	func updateAlert(updateAlert: SUUpdateAlert!, finishedWithChoice choice: SUUpdateAlertChoice) {
		if (choice == .InstallUpdateChoice) {
			beginDownload()
		} else {
			if (choice == .SkipThisVersionChoice) {
				NSUserDefaults.standardUserDefaults().setObject(latest!.versionString, forKey: SKIPPED_VERSION_KEY)
			}
			NSApplication.sharedApplication().terminate(self)
		}
	}
	
	@objc(showUpdateErrorAlertWithInfo:) func showUpdateErrorAlert(#info: String) {
		let anAlert = NSAlert()
		anAlert.messageText = SULocalizedString("Update Error!", nil)
		anAlert.informativeText = info
		anAlert.addButtonWithTitle(SULocalizedString("Cancel", nil))
		
		anAlert.runModal()
	}
	
	func beginDownload() {
		let aStat = SUStatusController()
		statusController = aStat
		aStat.beginActionWithTitle(SULocalizedString("Downloading update...", nil), maxProgressValue: 0, statusText: nil)
		aStat.setButtonTitle(SULocalizedString("Cancel", nil), target: self, action: "cancelDownload:", isDefault: false)
		aStat.showWindow(self)
		
		downloader = NSURLDownload(request: NSURLRequest(URL: latest!.fileURL!), delegate: self)
	}

	@IBAction func cancelDownload(sender: AnyObject?) {
		downloader?.cancel()
		statusController?.close()
		NSApplication.sharedApplication().terminate(self)
	}
	
	// MARK: NSURLDownload delegate methods
	func download(download: NSURLDownload, didReceiveResponse response: NSURLResponse) {
		statusController?.maxProgressValue = Double(response.expectedContentLength)
	}
	
	func download(download: NSURLDownload, decideDestinationWithSuggestedFilename filename: String) {
		var name = filename
		// If name ends in .txt, the server probably has a stupid MIME configuration. We'll give
		// the developer the benefit of the doubt and chop that off.
		if name.pathExtension == "txt" {
			name = name.stringByDeletingPathExtension
		}
		
		// We create a temporary directory in /tmp and stick the file there.
		let tempURL = NSFileManager.defaultManager().URLForDirectory(.ItemReplacementDirectory, inDomain: .UserDomainMask, appropriateForURL: NSURL(fileURLWithPath: "/tmp", isDirectory: true)!, create: true, error: nil)?.URLByAppendingPathComponent(NSProcessInfo.processInfo().globallyUniqueString)
		let tempDir = tempURL!.path!
		let success = NSFileManager.defaultManager().createDirectoryAtURL(tempURL!, withIntermediateDirectories: true, attributes: nil, error: nil)
		
		if (!success) {
			download.cancel()
			fatalError("Couldn't create temporary directory at \(tempDir)")
			
			//[NSException raise:@"SUFailTmpWrite" format:@"Couldn't create temporary directory in /tmp"];
			//[download cancel];
		}
		
		self.downloadPath = tempDir.stringByAppendingPathComponent(name)
		download.setDestination(downloadPath, allowOverwrite: true)
	}
	
	func download(download: NSURLDownload, didReceiveDataOfLength length: Int) {
		statusController?.progressValue += Double(length)
		statusController?.statusText = NSString(format: SULocalizedString("%.0lfk of %.0lfk", nil), statusController!.progressValue / 1024.0, statusController!.maxProgressValue / 1024.0)
	}
	
	func download(download: NSURLDownload, didFailWithError error: NSError) {
		updateFailed()
		NSLog("Download error: %@", error.localizedDescription)
		showUpdateErrorAlert(info: SULocalizedString("An error occurred while trying to download the newest version of Perian. Please try again later.", nil))
		NSApplication.sharedApplication().terminate(self)
	}
	
	// MARK: Stolen from sprakle
	func extractDMG(archivePath: String) -> Bool {
		// First, we internet-enable the volume.
		var hdiTask = NSTask.launchedTaskWithLaunchPath("/usr/bin/env", arguments: ["hdiutil", "internet-enable", "-quiet", archivePath])
		hdiTask.waitUntilExit()
		if hdiTask.terminationStatus != 0 {
			return false
		}
		
		// Now, open the volume; it'll extract into its own directory.
		hdiTask = NSTask.launchedTaskWithLaunchPath("/usr/bin/env", arguments: ["hdiutil", "attach", "-idme", "-noidmereveal", "-noidmetrash", "-noverify", "-nobrowse", "-noautoopen", "-quiet", archivePath])
		hdiTask.waitUntilExit()
		if hdiTask.terminationStatus != 0 {
			return false
		}

		return true
	}
	
	func downloadDidFinish(download: NSURLDownload) {
		downloader = nil
		
		//Indeterminate progress bar
		statusController?.maxProgressValue = 0
		statusController?.statusText = SULocalizedString("Extracting...", nil)
		
		if !extractDMG(downloadPath) {
			updateFailed()
			showUpdateErrorAlert(info: NSLocalizedString("Could not Extract Downloaded File", comment: ""))
		}
		
		var prefpanelocation = ""
		var moveErr: NSError? = nil
		if let dirEnum = NSFileManager.defaultManager().enumeratorAtPath(downloadPath.stringByDeletingLastPathComponent) {
			while let file = dirEnum.nextObject() as? String {
				if dirEnum.fileAttributes![NSFileTypeSymbolicLink] as Bool {
					dirEnum.skipDescendants()
				}
				if file.pathExtension == "prefPane" {
					var containingLocation = downloadPath.stringByDeletingLastPathComponent
					var oldLocation = containingLocation.stringByAppendingPathComponent(file)
					prefpanelocation = containingLocation.stringByDeletingLastPathComponent.stringByAppendingPathComponent(file.lastPathComponent)
					NSFileManager.defaultManager().moveItemAtPath(oldLocation, toPath: prefpanelocation, error: &moveErr)
				}
			}
		}
		let buf = "open \"$PREFPANE_LOCATION\"; rm -rf \"$TEMP_FOLDER\""
		
		if moveErr != nil {
			updateFailed()
			showUpdateErrorAlert(info: NSLocalizedString("Could not Create Extraction Script", comment: ""))
		}
		
		let quitSysPrefsScript = NSAppleScript(source: "tell application \"System Preferences\" to quit")
		quitSysPrefsScript?.executeAndReturnError(nil)
		
		//	const char * args[] = {"/bin/sh", "-c", buf, NULL};
		let args = ["/bin/sh", "-c", buf]
		//setenv("PREFPANE_LOCATION", prefpanelocation.fileSystemRepresentation(), 1);
		//setenv("TEMP_FOLDER", downloadPath.stringByDeletingLastPathComponent.fileSystemRepresentation(), 1);
		let moveTask = NSTask()
		moveTask.launchPath = "/bin/sh"
		moveTask.arguments = ["-c", buf]
		moveTask.environment = ["PREFPANE_LOCATION": prefpanelocation,
			"TEMP_FOLDER": downloadPath.stringByDeletingLastPathComponent]
		
		moveTask.launch()
		moveTask.waitUntilExit()
		
		if moveTask.terminationStatus != 0 {
			updateFailed()
			showUpdateErrorAlert(info: NSLocalizedString("Could not Run Update", comment: ""))
		}
		
		NSApplication.sharedApplication().terminate(self)
		//And, we are out of here!!!
	}
	
	let showsReleaseNotes = true

	func updateFailed() {
		if let alastRun = lastRunDate {
			NSUserDefaults.standardUserDefaults().setObject(lastRunDate, forKey: NEXT_RUN_KEY)
		} else {
			NSUserDefaults.standardUserDefaults().removeObjectForKey(NEXT_RUN_KEY)
		}
	}
	
	
}
