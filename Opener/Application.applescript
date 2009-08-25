on idle
	quit
end idle

on open of theFiles
	tell application "QuickTime Player"
		repeat with theFile in theFiles
			open theFile
		end repeat
		
		activate
	end tell
	
	quit
end open
