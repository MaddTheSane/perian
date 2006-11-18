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
#include "log.h"

#ifdef DEBUG_BUILD
#define CODEC_HEADER			"Perian Codec: "

static int Codecvprintf(FILE *fileLog, const char *format, va_list va, int print_header)
{
	int ret;
	
#ifdef FILELOG
	if(fileLog)
	{
		if(print_header)
			fprintf(glob->fileLog, CODEC_HEADER);
		ret = vfprintf(fileLog, format, va);
		fflush(glob->fileLog);
	}
	else
	{
#endif
		if(print_header)
			printf(CODEC_HEADER);
		
		ret = vprintf(format, va);
#ifdef FILELOG
	}
#endif
	
}

int Codecprintf(FILE *fileLog, const char *format, ...)
{
	int ret;
	va_list va;
	va_start(va, format);
	ret = Codecvprintf(fileLog, format, va, 1);
	va_end(va);
	return ret;
}

void FourCCprintf (char *string, unsigned long a)
{
    if (a < 64)
    {
        Codecprintf(NULL, "%s%ld\n", string, a);
    }
    else
    {
        Codecprintf(NULL, "%s%c%c%c%c\n", string,
			   (unsigned char)((a >> 24) & 0xff), 
			   (unsigned char)((a >> 16) & 0xff), 
			   (unsigned char)((a >> 8) & 0xff), 
			   (unsigned char)(a & 0xff));
    }
}
#else
#define Codecvprintf(file, fmt, va, print_header) /**/
#endif

void FFMpegCodecprintf(void* ptr, int level, const char* fmt, va_list vl)
{
    static int print_prefix=1;
	int print_header = 1;
    AVClass* avc= ptr ? *(AVClass**)ptr : NULL;
    if(level>av_log_level)
        return;

    if(print_prefix && avc) {
		Codecprintf(NULL, "[%s @ %p]", avc->item_name(ptr), avc);
		print_header = 0;
    }
	
    print_prefix= strstr(fmt, "\n") != NULL;
	
	Codecvprintf(NULL, fmt, vl, print_header);
}