/*
 * CompressAudioCodec.cpp
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

#include "CompressAudioCodec.h"
#include "ACCodecDispatch.h"
#include "CodecIDs.h"
#include "CompressCodecUtils.h"

static const OSType kAllInputFormats[] = 
{
	kCompressedAC3,
	kCompressedMP3,
	kCompressedDTS,
	0,
};

CompressAudioCodec::CompressAudioCodec(UInt32 inInputBufferByteSize) : FFissionCodec(0)
{
	UInt32 kIntPCMOutFormatFlag = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagsNativeEndian | kLinearPCMFormatFlagIsPacked;
	
	for (int i = 0; kAllInputFormats[i] != CODEC_ID_NONE; i++) {
		CAStreamBasicDescription theInputFormat(kAudioStreamAnyRate, kAllInputFormats[i], 0, 1, 0, 0, 0, 0);
		AddInputFormat(theInputFormat);
	}
	
	// libavcodec outputs 16-bit native-endian integer pcm, so why do conversions ourselves?
	CAStreamBasicDescription theOutputFormat(kAudioStreamAnyRate, kAudioFormatLinearPCM, 0, 1, 0, 0, 16, kIntPCMOutFormatFlag);
	AddOutputFormat(theOutputFormat);
	strippedHeader = NULL;
	strippedHeaderSize = 0;
	innerCookie = NULL;
	innerCookieSize = 0;
	actualUnit = NULL;
}

CompressAudioCodec::~CompressAudioCodec()
{
	if(strippedHeader)
		delete[] strippedHeader;
	if(innerCookie)
		delete[] innerCookie;
	if(actualUnit)
		CloseComponent(actualUnit);
}

UInt32 CompressAudioCodec::ParseCookieAtom(const uint8_t* inAtom, UInt32 inAtomMaxSize)
{
	if(inAtomMaxSize < 8)
		//Invalid; atom must be at least 8 bytes.
		return inAtomMaxSize;
	const UInt32 *atomElements = reinterpret_cast<const UInt32 *>(inAtom);
	UInt32 atomSize = EndianU32_BtoN(atomElements[0]);
	UInt32 atomType = EndianU32_BtoN(atomElements[1]);
	
	if(atomSize > inAtomMaxSize)
		return inAtomMaxSize;
	
	switch (atomType) {
		case 'CpSt': {
			//Stripped header
			UInt32 headerSize = atomSize - 8;
			if(headerSize == 0)
				break;
			strippedHeaderSize = headerSize;
			strippedHeader = new Byte[headerSize];
			memcpy(strippedHeader, inAtom+8, headerSize);
		}
			break;
		case 'CpCk': {
			//Real cookie
			UInt32 cookieSize = atomSize - 8;
			if(cookieSize == 0)
				break;
			innerCookieSize = cookieSize;
			innerCookie = new Byte[cookieSize];
			memcpy(innerCookie, inAtom+8, cookieSize);
		}
			break;
		default:
			break;
	}
	
	return atomSize;
}

void CompressAudioCodec::ParseCookie(const uint8_t* inMagicCookie, UInt32 inMagicCookieByteSize)
{
	UInt32 offset = 0;
	while(offset < inMagicCookieByteSize) {
		offset += ParseCookieAtom(inMagicCookie + offset, inMagicCookieByteSize - offset);
	}
}

void CompressAudioCodec::Initialize(const AudioStreamBasicDescription* inInputFormat, const AudioStreamBasicDescription* inOutputFormat, const void* inMagicCookie, UInt32 inMagicCookieByteSize)
{
	if(inInputFormat)
	{
		OSType original = originalStreamFourCC(inInputFormat->mFormatID);
		if(original == 0)
			CODEC_THROW(kAudioCodecUnsupportedFormatError);
		
		ComponentDescription desc;
		memset(&desc, 0, sizeof(ComponentDescription));
		desc.componentType = kAudioDecoderComponentType;
		desc.componentSubType = original;
		Component component = FindNextComponent(NULL, &desc);
		if(component != NULL)
		{
			ComponentResult err = OpenAComponent(component, &actualUnit);
			AudioStreamBasicDescription input = *inInputFormat;
			input.mFormatID = original;
			err = AudioCodecInitialize(actualUnit, &input, inOutputFormat, innerCookie, innerCookieSize);
		}
	}
	if(inMagicCookie)
		SetMagicCookie(inMagicCookie, inMagicCookieByteSize);
	FFissionCodec::Initialize(inInputFormat, inOutputFormat, inMagicCookie, inMagicCookieByteSize);
}

void CompressAudioCodec::Uninitialize()
{
	FFissionCodec::Uninitialize();
	if(actualUnit)
		AudioCodecUninitialize(actualUnit);
}

void CompressAudioCodec::Reset()
{
	if(actualUnit)
		AudioCodecReset(actualUnit);
}

void CompressAudioCodec::GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData)
{
	if(actualUnit)
	{
		AudioCodecGetProperty(actualUnit, inPropertyID, &ioPropertyDataSize, outPropertyData);
		switch (inPropertyID) {
			case kAudioCodecPropertyCurrentInputFormat:
			case kAudioCodecPropertySupportedInputFormats:
			case kAudioCodecInputFormatsForOutputFormat:
			{
				int formatCount = ioPropertyDataSize / sizeof(AudioStreamBasicDescription);
				AudioStreamBasicDescription *formats = reinterpret_cast<AudioStreamBasicDescription*>(outPropertyData);
				for(int i=0; i<formatCount; i++)
				{
					formats[i].mFormatID = mInputFormat.mFormatID;
				}
			}
				break;
			default:
				break;
		}
	}
	else
		FFissionCodec::GetProperty(inPropertyID, ioPropertyDataSize, outPropertyData);
}

void CompressAudioCodec::SetMagicCookie(const void* inMagicCookieData, UInt32 inMagicCookieDataByteSize)
{
	ParseCookie(static_cast<const uint8_t *> (inMagicCookieData), inMagicCookieDataByteSize);
	FFissionCodec::SetMagicCookie(inMagicCookieData, inMagicCookieDataByteSize);
	if(actualUnit)
	{
		OSType original = originalStreamFourCC(mInputFormat.mFormatID);
		AudioStreamBasicDescription input = mInputFormat;
		input.mFormatID = original;
		AudioStreamBasicDescription output = mOutputFormat;
		AudioCodecInitialize(actualUnit, &input, &output, innerCookie, innerCookieSize);
	}
}

void CompressAudioCodec::SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat)
{
	FFissionCodec::SetCurrentInputFormat(inInputFormat);
}

void CompressAudioCodec::SetCurrentOutputFormat(const AudioStreamBasicDescription& inOutputFormat)
{
	FFissionCodec::SetCurrentOutputFormat(inOutputFormat);
}

void CompressAudioCodec::AppendInputData(const void* inInputData, UInt32& ioInputDataByteSize, UInt32& ioNumberPackets, const AudioStreamPacketDescription* inPacketDescription)
{
	if(inPacketDescription && ioNumberPackets && actualUnit)
	{
		UInt32 totalSize = 0;
		for(int i=0; i<ioNumberPackets; i++)
			totalSize += inPacketDescription[i].mDataByteSize;
		
		Byte *newInputData = new Byte[totalSize + strippedHeaderSize*ioNumberPackets];
		AudioStreamPacketDescription *newPackets = new AudioStreamPacketDescription[ioNumberPackets];
		UInt32 offset = 0;
		for(int i=0; i<ioNumberPackets; i++)
		{
			memcpy(newInputData + offset, strippedHeader, strippedHeaderSize);
			UInt32 packetSize = inPacketDescription[i].mDataByteSize;
			memcpy(newInputData + offset + strippedHeaderSize, static_cast<const Byte *>(inInputData) + inPacketDescription[i].mStartOffset, packetSize);
			newPackets[i].mDataByteSize = packetSize + strippedHeaderSize;
			newPackets[i].mStartOffset = offset;
			newPackets[i].mVariableFramesInPacket = inPacketDescription[i].mVariableFramesInPacket;
			offset += packetSize + strippedHeaderSize;
		}
		AudioCodecAppendInputData(actualUnit, newInputData, &offset, &ioNumberPackets, newPackets);
		AudioStreamPacketDescription lastPacket = inPacketDescription[ioNumberPackets-1];
		ioInputDataByteSize = lastPacket.mStartOffset + lastPacket.mDataByteSize;
		delete[] newInputData;
		delete[] newPackets;
	}
}

UInt32 CompressAudioCodec::ProduceOutputPackets(void* outOutputData, UInt32& ioOutputDataByteSize, UInt32& ioNumberPackets, AudioStreamPacketDescription* outPacketDescription)
{
	UInt32 status = noErr;
	if(actualUnit)
		AudioCodecProduceOutputPackets(actualUnit, outOutputData, &ioOutputDataByteSize, &ioNumberPackets, outPacketDescription, &status);
	return status;
}

extern "C"
ComponentResult	CompressAudioDecoderEntry(ComponentParameters* inParameters, CompressAudioCodec* inThis)
{
	return ACCodecDispatch(inParameters, inThis);
}