/*
 * FFissionCodec.h
 * Copyright (c) 2006 David Conrad
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


#ifndef __FFISSIONCODEC_H__
#define __FFISSIONCODEC_H__

#include "ACSimpleCodec.h"
extern "C" {
#define CodecType FF_CodecType
#include "avcodec.h"
#undef CodecType
}

//=============================================================================
//	ACffAudioCodec
//
//	This class encapsulates the common implementation of an audio codec using ffmpeg.
//=============================================================================

class FFissionCodec : public ACSimpleCodec {
	
	//	Construction/Destruction
public:
	FFissionCodec(UInt32 inInputBufferByteSize);
	virtual ~FFissionCodec();
	
	//	Data Handling
	virtual void Initialize(const AudioStreamBasicDescription* inInputFormat, const AudioStreamBasicDescription* inOutputFormat, const void* inMagicCookie, UInt32 inMagicCookieByteSize);
	
	virtual void GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData);
	virtual void GetPropertyInfo(AudioCodecPropertyID inPropertyID, UInt32& outPropertyDataSize, bool& outWritable);
	
protected:
	AVCodec         *avCodec;
	AVCodecContext  *avContext;
};

#endif
