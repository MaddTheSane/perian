include(`defines.m4')

define(<!doCase!>, <!ifelse($1, lastCase, , <!ifdef(<!lastCase!>, <!			codecID = lastCase;
			break;
		
!>)!>)<!		case $2:!>
define(<!lastCase!>, <!$1!>)!>)dnl
define(<!printCaseStatement!>, <!ifelse(len($1), 0, <!!>, <!foreach(<!X!>, <!doCase($1, X)!>, shift($@))!>)!>)dnl
define(<!Codec!>, <!printCaseStatement($2, shift(shift(shift(shift($@)))))!>)dnl
dnl
define(<!EntryPoint!>, <!!>)dnl
define(<!ResourceOnly!>, <!!>)dnl
#include <QuickTime/QuickTime.h>
#include <libavcodec/avcodec.h>

#include "CodecIDs.h"
#include "PerianResourceIDs.h"

int getCodecID(OSType componentType)
{
	enum CodecID codecID = CODEC_ID_NONE;
	switch(componentType)
	{
include(<!codecList.m4!>)
			codecID = lastCase;
			break;
		default:
			break;
	}
	return codecID;
}

undefine(<!lastCase!>)dnl
define(<!doCase!>, <!ifelse($1, lastCase, , <!ifdef(<!lastCase!>, <!			err = GetComponentResource((Component)self, codecInfoResourceType, lastCase, (Handle *)&tempCodecInfo);
			break;
		
!>)!>)<!		case $2:!>
define(<!lastCase!>, <!$1!>)!>)dnl
define(<!Codec!>, <!printCaseStatement($1, shift(shift(shift(shift($@)))))!>)dnl

pascal ComponentResult getPerianCodecInfo(ComponentInstance self, OSType componentType, void *info)
{
    OSErr err = noErr;
	
    if (info == NULL) 
    {
        err = paramErr;
    }
    else 
    {
        CodecInfo **tempCodecInfo;
		
        switch (componentType)
        {
include(<!codecList.m4!>)
                err = GetComponentResource((Component)self, codecInfoResourceType, lastCase, (Handle *)&tempCodecInfo);
                break;


            default:	// should never happen but we have to handle the case
                err = GetComponentResource((Component)self, codecInfoResourceType, kDivX4CodecInfoResID, (Handle *)&tempCodecInfo);
				
        }
        
        if (err == noErr) 
        {
            *((CodecInfo *)info) = **tempCodecInfo;
            
            DisposeHandle((Handle)tempCodecInfo);
        }
    }
	
    return err;
}
