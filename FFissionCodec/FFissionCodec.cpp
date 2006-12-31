/*
 *  FFissionCodec.cpp
 *
 *  Copyright (c) 2006  David Conrad
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; 
 *  version 2.1 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "avcodec.h"
#include "FFissionCodec.h"

extern "C" void initLib();

FFissionCodec::FFissionCodec(UInt32 inInputBufferByteSize) : ACSimpleCodec(inInputBufferByteSize)
{
	initLib();
	
	avContext = avcodec_alloc_context();
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
	
	ACSimpleCodec::Initialize(inInputFormat, inOutputFormat, inMagicCookie, inMagicCookieByteSize);
}

void FFissionCodec::GetPropertyInfo(AudioCodecPropertyID inPropertyID, UInt32& outPropertyDataSize, bool& outWritable) {
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
			
		default:
			ACSimpleCodec::GetPropertyInfo(inPropertyID, outPropertyDataSize, outWritable);
			break;
	}
}

void FFissionCodec::GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData) {	
	switch(inPropertyID) {
		case kAudioCodecPropertyManufacturerCFString:
			if (ioPropertyDataSize != sizeof(CFStringRef)) {
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			
			CFStringRef name = CFCopyLocalizedStringFromTableInBundle(CFSTR("Perian Project"), CFSTR("CodecNames"), GetCodecBundle(), CFSTR(""));
			*(CFStringRef*)outPropertyData = name;
			break; 
			
		case kAudioCodecPropertyMaximumPacketByteSize:
			if(ioPropertyDataSize == sizeof(UInt32)) {
				if (avContext)
					*reinterpret_cast<UInt32*>(outPropertyData) = avContext->block_align;
				else
					*reinterpret_cast<UInt32*>(outPropertyData) = 0;
            } else {
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
            break;
			
        case kAudioCodecPropertyRequiresPacketDescription:
			if(ioPropertyDataSize == sizeof(UInt32)) {
                *reinterpret_cast<UInt32*>(outPropertyData) = false; 
            } else {
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
            break;
			
        case kAudioCodecPropertyHasVariablePacketByteSizes:
			if(ioPropertyDataSize == sizeof(UInt32)) {
                *reinterpret_cast<UInt32*>(outPropertyData) = false;
            } else {
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
            break;
			
		case kAudioCodecPropertyPacketFrameSize:
			if(ioPropertyDataSize == sizeof(UInt32)) {
				if (avContext)
					*reinterpret_cast<UInt32*>(outPropertyData) = avContext->frame_size;
				else
					*reinterpret_cast<UInt32*>(outPropertyData) = 0;
            } else {
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;
			
		default:
			ACSimpleCodec::GetProperty(inPropertyID, ioPropertyDataSize, outPropertyData);
	}
}
