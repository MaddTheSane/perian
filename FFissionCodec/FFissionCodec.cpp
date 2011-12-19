/*
 * FFissionCodec.cpp
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

/*
 * TODO:
 * This file is pointless as long as we don't have any encoders.
 * It should be merged into FFissionDecoder unless we get some.
 */

#include <AudioToolbox/AudioToolbox.h>
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "FFissionCodec.h"
#include "FFmpegUtils.h"

FFissionCodec::FFissionCodec(AudioComponentInstance inInstance) : ACBaseCodec(inInstance)
{
	FFInitFFmpeg();
	
	// FIXME avcodec_alloc_context3
	avContext = avcodec_alloc_context2(AVMEDIA_TYPE_AUDIO);
	avCodec = NULL;
}

FFissionCodec::~FFissionCodec()
{
	if (avContext) {
		if (avCodec) {
			avcodec_close(avContext);
		}
		
		av_free(avContext);
	}
}

void FFissionCodec::Initialize(const AudioStreamBasicDescription* inInputFormat,
							   const AudioStreamBasicDescription* inOutputFormat,
							   const void* inMagicCookie, UInt32 inMagicCookieByteSize)
{
	//	use the given arguments, if necessary
	if (inInputFormat != NULL)
	{
		SetCurrentInputFormat(*inInputFormat);
	}
	
	if (inOutputFormat != NULL)
	{
		SetCurrentOutputFormat(*inOutputFormat);
	}
	
	//	make sure the sample rate and number of channels match between the input format and the output format
	
	if ((mInputFormat.mSampleRate != mOutputFormat.mSampleRate) ||
		(mInputFormat.mChannelsPerFrame != mOutputFormat.mChannelsPerFrame))
	{
		CODEC_THROW(kAudioCodecUnsupportedFormatError);
	}
	
	ACBaseCodec::Initialize(inInputFormat, inOutputFormat, inMagicCookie, inMagicCookieByteSize);
}

void FFissionCodec::GetPropertyInfo(AudioCodecPropertyID inPropertyID, UInt32& outPropertyDataSize, Boolean& outWritable) {
	switch(inPropertyID)
	{
		case kAudioCodecPropertyMaximumPacketByteSize:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;
			
		case kAudioCodecPropertyHasVariablePacketByteSizes:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;
			
		case kAudioCodecPropertyPacketFrameSize:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;
			
		case kAudioCodecPropertyFormatInfo:
			outPropertyDataSize = sizeof(AudioFormatInfo);
			outWritable = false;
			break;
			
		default:
			ACBaseCodec::GetPropertyInfo(inPropertyID, outPropertyDataSize, outWritable);
			break;
	}
}

void FFissionCodec::GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData)
{
	switch(inPropertyID) {
		case kAudioCodecPropertyManufacturerCFString:
			if (ioPropertyDataSize != sizeof(CFStringRef))
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			break; 
			
		case kAudioCodecPropertyMaximumPacketByteSize:
		case kAudioCodecPropertyRequiresPacketDescription:
		case kAudioCodecPropertyHasVariablePacketByteSizes:
		case kAudioCodecPropertyPacketFrameSize:
			if(ioPropertyDataSize != sizeof(UInt32))
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			break;
	}
	
	switch (inPropertyID) {
		case kAudioCodecPropertyManufacturerCFString:
			*(CFStringRef*)outPropertyData = CFSTR("Perian");
			break;
			
		case kAudioCodecPropertyNameCFString:
		case kAudioCodecPropertyFormatCFString:
			// FIXME read our Rez strings here
			CODEC_THROW(kAudioCodecUnknownPropertyError);
			break;

		case kAudioCodecPropertyMaximumPacketByteSize:
			if (avContext && avContext->block_align)
				*reinterpret_cast<UInt32*>(outPropertyData) = avContext->block_align;
			else
				*reinterpret_cast<UInt32*>(outPropertyData) = mInputFormat.mBytesPerPacket;
			break;
			
		case kAudioCodecPropertyRequiresPacketDescription:
			*reinterpret_cast<UInt32*>(outPropertyData) = false;
			break;
			
		case kAudioCodecPropertyHasVariablePacketByteSizes:
			*reinterpret_cast<UInt32*>(outPropertyData) = false;
			break;
			
		case kAudioCodecPropertyPacketFrameSize:
			if (avContext && avContext->frame_size)
				*reinterpret_cast<UInt32*>(outPropertyData) = avContext->frame_size;
			else if (mInputFormat.mFramesPerPacket)
				*reinterpret_cast<UInt32*>(outPropertyData) = mInputFormat.mFramesPerPacket;
			else
				*reinterpret_cast<UInt32*>(outPropertyData) = 1;
			break;
			
		default:
			ACBaseCodec::GetProperty(inPropertyID, ioPropertyDataSize, outPropertyData);
	}
}
