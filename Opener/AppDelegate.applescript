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
		tell me
			set origDelimiters to AppleScript's text item delimiters
			set OSversion to (system version of (system info) as string)
			set AppleScript's text item delimiters to "."
			set osver to system version of (system info)
			set text item delimiters to "."
			if ((second text item of OSversion) as number < 9) then
				set AppleScript's text item delimiters to origDelimiters
				return true
			else
				set AppleScript's text item delimiters to origDelimiters
				return false
			end if
		end tell
	end isMavericksOrLater
	
	on applicationWillFinishLaunching:aNotification
		-- Insert code here to initialize your application before any files are opened 
	end applicationWillFinishLaunching:
	
	on applicationDidFinishLaunching:aNotification
		-- Insert code here to initialize your application before any files are opened
		tell current application's NSTimer to set theTimer to scheduledTimerWithTimeInterval_target_selector_userInfo_repeats_(10, me, "timerFired:", missing value, false)
	end applicationDidFinishLaunching:
	
	on timerFired:timer
		quit
	end timerFired:
	
	on applicationShouldTerminate:sender
		-- Insert code here to do any housekeeping before your application quits 
		return current application's NSTerminateNow
	end applicationShouldTerminate:
	
	on |application|:appl openFiles:theFiles
		
		set mavericks to isMavericksOrLater()
		if mavericks is true then
			try
				tell me
					set qtName to name of application file id "com.apple.quicktimeplayer"
					tell application id "com.apple.quicktimeplayer"
						repeat with theFile in theFiles's allObjects()
							set theFilePox to (theFile's fileSystemRepresentation())
							open theFilePox as POSIX file
						end repeat
						
						activate
						
					end tell
				end tell
			on error err_msg number err_num
				(* TODO: check for QuickTime 7, and point the user to the download location if they don't *)
				set qt7DLdialog to Â
					(display dialog "Unable to find QuickTime 7" buttons Â
						{"Download QT7", "Quit"} Â
							with icon stop Â
						default button 1 Â
						Â
						Â
						)
				
				set button to button returned of DIALOG_1
				
				return no
			end try
			
		else
			
			tell application "QuickTime Player"
				repeat with theFile in theFiles's allObjects()
					set theFilePox to (theFile's fileSystemRepresentation())
					open theFilePox as POSIX file
				end repeat
				
				activate
			end tell
		end if
		quit
		
		return yes
	end |application|:openFiles:
	
end script
