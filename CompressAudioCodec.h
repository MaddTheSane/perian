/*
 * CompressAudioCodec.h
 * Created by Graham Booker on 8/14/10.
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

#ifndef __COMPRESSAUDIOCODEC_H__
#define __COMPRESSAUDIOCODEC_H__

#include "FFissionCodec.h"

class CompressAudioCodec : public FFissionCodec
{
public:
	CompressAudioCodec(AudioComponentInstance inInstance);
	virtual ~CompressAudioCodec();
	
	virtual void Initialize(const AudioStreamBasicDescription* inInputFormat, const AudioStreamBasicDescription* inOutputFormat, const void* inMagicCookie, UInt32 inMagicCookieByteSize);
	virtual void Uninitialize();
	virtual void Reset();

	virtual void GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData);
	virtual void SetMagicCookie(const void* inMagicCookieData, UInt32 inMagicCookieDataByteSize);
	
	virtual void SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat);
	virtual void SetCurrentOutputFormat(const AudioStreamBasicDescription& inOutputFormat);
	
	virtual void AppendInputData(const void* inInputData, UInt32& ioInputDataByteSize, UInt32& ioNumberPackets, const AudioStreamPacketDescription* inPacketDescription);
	virtual UInt32 ProduceOutputPackets(void* outOutputData, UInt32& ioOutputDataByteSize, UInt32& ioNumberPackets, AudioStreamPacketDescription* outPacketDescription);

	virtual void ReallocateInputBuffer(UInt32 inInputBufferByteSize) {}
	virtual UInt32 GetInputBufferByteSize() const {return 0;}
	virtual UInt32 GetUsedInputBufferByteSize() const {return 0;}

private:
	UInt32 ParseCookieAtom(const uint8_t* inAtom, UInt32 inAtomMaxSize);
	void ParseCookie(const uint8_t* inMagicCookie, UInt32 inMagicCookieByteSize);

	UInt32	strippedHeaderSize;
	Byte	*strippedHeader;
	
	UInt32	innerCookieSize;
	Byte	*innerCookie;
	
	AudioCodec	actualUnit;
};

#endif