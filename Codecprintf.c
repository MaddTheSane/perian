/*
 * Codecprintf.c
 * Created by Augie Fackler on 7/16/06.
 *
 * This file is part of Perian.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "Codecprintf.h"
#include <stdio.h>
#include <stdarg.h>
#include "log.h"

#define CODEC_HEADER			"Perian: "

static int Codecvprintf(FILE *fileLog, const char *format, va_list va, int print_header)
{
	int ret = 0;
	
	if(fileLog)
	{
		if(print_header)
			fprintf(fileLog, CODEC_HEADER);
		ret = vfprintf(fileLog, format, va);
		fflush(fileLog);
	}
	else
	{
#ifdef DEBUG_BUILD
		if(print_header)
			printf(CODEC_HEADER);
		
		ret = vprintf(format, va);
#endif
	}
	
	return ret;
}

int Codecprintf(FILE *fileLog, const char *format, ...)
{
	int ret;
	va_list va;
	va_start(va, format);
	ret = Codecvprintf(fileLog, format, va, !fileLog);
	va_end(va);
	return ret;
}

const char *FourCCString(FourCharCode c)
{
    static char fourcc[sizeof("0xFFFF")] = {0};
    int i;
    
	//not a fourcc or twocc
	if (c < '\0\0AA') {
		snprintf(fourcc, sizeof(fourcc), "%#x", (unsigned)c);
	} else {
		for (i = 0; i < 4; i++) fourcc[i] = c >> 8*(3-i);
		fourcc[4] = '\0';
	}
    
    return (char*)fourcc;
}

void FourCCprintf(const char *string, FourCharCode a)
{
    Codecprintf(NULL, "%s%s\n", string, FourCCString(a));
}

void FFMpegCodecprintf(void* ptr, int level, const char* fmt, va_list vl)
{
    static int print_prefix=1;
	int print_header = 1;
    AVClass* avc= ptr ? *(AVClass**)ptr : NULL;
    if(level>av_log_get_level())
        return;

    if(print_prefix && avc) {
		Codecprintf(NULL, "[%s @ %p]", avc->item_name(ptr), avc);
		print_header = 0;
    }
	
    print_prefix= strstr(fmt, "\n") != NULL;
	
	Codecvprintf(NULL, fmt, vl, print_header);
}