--
--  AppDelegate.applescript
--  PerianOpener
--
--  Created by C.W. Betts on 9/25/13.
--  Copyright (c) 2013 Perian Team. All rights reserved.
--

script AppDelegate
	property parent : class "NSObject"
	
	on applicationWillFinishLaunching_(aNotification)
		-- Insert code here to initialize your application before any files are opened 
	end applicationWillFinishLaunching_
	
	on applicationDidFinishLaunching_(aNotification)
		-- Insert code here to initialize your application before any files are opened
		tell current application's NSTimer to set theTimer to scheduledTimerWithTimeInterval_target_selector_userInfo_repeats_(10, me, "timerFired:", missing value, false)
	end applicationDidFinishLaunching_
	
	on timerFired_(timer)
		quit
	end timerFired_
	
	on applicationShouldTerminate_(sender)
		-- Insert code here to do any housekeeping before your application quits 
		return current application's NSTerminateNow
	end applicationShouldTerminate_
	
	on application_openFiles_(appl, theFiles)
		tell application "QuickTime Player"
			repeat with theFile in theFiles's allObjects()
				set theFilePox to (theFile's fileSystemRepresentation())
				open theFilePox as POSIX file
			end repeat
			
			activate
		end tell
		quit
		
		return yes
	end application_openFiles_
	
end script
