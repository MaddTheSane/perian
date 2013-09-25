--
--  AppDelegate.applescript
--  testAS
--
--  Created by C.W. Betts on 9/25/13.
--  Copyright (c) 2013 C.W. Betts. All rights reserved.
--

script AppDelegate
	property parent : class "NSObject"
	
	on applicationWillFinishLaunching_(aNotification)
		-- Insert code here to initialize your application before any files are opened 
	end applicationWillFinishLaunching_
	
	on applicationDidFinishLaunching_(aNotification)
		-- Insert code here to initialize your application before any files are opened
	end applicationDidFinishLaunching_
	
	
	on applicationShouldTerminate_(sender)
		-- Insert code here to do any housekeeping before your application quits 
		return current application's NSTerminateNow
	end applicationShouldTerminate_
	
	on application_openFiles_(appl, theFiles)
		tell application "QuickTime Player"
			--For loop here, with calling arrays
			repeat with theFile in theFiles
				set theFilePox to theFile's fileSystemRepresentation()
				set m_path to POSIX file theFilePox
				open m_path
			end repeat
			
			activate
		end tell
		
		return yes
	end application_openFiles_
	
end script