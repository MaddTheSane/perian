/*
 *  Codecprintf.h
 *  Perian
 *
 *  Created by Augie Fackler on 7/16/06.
 *  Copyright 2006 Perian Project. All rights reserved.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG_BUILD
int Codecprintf(FILE *, const char *format, ...);
void FourCCprintf(char *string, FourCharCode a);
const char *FourCCString(FourCharCode c);
#else
#define Codecprintf(file, fmt, ...) /**/
#define FourCCprintf(string,a) /**/
#endif

void FFMpegCodecprintf(void* ptr, int level, const char* fmt, va_list vl);

#ifdef __cplusplus
}
#endif
