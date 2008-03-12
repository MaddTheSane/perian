/*
 *  ColorConversions.h
 *  Perian
 *
 *  Created by Alexander Strange on 1/10/07.
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA */

#include "ColorConversions.h"
#include <QuickTime/QuickTime.h>
#include <Accelerate/Accelerate.h>
#include <sys/types.h>
#include <sys/sysctl.h>

//-----------------------------------------------------------------
// FastY420
//-----------------------------------------------------------------
// Returns y420 data directly to QuickTime which then converts
// in RGB for display
//-----------------------------------------------------------------

void FastY420(UInt8 *baseAddr, AVFrame *picture)
{
    PlanarPixmapInfoYUV420 *planar;
	
	/*From Docs: PixMap baseAddr points to a big-endian PlanarPixmapInfoYUV420 struct; see ImageCodec.i. */
    planar = (PlanarPixmapInfoYUV420 *) baseAddr;
    
    // if ya can't set da poiners, set da offsets
    planar->componentInfoY.offset = EndianU32_NtoB(picture->data[0] - baseAddr);
    planar->componentInfoCb.offset =  EndianU32_NtoB(picture->data[1] - baseAddr);
    planar->componentInfoCr.offset =  EndianU32_NtoB(picture->data[2] - baseAddr);
    
    // for the 16/32 add look at EDGE in mpegvideo.c
    planar->componentInfoY.rowBytes = EndianU32_NtoB(picture->linesize[0]);
    planar->componentInfoCb.rowBytes = EndianU32_NtoB(picture->linesize[1]);
    planar->componentInfoCr.rowBytes = EndianU32_NtoB(picture->linesize[2]);
}

//-----------------------------------------------------------------
// FFusionSlowDecompress
//-----------------------------------------------------------------
// We have to return 2yuv values because
// QT version has no built-in y420 component.
// Since we do the conversion ourselves it is not really optimized....
// The function should never be called since many people now
// have a decent OS/QT version.
//-----------------------------------------------------------------

#ifdef __BIG_ENDIAN__
//hand-unrolled code is a bad idea on modern CPUs. luckily, this does not run on modern CPUs, only G3s.
//also, big-endian only

static void Y420toY422_ppc_scalar(UInt8* baseAddr, unsigned outRB, unsigned width, unsigned height, AVFrame * picture)
{
	 unsigned             y = height / 2;
	 unsigned             halfWidth = width / 2, halfHalfWidth = width / 4;
	 UInt8          *inY = picture->data[0], *inU = picture->data[1], *inV = picture->data[2];
	 int             rB = picture->linesize[0], rbU = picture->linesize[1], rbV = picture->linesize[2];
	 
	 while (y--) {
		 UInt32         *ldst = (UInt32 *) baseAddr, *ldstr2 = (UInt32 *) (baseAddr + outRB);
		 UInt32         *lsrc = (UInt32 *) inY, *lsrcr2 = (UInt32 *) (inY + rB);
		 UInt16         *sU = (UInt16 *) inU, *sV = (UInt16 *) inV;
		 ptrdiff_t		off;
		 
		 for (off = 0; off < halfHalfWidth; off++) {
			 UInt16          chrU = sU[off], chrV = sV[off];
			 UInt32          row1luma = lsrc[off], row2luma = lsrcr2[off];
			 UInt32          chromas1 = (chrU & 0xff00) << 16 | (chrV & 0xff00), chromas2 = (chrU & 0xff) << 24 | (chrV & 0xff) << 8;
			 int             off2 = off * 2;
			 
			 ldst[off2] = chromas1 | (row1luma & 0xff000000) >> 8 | (row1luma & 0xff0000) >> 16;
			 ldstr2[off2] = chromas1 | (row2luma & 0xff000000) >> 8 | (row2luma & 0xff0000) >> 16;
			 off2++;
			 ldst[off2] = chromas2 | (row1luma & 0xff00) << 8 | row1luma & 0xff;
			 ldstr2[off2] = chromas2 | (row2luma & 0xff00) << 8 | row2luma & 0xff;
		 }
		 
		 if (halfWidth % 4) {
			 UInt16         *ssrc = (UInt16 *) inY, *ssrcr2 = (UInt16 *) (inY + rB);
			 
			 ptrdiff_t       off = halfWidth - 2;
			 UInt32          chromas = inV[off] << 8 | (inU[off] << 24);
			 UInt16          row1luma = ssrc[off], row2luma = ssrcr2[off];
			 
			 ldst[off] = chromas | row1luma & 0xff | (row1luma & 0xff00) << 8;
			 ldstr2[off] = chromas | row2luma & 0xff | (row2luma & 0xff00) << 8;
		 }
		 inY += rB * 2;
		 inU += rbU;
		 inV += rbV;
		 baseAddr += outRB * 2;
	 }
 }

static void Y420toY422_ppc_altivec(UInt8 * o, unsigned outRB, unsigned width, unsigned height, AVFrame * picture)
{
	UInt8			*yc = picture->data[0], *uc = picture->data[1], *vc = picture->data[2];
	unsigned		rY = picture->linesize[0], rU = picture->linesize[1], rV = picture->linesize[2];
	unsigned		y,x,x2,x4, vWidth = width / 32, halfheight = height / 2;
	
	for (y = 0; y < halfheight; y ++) {
		vUInt8 *ov = (vUInt8 *)o, *ov2 = (vUInt8 *)(o + outRB), *yv2 = (vUInt8 *)(yc + rY);
		vUInt8 *uv  = (vUInt8 *)uc, *vv = (vUInt8 *)vc, *yv = (vUInt8 *)yc;
		
		for (x = 0; x < vWidth; x++) {
			x2 = x*2; x4 = x*4;
			// ldl/stl = mark data as least recently used in cache so they will be flushed out
			__builtin_prefetch(&yv[x+1], 0, 0); __builtin_prefetch(&yv2[x+1], 0, 0);
			__builtin_prefetch(&uv[x+1], 0, 0); __builtin_prefetch(&vv[x+1], 0, 0);
			vUInt8 tmp_u = vec_ldl(0, &uv[x]), tmp_v = vec_ldl(0, &vv[x]), chroma = vec_mergeh(tmp_u, tmp_v),
					tmp_y = vec_ldl(0, &yv[x2]), tmp_y2 = vec_ldl(0, &yv2[x2]),
					tmp_y3 = vec_ldl(16, &yv[x2]), tmp_y4 = vec_ldl(16, &yv2[x2]), chromal = vec_mergel(tmp_u, tmp_v);
			
			vec_stl(vec_mergeh(chroma, tmp_y), 0, &ov[x4]);
			vec_stl(vec_mergel(chroma, tmp_y), 16, &ov[x4]);
			vec_stl(vec_mergeh(chromal, tmp_y3), 32, &ov[x4]);
			vec_stl(vec_mergel(chromal, tmp_y3), 48, &ov[x4]);
			
			vec_stl(vec_mergeh(chroma, tmp_y2), 0, &ov2[x4]);
			vec_stl(vec_mergel(chroma, tmp_y2), 16, &ov2[x4]);
			vec_stl(vec_mergeh(chromal, tmp_y4), 32, &ov2[x4]);
			vec_stl(vec_mergel(chromal, tmp_y4), 48, &ov2[x4]);
		}
		
		if (width % 32) { //spill to scalar for the end if the row isn't a multiple of 32
			UInt8 *o2 = o + outRB, *yc2 = yc + rY;
			for (x = vWidth * 32, x2 = x*2; x < width; x += 2, x2 += 4) {
				unsigned             hx = x / 2;
				o2[x2] = o[x2] = uc[hx];
				o[x2 + 1] = yc[x];
				o2[x2 + 1] = yc2[x];
				o2[x2 + 2] = o[x2 + 2] = vc[hx];
				o[x2 + 3] = yc[x + 1];
				o2[x2 + 3] = yc2[x + 1];
			}			
		}
		
		o += outRB; o += outRB;
		yc += rY; yc += rY;
		uc += rU;
		vc += rV;
	}
}

void Y420toY422(UInt8 * o, unsigned outRB, unsigned width, unsigned height, AVFrame * picture)
{
	static void (*y420_function)(UInt8* baseAddr, unsigned outRB, unsigned width, unsigned height, AVFrame * picture) = NULL;
	if (!y420_function) {
		int sels[2] = { CTL_HW, HW_VECTORUNIT }; // from http://developer.apple.com/hardwaredrivers/ve/g3_compatibility.html
		int vType = 0; //0 == scalar only
		size_t length = sizeof(vType);
		int error = sysctl(sels, 2, &vType, &length, NULL, 0);
		if( 0 == error && vType ) y420_function = Y420toY422_ppc_altivec;
		else 
		y420_function = Y420toY422_ppc_scalar;
	}
	
	y420_function(o, outRB, width, height, picture);
}
#else
#include <emmintrin.h>

static void Y420toY422_sse2(UInt8 *  o, unsigned outRB, unsigned width, unsigned height, AVFrame * picture)
{
	UInt8	*yc = picture->data[0], *uc = picture->data[1], *vc = picture->data[2];
	unsigned	rY = picture->linesize[0], rU = picture->linesize[1], rV = picture->linesize[2];
	unsigned	y,x, vWidth = width / 32, halfheight = height / 2;
	unsigned	halfwidth = width / 2; 
	
	for (y = 0; y < halfheight; y++) {
		UInt8   * o2 = o + outRB,   * yc2 = yc + rY;
		__m128i * ov = (__m128i*)o, * ov2 = (__m128i*)o2, * yv = (__m128i*)yc, * yv2 = (__m128i*)yc2;
		__m128i * uv = (__m128i*)uc,* vv  = (__m128i*)vc;
		
		for (x = 0; x < vWidth; x++) {
			unsigned x2 = x*2, x4 = x*4;

#if 0
			asm volatile(
				"movdqu		%4,		%%xmm4	\n\t"
				"movdqu		%5,		%%xmm5	\n\t"
				"movdqa		%2,		%%xmm0	\n\t"
				"movdqa	16+1*%2,	%%xmm2	\n\t"
				"movdqa		%3,		%%xmm1	\n\t"
				"movdqa	16+1*%3,	%%xmm3	\n\t"
				"movdqa		%%xmm4, %%xmm6	\n\t"
				"punpcklbw	%%xmm5, %%xmm4	\n\t" /*chroma_l*/
				"punpckhbw	%%xmm5, %%xmm6	\n\t" /*chroma_h*/
				"movdqa		%%xmm4, %%xmm5	\n\t"
				"punpcklbw	%%xmm0, %%xmm5	\n\t"
				"movntdq	%%xmm5, %0		\n\t" /*ov[x4]*/
				"movdqa		%%xmm4, %%xmm5	\n\t"
				"punpckhbw	%%xmm0, %%xmm5	\n\t"
				"movntdq	%%xmm5, 16+1*%0	\n\t" /*ov[x4+1]*/
				"movdqa		%%xmm6, %%xmm5	\n\t"
				"punpcklbw	%%xmm2, %%xmm5	\n\t"
				"movntdq	%%xmm5, 32+1*%0	\n\t" /*ov[x4+2]*/
				"movdqa		%%xmm6, %%xmm5	\n\t"
				"punpckhbw	%%xmm2, %%xmm5	\n\t"
				"movntdq	%%xmm5, 48+1*%0	\n\t" /*ov[x4+3]*/
				"movdqa		%%xmm4, %%xmm5	\n\t"
				"punpcklbw	%%xmm1, %%xmm5	\n\t"
				"movntdq	%%xmm5, %1		\n\t" /*ov2[x4]*/
				"punpckhbw	%%xmm1, %%xmm4	\n\t"
				"movntdq	%%xmm4, 16+1*%1	\n\t" /*ov2[x4+1]*/
				"movdqa		%%xmm6, %%xmm5	\n\t"
				"punpcklbw	%%xmm3, %%xmm5	\n\t"
				"movntdq	%%xmm5, 32+1*%1	\n\t" /*ov2[x4+2]*/
				"punpckhbw	%%xmm3, %%xmm6	\n\t"
				"movntdq	%%xmm6, 48+1*%1	\n\t" /*ov2[x4+3]*/
				: "=m" (ov[x4]), "=m" (ov2[x4])
				: "m" (yv[x2]), "m" (yv2[x2]), "m" (uv[x]), "m" (vv[x])
				);
#else
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
#endif
		}
		
		for (x = x * 16; x < halfwidth; x++) {
			unsigned x4 = x*4, x2 = x*2;
			o2[x4] = o[x4] = uc[x];
			o[x4 + 1] = yc[x2];
			o2[x4 + 1] = yc2[x2];
			o2[x4 + 2] = o[x4 + 2] = vc[x];
			o[x4 + 3] = yc[x2 + 1];
			o2[x4 + 3] = yc2[x2 + 1];
		}			
		
		o += outRB*2;
		yc += rY*2;
		uc += rU;
		vc += rV;
	}
}


static void __attribute__((noinline)) Y420toY422_x86_scalar(UInt8 * o, unsigned outRB, unsigned width, unsigned height, AVFrame * picture)
{
	UInt8		*yc = picture->data[0], *u = picture->data[1], *v = picture->data[2];
	unsigned	rY = picture->linesize[0], rU = picture->linesize[1], rV = picture->linesize[2], halfheight = height / 2, halfwidth = width / 2;
	unsigned	y, x;
	
	for (y = 0; y < halfheight; y ++) {
		UInt8 *o2 = o + outRB, *yc2 = yc + rY;
		
		for (x = 0; x < halfwidth; x++) {
			unsigned x4 = x*4, x2 = x*2;
			o2[x4] = o[x4] = u[x];
			o[x4 + 1] = yc[x2];
			o2[x4 + 1] = yc2[x2];
			o2[x4 + 2] = o[x4 + 2] = v[x];
			o[x4 + 3] = yc[x2 + 1];
			o2[x4 + 3] = yc2[x2 + 1];
		}
		
		o += outRB*2;
		yc += rY*2;
		u += rU;
		v += rV;
	}
}

void Y420toY422(UInt8 * o, unsigned outRB, unsigned width, unsigned height, AVFrame * picture)
{
	uintptr_t yc = (uintptr_t)picture->data[0];

	//make sure the ffmpeg picture buffers are aligned enough, they're only guaranteed to be 8-byte for some reason...
	if ((yc | picture->linesize[0]) % 16 == 0) {
		Y420toY422_sse2(o, outRB, width, height, picture);
		_mm_sfence();
	} else Y420toY422_x86_scalar(o, outRB, width, height, picture);
}
#endif

void BGR24toRGB24(UInt8 *baseAddr, unsigned rowBytes, unsigned width, unsigned height, AVFrame *picture)
{
	unsigned i, j;
	UInt8 *srcPtr = picture->data[0];
	unsigned width_third = width / 3;
	
	for (i = 0; i < height; ++i)
	{
		for (j = 0; j < width_third; j ++)
		{
			unsigned j3 = j * 3;
			baseAddr[j3] = srcPtr[j3+2];
			baseAddr[j3+1] = srcPtr[j3+1];
			baseAddr[j3+2] = srcPtr[j3];
		}
		baseAddr += rowBytes;
		srcPtr += picture->linesize[0];
	}
}

void RGB32toRGB32(UInt8 *baseAddr, unsigned rowBytes, unsigned width, unsigned height, AVFrame *picture)
{
	unsigned y;
	UInt8 *srcPtr = picture->data[0];
	
	for (y = 0; y < height; y++) {
#ifdef __BIG_ENDIAN__
		memcpy(baseAddr, srcPtr, width * 4);
#else
		unsigned x;
		UInt32 *oRow = (UInt32 *)baseAddr, *iRow = (UInt32 *)srcPtr;
		for (x = 0; x < width; x++) {oRow[x] = EndianU32_BtoN(iRow[x]);}
#endif
		
		baseAddr += rowBytes;
		srcPtr += picture->linesize[0];
	}
}

void Y422toY422(UInt8* o, unsigned outRB, unsigned width, unsigned height, AVFrame * picture)
{
	UInt8          *yc = picture->data[0], *u = picture->data[1], *v = picture->data[2];
	unsigned       rY = picture->linesize[0], rU = picture->linesize[1], rV = picture->linesize[2], y, x, halfwidth = width / 2;
	
	for (y = 0; y < height; y++) {
		for (x = 0; x < halfwidth; x++) {
			unsigned x2 = x * 2, x4 = x * 4;
			o[x4] = u[x];
			o[x4 + 1] = yc[x2];
			o[x4 + 2] = v[x];
			o[x4 + 3] = yc[x2 + 1];
		}
		
		o += outRB;
		yc += rY;
		u += rU;
		v += rV;
	}
}
