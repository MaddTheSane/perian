--
--  AppDelegate.applescript
--  PerianOpener
--
--  Created by C.W. Betts on 9/25/13.
--  Copyright (c) 2013 Perian Team. All rights reserved.
--

script AppDelegate
	property parent : class "NSObject"
	
	on isMavericksOrLater()
		set osver to system version of (system info)
		(* skip the first three characters: We don't run on pre-OS X systems,
		 and we won't reach Mac OS version 100 any time soon. *)
		set osList to characters 4 thru 6 of osver
		(* If we get a decimal as the last item, the second version value is greater than 10.
		 We're looking for Mac versions greater than nine. *)
		if (osList's item 3) is equal to "." then
			return yes
			else
			set preTen to osList as text as real
			if preTen is greater than or equal to 9.0 then
				return yes
				else
				return no
			end if
		end if
	end isMavericksOrLater
	
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
		
		set mavericks to isMavericksOrLater()
		if mavericks is true
		(* TODO: check for QuickTime 7, and point the user to the download location if they don't *)
		tell application id "com.apple.quicktimeplayer"
			repeat with theFile in theFiles's allObjects()
				set theFilePox to (theFile's fileSystemRepresentation())
				open theFilePox as POSIX file
			end repeat
			
			activate
		end tell

		else
		
		tell application "QuickTime Player"
			repeat with theFile in theFiles's allObjects()
				set theFilePox to (theFile's fileSystemRepresentation())
				open theFilePox as POSIX file
			end repeat
			
			activate
		end tell
		end
		quit
		
		return yes
	end application_openFiles_
	
end script
