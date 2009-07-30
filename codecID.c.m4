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
#include "avcodec.h"
#include "CodecIDs.h"

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
