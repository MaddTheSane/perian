/*
 *  Codecprintf.c
 *  Perian
 *
 *  Created by Augie Fackler on 7/16/06.
 *  Copyright 2006 Perian Project. All rights reserved.
 *
 */

#include "Codecprintf.h"
#include <stdio.h>
#include <stdarg.h>

#ifdef DEBUG_BUILD
#define CODEC_HEADER			"Perian Codec: "
int Codecprintf(FILE *fileLog, const char *format, ...)
{
	int ret;
	va_list va;
	va_start(va, format);
#ifdef FILELOG
	if(fileLog)
	{
		fprintf(glob->fileLog, CODEC_HEADER);
		ret = vfprintf(fileLog, format, va);
		fflush(glob->fileLog);
	}
	else
	{
#endif
		printf(CODEC_HEADER);
		
		return vprintf(format, va);
#ifdef FILELOG
	}
#endif
	va_end(va);
	return ret;
}

void FourCCprintf (char *string, unsigned long a)
{
    if (a < 64)
    {
        printf("%s: %ld\n", string, a);
    }
    else
    {
        printf("%s: %c%c%c%c\n", string, (unsigned char)((a >> 24) & 0xff), 
			   (unsigned char)((a >> 16) & 0xff), 
			   (unsigned char)((a >> 8) & 0xff), 
			   (unsigned char)(a & 0xff));
    }
}
#endif