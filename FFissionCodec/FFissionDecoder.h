/*
 * FFissionDecoder.h
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

#ifndef __FFISSIONDECODER_H__
#define __FFISSIONDECODER_H__

#include "FFissionCodec.h"
#include "RingBuffer.h"

class FFissionDecoder : public FFissionCodec
{
public:
	FFissionDecoder(AudioComponentInstance inInstance);
	virtual ~FFissionDecoder();
	
	virtual void Initialize(const AudioStreamBasicDescription* inInputFormat, const AudioStreamBasicDescription* inOutputFormat, const void* inMagicCookie, UInt32 inMagicCookieByteSize);
	virtual void Uninitialize();
	virtual void Reset();
	
	virtual void GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData);
	virtual void SetMagicCookie(const void* inMagicCookieData, UInt32 inMagicCookieDataByteSize);
	
	virtual void SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat);
	virtual void SetCurrentOutputFormat(const AudioStreamBasicDescription& inOutputFormat);
	virtual UInt32 GetVersion() const;

	virtual void AppendInputData(const void* inInputData, UInt32& ioInputDataByteSize, UInt32& ioNumberPackets, const AudioStreamPacketDescription* inPacketDescription);
	virtual UInt32 InterleaveSamples(void *outputDataUntyped, Byte *inputDataUntyped, int amountToCopy);
	virtual UInt32 ProduceOutputPackets(void* outOutputData, UInt32& ioOutputDataByteSize, UInt32& ioNumberPackets, AudioStreamPacketDescription* outPacketDescription);
	
	virtual void ReallocateInputBuffer(UInt32 inInputBufferByteSize);
	virtual UInt32 GetInputBufferByteSize() const;
	virtual UInt32 GetUsedInputBufferByteSize() const;

private:
	void SetupExtradata();
	int ConvertXiphVorbisCookie();
	void OpenAVCodec();
	void CloseAVCodec();
	
	Byte *magicCookie;
	UInt32 magicCookieSize;
	
	RingBuffer inputBuffer;
	Byte *outputBuffer;
	int outBufSize;
	int outBufUsed;
	bool dtsPassthrough;
	int fullChannelMap[6];
	AVCodecParserContext *parser;
};

// kAudioCodecPropertyHasVariablePacketByteSizes is queried before our input format is set,
// so we can't use that to determine our answer...
class FFissionVBRDecoder : public FFissionDecoder
{
public:
	FFissionVBRDecoder(AudioComponentInstance inInstance) : FFissionDecoder(inInstance) { }
	virtual void GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData);
};

#endif
