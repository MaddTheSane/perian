/*
 *  FFissionDecoder.cpp
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

#include "FFissionDecoder.h"
#include "ACCodecDispatch.h"
#include "FFusionCodec.h"
#include "Codecprintf.h"
#include "CodecIDs.h"

struct CodecPair {
	OSType mFormatID;
	CodecID codecID;
};

static const CodecPair kAllInputFormats[] = 
{
	{ kAudioFormatWMA1MS, CODEC_ID_WMAV1 },
	{ kAudioFormatWMA2MS, CODEC_ID_WMAV2 },
	{ 0, CODEC_ID_NONE }
};

static CodecID GetCodecID(OSType formatID)
{
	for (int i = 0; kAllInputFormats[i].codecID != CODEC_ID_NONE; i++) {
		if (kAllInputFormats[i].mFormatID == formatID)
			return kAllInputFormats[i].codecID;
	}
	return CODEC_ID_NONE;
}


FFissionDecoder::FFissionDecoder(UInt32 inInputBufferByteSize) : FFissionCodec(inInputBufferByteSize)
{
	kIntPCMOutFormatFlag = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagsNativeEndian | kLinearPCMFormatFlagIsPacked;
	magicCookie = NULL;
	magicCookieSize = 0;
	
	for (int i = 0; i < kAllInputFormats[i].codecID != CODEC_ID_NONE; i++) {
		CAStreamBasicDescription theInputFormat(kAudioStreamAnyRate, kAllInputFormats[i].mFormatID, 0, 1, 0, 0, 0, 0);
		AddInputFormat(theInputFormat);
	}
	
	// libavcodec outputs 16-bit native-endian integer pcm, so why do conversions ourselves?
	CAStreamBasicDescription theOutputFormat(kAudioStreamAnyRate, kAudioFormatLinearPCM, 0, 1, 0, 0, 16, kIntPCMOutFormatFlag);
	AddOutputFormat(theOutputFormat);
}

FFissionDecoder::~FFissionDecoder()
{
	if (magicCookie)
		delete[] magicCookie;
}

void FFissionDecoder::Initialize(const AudioStreamBasicDescription* inInputFormat, const AudioStreamBasicDescription* inOutputFormat, const void* inMagicCookie, UInt32 inMagicCookieByteSize)
{
	if (inMagicCookieByteSize > 0)
		SetMagicCookie(inMagicCookie, inMagicCookieByteSize);
	
	FFissionCodec::Initialize(inInputFormat, inOutputFormat, inMagicCookie, inMagicCookieByteSize);
}

void FFissionDecoder::SetMagicCookie(const void* inMagicCookieData, UInt32 inMagicCookieDataByteSize)
{
	if (magicCookie)
		delete[] magicCookie;
	
	magicCookieSize = inMagicCookieDataByteSize;
	
	if (magicCookieSize > 0) {
		magicCookie = new Byte[magicCookieSize + FF_INPUT_BUFFER_PADDING_SIZE];
		memcpy(magicCookie, inMagicCookieData, magicCookieSize);
	} else
		magicCookie = NULL;
	
	FFissionCodec::SetMagicCookie(inMagicCookieData, inMagicCookieDataByteSize);
}

void FFissionDecoder::SetupExtradata(OSType formatID)
{
	if (!magicCookie) return;
	
	switch (formatID) {
		case kAudioFormatWMA1MS:
		case kAudioFormatWMA2MS:
			if (magicCookieSize < 12 + 18 + 8 + 8)
				return;
			
			avContext->extradata = magicCookie + 12 + 18 + 8;
			avContext->extradata_size = magicCookieSize - 12 - 18 - 8 - 8;
			break;
			
		default:
			return;
	}
	
	// this is safe because we always allocate this amount of additional memory for our copy of the magic cookie
	memset(avContext->extradata + avContext->extradata_size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
}

void FFissionDecoder::GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData) {
	switch (inPropertyID) {
		case kAudioCodecPropertyNameCFString:
			if (ioPropertyDataSize != sizeof(CFStringRef))
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			break;
	}
	switch (inPropertyID) {
		case kAudioCodecPropertyNameCFString:
			CFStringRef name = CFCopyLocalizedStringFromTableInBundle(CFSTR("Perian FFmpeg audio decoder"), CFSTR("CodecNames"), GetCodecBundle(), CFSTR(""));
			*(CFStringRef*)outPropertyData = name;
			break; 
			
		default:
			FFissionCodec::GetProperty(inPropertyID, ioPropertyDataSize, outPropertyData);
	}
}

void FFissionDecoder::SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat)
{
	if(mIsInitialized) {
		CODEC_THROW(kAudioCodecStateError);
	}
	
	if (avCodec) {
		avcodec_close(avContext);
		avCodec = NULL;
	}
	
	CodecID codecID = GetCodecID(inInputFormat.mFormatID);
	
	avCodec = avcodec_find_decoder(codecID);
	
	// check to make sure the input format is legal
	if (avCodec == NULL) {
		Codecprintf(NULL, "Unsupported input format id %4.4s\n", &inInputFormat.mFormatID);
		CODEC_THROW(kAudioCodecUnsupportedFormatError);
	}
	
	avcodec_get_context_defaults(avContext);
	
	avContext->sample_rate = inInputFormat.mSampleRate;
	avContext->channels = inInputFormat.mChannelsPerFrame;
	avContext->block_align = inInputFormat.mBytesPerPacket;
	avContext->frame_size = inInputFormat.mFramesPerPacket;
	avContext->bits_per_sample = inInputFormat.mBitsPerChannel;
	
	if (avContext->sample_rate == 0) {
		Codecprintf(NULL, "Invalid sample rate %d\n", avContext->sample_rate);
		avCodec = NULL;
		return;
	}
	
	if (magicCookie) {
		SetupExtradata(inInputFormat.mFormatID);
	}
	
	if (avcodec_open(avContext, avCodec)) {
		Codecprintf(NULL, "error opening audio avcodec\n");
		
		avCodec = NULL;
		CODEC_THROW(kAudioCodecIllegalOperationError);
	}
	
	// tell our base class about the new format
	FFissionCodec::SetCurrentInputFormat(inInputFormat);
}

void FFissionDecoder::SetCurrentOutputFormat(const AudioStreamBasicDescription& inOutputFormat)
{
	if(mIsInitialized) {
		CODEC_THROW(kAudioCodecStateError);
	}
	
	//	check to make sure the output format is legal
	if (inOutputFormat.mFormatID != kAudioFormatLinearPCM ||
		inOutputFormat.mFormatFlags != kIntPCMOutFormatFlag || 
		inOutputFormat.mBitsPerChannel != 16)
	{
		Codecprintf(NULL, "We only support 16 bit native endian signed integer for output\n");
		CODEC_THROW(kAudioCodecUnsupportedFormatError);
	}
	
	//	tell our base class about the new format
	FFissionCodec::SetCurrentOutputFormat(inOutputFormat);
}

UInt32 FFissionDecoder::ProduceOutputPackets(void* outOutputData,
											  UInt32& ioOutputDataByteSize,	// number of bytes written to outOutputData
											  UInt32& ioNumberPackets, 
											  AudioStreamPacketDescription* outPacketDescription)
{
	// setup the return value, by assuming that everything is going to work
	UInt32 theAnswer = kAudioCodecProduceOutputPacketSuccess;
	
	if (!mIsInitialized)
		CODEC_THROW(kAudioCodecStateError);
	
	// clamp the number of packets to produce based on what is available in the input buffer
	UInt32 inputPacketSize = avContext->channels * avContext->block_align;
	UInt32 numberOfInputPackets = GetUsedInputBufferByteSize() / inputPacketSize;
	
	if (ioNumberPackets < numberOfInputPackets)
	{
		numberOfInputPackets = ioNumberPackets;
	}
	else if (ioNumberPackets > numberOfInputPackets)
	{
		ioNumberPackets = numberOfInputPackets;
		
		//	this also means we need more input to satisfy the request so set the return value
		theAnswer = kAudioCodecProduceOutputPacketNeedsMoreInputData;
	}
	
	UInt32 inputByteSize = numberOfInputPackets * inputPacketSize;
	
	if(ioNumberPackets > 0)
	{
		// make sure that there is enough space in the output buffer for the encoded data
		// it is an error to ask for more output than you pass in buffer space for
		UInt32 theOutputByteSize = ioNumberPackets * avContext->frame_size * mOutputFormat.mBytesPerFrame;
		ThrowIf(ioOutputDataByteSize < theOutputByteSize, static_cast<ComponentResult>(kAudioCodecNotEnoughBufferSpaceError), "ACAppleIMA4Decoder::ProduceOutputPackets: not enough space in the output buffer");
		
		// set the return value
		ioOutputDataByteSize = theOutputByteSize;
		
		// decode the input data for each channel
		Byte* theInputData = GetBytes(inputByteSize);
		
		int out_size = 0;
		SInt16* theOutputData = reinterpret_cast<SInt16*>(outOutputData);
		
		int len = avcodec_decode_audio(avContext, theOutputData, &out_size, theInputData, inputByteSize);
		ioOutputDataByteSize = out_size;
		
		ConsumeInputData(len);
	}
	else
	{
		// set the return value since we're not actually doing any work
		ioOutputDataByteSize = 0;
	}
	
	if((theAnswer == kAudioCodecProduceOutputPacketSuccess) && (GetUsedInputBufferByteSize() >= inputPacketSize))
	{
		// we satisfied the request, and there's at least one more full packet of data we can decode
		// so set the return value
		theAnswer = kAudioCodecProduceOutputPacketSuccessHasMore;
	}
	
	return theAnswer;
}

UInt32 FFissionDecoder::GetVersion() const
{
	return kFFusionCodecVersion;
}

extern "C"
ComponentResult	FFissionDecoderEntry(ComponentParameters* inParameters, FFissionDecoder* inThis)
{
	return ACCodecDispatch(inParameters, inThis);
}
