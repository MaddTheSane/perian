/*
 * ColorConversions.c
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

#include <QuickTime/QuickTime.h>
#include <Accelerate/Accelerate.h>
#include "ColorConversions.h"
#include "Codecprintf.h"
#include "CommonUtils.h"

/*
 Converts (without resampling) from ffmpeg pixel formats to the ones QT accepts
 
 Todo:
 - rewrite everything in asm (or C with all loop optimization opportunities removed)
 - add a version with bilinear resampling
 - handle YUV 4:2:0 with odd width
 */

#define unlikely(x) __builtin_expect(x, 0)
#define likely(x) __builtin_expect(x, 1)

//Handles the last row for Y420 videos with an odd number of luma rows
//FIXME: odd number of luma columns is not handled and they will be lost
static void Y420toY422_lastrow(UInt8 *o, UInt8 *yc, UInt8 *uc, UInt8 *vc, int halfWidth)
{
	int x;
	for(x=0; x < halfWidth; x++)
	{
		int x4 = x*4, x2 = x*2;

		o[x4]   = uc[x];
		o[x4+1] = yc[x2];
		o[x4+2] = vc[x];
		o[x4+3] = yc[x2+1];
	}
}

#define HandleLastRow(o, yc, uc, vc, halfWidth, height) if (unlikely(height & 1)) Y420toY422_lastrow(o, yc, uc, vc, halfWidth)

//Y420 Planar to Y422 Packed
//The only one anyone cares about, so implemented with SIMD
#include <emmintrin.h>

static FASTCALL void Y420toY422_sse2(AVPicture *picture, UInt8 *o, int outRB, int width, int height)
{
	UInt8	*yc = picture->data[0], *uc = picture->data[1], *vc = picture->data[2];
	int		rY = picture->linesize[0], rUV = picture->linesize[1];
	int		y, x, halfwidth = width >> 1, halfheight = height >> 1;
	int		vWidth = width >> 5;
	
	for (y = 0; y < halfheight; y++) {
		UInt8   *o2 = o + outRB,   *yc2 = yc + rY;
		__m128i *ov = (__m128i*)o, *ov2 = (__m128i*)o2, *yv = (__m128i*)yc, *yv2 = (__m128i*)yc2;
		__m128i *uv = (__m128i*)uc,*vv  = (__m128i*)vc;
		
#ifdef __i386__
		int vWidth_ = vWidth;

		asm volatile(
			"\n0:			\n\t"
			"movdqa		(%2),	%%xmm0	\n\t"
			"movdqa		16(%2),	%%xmm2	\n\t"
			"movdqa		(%3),		%%xmm1	\n\t"
			"movdqa		16(%3),	%%xmm3	\n\t"
			"movdqu		(%4),	%%xmm4	\n\t"
			"movdqu		(%5),	%%xmm5	\n\t"
			"addl		$32,	%2		\n\t"
			"addl		$32,	%3		\n\t"
			"addl		$16,	%4		\n\t"
			"addl		$16,	%5		\n\t"
			"movdqa		%%xmm4, %%xmm6	\n\t"
			"punpcklbw	%%xmm5, %%xmm4	\n\t" /*chroma_l*/
			"punpckhbw	%%xmm5, %%xmm6	\n\t" /*chroma_h*/
			"movdqa		%%xmm4, %%xmm5	\n\t"
			"punpcklbw	%%xmm0, %%xmm5	\n\t"
			"movntdq	%%xmm5, (%0)	\n\t" /*ov[x4]*/
			"movdqa		%%xmm4, %%xmm5	\n\t"
			"punpckhbw	%%xmm0, %%xmm5	\n\t"
			"movntdq	%%xmm5, 16(%0)	\n\t" /*ov[x4+1]*/
			"movdqa		%%xmm6, %%xmm5	\n\t"
			"punpcklbw	%%xmm2, %%xmm5	\n\t"
			"movntdq	%%xmm5, 32(%0)	\n\t" /*ov[x4+2]*/
			"movdqa		%%xmm6, %%xmm5	\n\t"
			"punpckhbw	%%xmm2, %%xmm5	\n\t"
			"movntdq	%%xmm5, 48(%0)	\n\t" /*ov[x4+3]*/
			"addl		$64,	%0		\n\t"
			"movdqa		%%xmm4, %%xmm5	\n\t"
			"punpcklbw	%%xmm1, %%xmm5	\n\t"
			"movntdq	%%xmm5, (%1)	\n\t" /*ov2[x4]*/
			"punpckhbw	%%xmm1, %%xmm4	\n\t"
			"movntdq	%%xmm4, 16(%1)	\n\t" /*ov2[x4+1]*/
			"movdqa		%%xmm6, %%xmm5	\n\t"
			"punpcklbw	%%xmm3, %%xmm5	\n\t"
			"movntdq	%%xmm5, 32(%1)	\n\t" /*ov2[x4+2]*/
			"punpckhbw	%%xmm3, %%xmm6	\n\t"
			"movntdq	%%xmm6, 48(%1)	\n\t" /*ov2[x4+3]*/
			"addl		$64,	%1		\n\t"
			"decl		%6				\n\t"
			"jnz		0b				\n\t"
			: "+r" (ov), "+r" (ov2), "+r" (yv),
			  "+r" (yv2), "+r" (uv), "+r" (vv), "+m"(vWidth_)
			:
			: "memory", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
			);
#else
		for (x = 0; x < vWidth; x++) {
			int x2 = x*2, x4 = x*4;

			__m128i	tmp_y = yv[x2], tmp_y3 = yv[x2+1],
					tmp_y2 = yv2[x2], tmp_y4 = yv2[x2+1],
					tmp_u = _mm_loadu_si128(&uv[x]), tmp_v = _mm_loadu_si128(&vv[x]),
					chroma_l = _mm_unpacklo_epi8(tmp_u, tmp_v),
					chroma_h = _mm_unpackhi_epi8(tmp_u, tmp_v);
			
			_mm_stream_si128(&ov[x4],   _mm_unpacklo_epi8(chroma_l, tmp_y)); 
			_mm_stream_si128(&ov[x4+1], _mm_unpackhi_epi8(chroma_l, tmp_y)); 
			_mm_stream_si128(&ov[x4+2], _mm_unpacklo_epi8(chroma_h, tmp_y3)); 
			_mm_stream_si128(&ov[x4+3], _mm_unpackhi_epi8(chroma_h, tmp_y3)); 
			
			_mm_stream_si128(&ov2[x4],  _mm_unpacklo_epi8(chroma_l, tmp_y2)); 
			_mm_stream_si128(&ov2[x4+1],_mm_unpackhi_epi8(chroma_l, tmp_y2));
			_mm_stream_si128(&ov2[x4+2],_mm_unpacklo_epi8(chroma_h, tmp_y4));
			_mm_stream_si128(&ov2[x4+3],_mm_unpackhi_epi8(chroma_h, tmp_y4));
		}
#endif

		for (x=vWidth * 16; x < halfwidth; x++) {
			int x4 = x*4, x2 = x*2;
			o2[x4]     = o[x4] = uc[x];
			o [x4 + 1] = yc[x2];
			o2[x4 + 1] = yc2[x2];
			o2[x4 + 2] = o[x4 + 2] = vc[x];
			o [x4 + 3] = yc[x2 + 1];
			o2[x4 + 3] = yc2[x2 + 1];
		}			
		
		o  += outRB*2;
		yc += rY*2;
		uc += rUV;
		vc += rUV;
	}

	HandleLastRow(o, yc, uc, vc, halfwidth, height);
}

static FASTCALL void Y420toY422_x86_scalar(AVPicture *picture, UInt8 *o, int outRB, int width, int height)
{
	UInt8	*yc = picture->data[0], *u = picture->data[1], *v = picture->data[2];
	int		rY = picture->linesize[0], rUV = picture->linesize[1];
	int		halfheight = height >> 1, halfwidth = width >> 1;
	int		y, x;
	
	for (y = 0; y < halfheight; y ++) {
		UInt8 *o2 = o + outRB, *yc2 = yc + rY;
		
		for (x = 0; x < halfwidth; x++) {
			int x4 = x*4, x2 = x*2;
			o2[x4]     = o[x4] = u[x];
			o [x4 + 1] = yc[x2];
			o2[x4 + 1] = yc2[x2];
			o2[x4 + 2] = o[x4 + 2] = v[x];
			o [x4 + 3] = yc[x2 + 1];
			o2[x4 + 3] = yc2[x2 + 1];
		}
		
		o  += outRB*2;
		yc += rY*2;
		u  += rUV;
		v  += rUV;
	}

	HandleLastRow(o, yc, u, v, halfwidth, height);
}

//Y420+Alpha Planar to V408 (YUV 4:4:4+Alpha 32-bit packed)
//Could be fully unrolled to avoid x/2
static FASTCALL void YA420toV408(AVPicture *picture, UInt8 *o, int outRB, int width, int height)
{
	UInt8	*yc = picture->data[0], *u = picture->data[1], *v = picture->data[2], *a = picture->data[3];
	int		rYA = picture->linesize[0], rUV = picture->linesize[1];
	int y, x;
	
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			o[x*4]   = u[x>>1];
			o[x*4+1] = yc[x];
			o[x*4+2] = v[x>>1];
			o[x*4+3] = a[x];
		}
		
		o  += outRB;
		yc += rYA;
		a  += rYA;
		if (y & 1) {
			u += rUV;
			v += rUV;
		}
	}
}

static FASTCALL void BGR24toRGB24(AVPicture *picture, UInt8 *baseAddr, int rowBytes, int width, int height)
{
	UInt8 *srcPtr = picture->data[0];
	int srcRB = picture->linesize[0];
	int x, y;
	
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int x3 = x * 3;
			baseAddr[x3] = srcPtr[x3+2];
			baseAddr[x3+1] = srcPtr[x3+1];
			baseAddr[x3+2] = srcPtr[x3];
		}
		baseAddr += rowBytes;
		srcPtr += srcRB;
	}
}

static FASTCALL void RGBtoRGB(AVPicture *picture, UInt8 *baseAddr, int rowBytes, int width, int height, int bytesPerPixel)
{
	UInt8 *srcPtr = picture->data[0];
	int srcRB = picture->linesize[0];
	int y;
	
	for (y = 0; y < height; y++) {
		memcpy(baseAddr, srcPtr, width * bytesPerPixel);
		
		baseAddr += rowBytes;
		srcPtr += srcRB;
	}
}

//Big-endian XRGB32 to big-endian XRGB32
static FASTCALL void RGB32toRGB32Copy(AVPicture *picture, UInt8 *baseAddr, int rowBytes, int width, int height)
{
	RGBtoRGB(picture, baseAddr, rowBytes, width, height, 4);
}

static FASTCALL void RGB24toRGB24(AVPicture *picture, UInt8 *baseAddr, int rowBytes, int width, int height)
{
	RGBtoRGB(picture, baseAddr, rowBytes, width, height, 3);
}

static FASTCALL void RGB16toRGB16(AVPicture *picture, UInt8 *baseAddr, int rowBytes, int width, int height)
{
	RGBtoRGB(picture, baseAddr, rowBytes, width, height, 2);
}

//Little-endian XRGB32 to big-endian XRGB32
static FASTCALL void RGB32toRGB32Swap(AVPicture *picture, UInt8 *baseAddr, int rowBytes, int width, int height)
{
	UInt8 *srcPtr = picture->data[0];
	int srcRB = picture->linesize[0];
	int x, y;
	
	for (y = 0; y < height; y++) {
		UInt32 *oRow = (UInt32 *)baseAddr, *iRow = (UInt32 *)srcPtr;
		for (x = 0; x < width; x++) oRow[x] = EndianU32_LtoB(iRow[x]);
		
		baseAddr += rowBytes;
		srcPtr += srcRB;
	}
}

static FASTCALL void RGB16toRGB16Swap(AVPicture *picture, UInt8 *baseAddr, int rowBytes, int width, int height)
{
	UInt8 *srcPtr = picture->data[0];
	int srcRB = picture->linesize[0];
	int x, y;
	
	for (y = 0; y < height; y++) {
		UInt16 *oRow = (UInt16 *)baseAddr, *iRow = (UInt16 *)srcPtr;
		for (x = 0; x < width; x++) oRow[x] = EndianU16_LtoB(iRow[x]);
		
		baseAddr += rowBytes;
		srcPtr += srcRB;
	}
}

static FASTCALL void Y422toY422(AVPicture *picture, UInt8 *o, int outRB, int width, int height)
{
	UInt8	*yc = picture->data[0], *u = picture->data[1], *v = picture->data[2];
	int		rY = picture->linesize[0], rUV = picture->linesize[1];
	int		x, y, halfwidth = width >> 1;
	
	for (y = 0; y < height; y++) {
		for (x = 0; x < halfwidth; x++) {
			int x2 = x * 2, x4 = x * 4;
			o[x4] = u[x];
			o[x4 + 1] = yc[x2];
			o[x4 + 2] = v[x];
			o[x4 + 3] = yc[x2 + 1];
		}
		
		o  += outRB;
		yc += rY;
		u  += rUV;
		v  += rUV;
	}
}

static FASTCALL void Y410toY422(AVPicture *picture, UInt8 *o, int outRB, int width, int height)
{
	UInt8	*yc = picture->data[0], *u = picture->data[1], *v = picture->data[2];
	int		rY = picture->linesize[0], rUV = picture->linesize[1];
	int		x, y, halfwidth = width >> 1;
	
	for (y = 0; y < height; y++) {
		for (x = 0; x < halfwidth; x++) {
			int x2 = x * 2, x4 = x * 4;
			o[x4] = u[x>>1];
			o[x4 + 1] = yc[x2];
			o[x4 + 2] = v[x>>1];
			o[x4 + 3] = yc[x2 + 1];
		}
		
		o  += outRB;
		yc += rY;
		
		if ((y & 3) == 3) {
			u += rUV;
			v += rUV;
		}
	}
}

static void ClearRGB(UInt8 *baseAddr, int rowBytes, int width, int height, int bytesPerPixel)
{
	int y;
	
	for (y = 0; y < height; y++) {
		memset(baseAddr, 0, width * bytesPerPixel);
		
		baseAddr += rowBytes;
	}
}

static FASTCALL void ClearRGB32(UInt8 *baseAddr, int rowBytes, int width, int height)
{
	ClearRGB(baseAddr, rowBytes, width, height, 4);
}

static FASTCALL void ClearRGB24(UInt8 *baseAddr, int rowBytes, int width, int height)
{
	ClearRGB(baseAddr, rowBytes, width, height, 3);
}

static FASTCALL void ClearRGB16(UInt8 *baseAddr, int rowBytes, int width, int height)
{
	ClearRGB(baseAddr, rowBytes, width, height, 2);
}

static FASTCALL void ClearV408(UInt8 *baseAddr, int rowBytes, int width, int height)
{
	int x, y;
	
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int x4 = x * 4;
			baseAddr[x4]   = 0x80; //zero chroma
			baseAddr[x4+1] = 0x10; //black
			baseAddr[x4+2] = 0x80; 
			baseAddr[x4+3] = 0xEB; //opaque
		}
		baseAddr += rowBytes;
	}
}

static FASTCALL void ClearY422(UInt8 *baseAddr, int rowBytes, int width, int height)
{
	int x, y;
	
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int x2 = x * 2;
			baseAddr[x2]   = 0x80; //zero chroma
			baseAddr[x2+1] = 0x10; //black
		}
		baseAddr += rowBytes;
	}
}

OSType ColorConversionDstForPixFmt(enum PixelFormat ffPixFmt)
{
	switch (ffPixFmt) {
		case PIX_FMT_RGB555LE:
		case PIX_FMT_RGB555BE:
			return k16BE555PixelFormat;
		case PIX_FMT_BGR24:
			return k24RGBPixelFormat; //FIXME: try k24BGRPixelFormat
		case PIX_FMT_RGB24:
			return k24RGBPixelFormat;
		case PIX_FMT_ARGB:
		case PIX_FMT_BGRA:
			return k32ARGBPixelFormat;
		case PIX_FMT_YUV410P:
			return k2vuyPixelFormat;
		case PIX_FMT_YUVJ420P:
		case PIX_FMT_YUV420P:
			return k2vuyPixelFormat; //disables "fast YUV" path
		case PIX_FMT_YUV422P:
			return k2vuyPixelFormat;
		case PIX_FMT_YUVA420P:
			return k4444YpCbCrA8PixelFormat;
		default:
			return 0; // error
	}
}

int ColorConversionFindFor(ColorConversionFuncs *funcs, enum PixelFormat ffPixFmt, AVPicture *ffPicture, OSType qtPixFmt)
{
	switch (ffPixFmt) {
		case PIX_FMT_YUVJ420P:
		case PIX_FMT_YUV420P:
			funcs->clear = ClearY422;
			
			//can't set this without the first real frame
			if (ffPicture) {
				if (ffPicture->linesize[0] & 15)
					funcs->convert = Y420toY422_x86_scalar;
				else
					funcs->convert = Y420toY422_sse2;
			}
			break;
		case PIX_FMT_BGR24:
			funcs->clear = ClearRGB24;
			funcs->convert = BGR24toRGB24;
			break;
		case PIX_FMT_ARGB:
			funcs->clear = ClearRGB32;
#ifdef __BIG_ENDIAN__
			funcs->convert = RGB32toRGB32Swap;
#else
			funcs->convert = RGB32toRGB32Copy;
#endif
			break;
		case PIX_FMT_BGRA:
			funcs->clear = ClearRGB32;
#ifdef __BIG_ENDIAN__
			funcs->convert = RGB32toRGB32Copy;
#else
			funcs->convert = RGB32toRGB32Swap;
#endif
			break;
		case PIX_FMT_RGB24:
			funcs->clear = ClearRGB24;
			funcs->convert = RGB24toRGB24;
			break;
		case PIX_FMT_RGB555LE:
			funcs->clear = ClearRGB16;
			funcs->convert = RGB16toRGB16Swap;
			break;
		case PIX_FMT_RGB555BE:
			funcs->clear = ClearRGB16;
			funcs->convert = RGB16toRGB16;
			break;
		case PIX_FMT_YUV410P:
			funcs->clear = ClearY422;
			funcs->convert = Y410toY422;
			break;
		case PIX_FMT_YUV422P:
			funcs->clear = ClearY422;
			funcs->convert = Y422toY422;
			break;
		case PIX_FMT_YUVA420P:
			funcs->clear = ClearV408;
			funcs->convert = YA420toV408;
			break;
		default:
			return paramErr;
	}
	
	return noErr;
}