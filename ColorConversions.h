/*
 * ColorConversions.h
 * Created by Alexander Strange on 1/10/07.
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

#ifndef __COLORCONVERSIONS_H__
#define __COLORCONVERSIONS_H__

#include <Carbon/Carbon.h>
#include "libavcodec/avcodec.h"

#if defined(__i386__)
#define FASTCALL __attribute__((fastcall))
#else
#define FASTCALL
#endif

// Converts libavcodec pixel formats to QT displayable pixel formats
// All parameters except buffer pointers are fixed for each frame
typedef struct CCConverterContext {
	// Input picture
	int inLineSizes[4];
	short width;
	short height;
	
	enum PixelFormat  inPixFmt;
	enum AVColorSpace inColorSpace;
	enum AVColorRange inColorRange;
	enum AVChromaLocation inChromaLocation;
	
	// Output picture
	int outLineSize;
	
	// Parameters end here
	enum PixelFormat outPixFmt;
	
	void (^convert)(AVPicture *inPicture, uint8_t *outPicture) FASTCALL;
	
	// Private
	int  type;
	void *opaque;
} CCConverterContext;

enum PixelFormat CCOutputPixFmtForInput(enum PixelFormat inPixFmt);
void CCClearPicture(CCConverterContext *ctx, uint8_t *outPicture);
void CCOpenConverter(CCConverterContext *ctx);
void CCCloseConverter(CCConverterContext *ctx);
#endif
