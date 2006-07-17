/*
 *  Codecprintf.h
 *  Perian
 *
 *  Created by Augie Fackler on 7/16/06.
 *  Copyright 2006 Perian Project. All rights reserved.
 *
 */

#ifdef DEBUG_BUILD
int Codecprintf(const char *format, ...);
void FourCCprintf(char *string, unsigned long a);
#else
#define Codecprintf(fmt, ...) /**/
#define FourCCprintf(string,a) /**/
#endif
