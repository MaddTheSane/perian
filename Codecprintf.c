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
int Codecprintf(const char *format, ...)
{
	va_list ap; /* Points to each unamed argument in turn */
	va_start(ap, format); /* Make ap point to the first unnamed argument */
    char *output = malloc(sizeof(char)*strlen(format)+strlen(CODEC_HEADER)+1);
	strncpy(output,CODEC_HEADER,strlen(CODEC_HEADER));
	strncat(output,format,sizeof(char)*strlen(format));
	int retV = vprintf(output,ap);
	dealloc(output);
	va_end(ap);
	return retV;
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