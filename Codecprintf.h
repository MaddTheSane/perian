/*
 * Codecprintf.h
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

#ifndef CODECPRINTF_H
#define CODECPRINTF_H
#include <stdio.h>
#include <QuickTime/QuickTime.h>

#ifdef __cplusplus
extern "C" {
#endif

#undef printf
int Codecprintf(FILE *, const char *format, ...) __attribute__((format (printf, 2, 3)));
void FourCCprintf(const char *string, FourCharCode a);
const char *FourCCString(FourCharCode c);

void FFMpegCodecprintf(void* ptr, int level, const char* fmt, va_list vl);

#ifdef __cplusplus
}
#endif
#endif