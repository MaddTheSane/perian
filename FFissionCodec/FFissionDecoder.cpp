/*
 * FFissionDecoder.cpp
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

#include <AudioToolbox/AudioToolbox.h>
#include <libavcodec/dca.h>

#include "ComponentBase.h"
#include "FFissionDecoder.h"
#include "PerianResourceIDs.h"
#include "Codecprintf.h"
#include "FFmpegUtils.h"
#include "CodecIDs.h"

#define MY_APP_DOMAIN CFSTR("org.perian.Perian")
#define PASSTHROUGH_KEY CFSTR("attemptDTSPassthrough")

typedef struct CookieAtomHeader {
    long           size;
    long           type;
    unsigned char  data[1];
} CookieAtomHeader;

struct WaveFormatEx {
	uint16_t wFormatTag;
	uint16_t nChannels;
	uint32_t nSamplesPerSec;
	uint32_t nAvgBytesPerSec;
	uint16_t nBlockAlign;
	uint16_t wBitsPerSample;
	uint16_t cbSize;
} __attribute__((packed));

static OSType kAllInputFormats[] = {
	kAudioFormatWMA1MS,
	kAudioFormatWMA2MS,
	kAudioFormatFlashADPCM,
	kAudioFormatXiphVorbis,
	kAudioFormatMPEGLayer1,
	kAudioFormatMPEGLayer2,
	'ms\0\0' + 0x50,
	kAudioFormatDTS,
	kAudioFormatNellymoser,
	kAudioFormatTTA,
	0
};

static const UInt32 kIntPCMOutFormatFlag = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagsNativeEndian | kLinearPCMFormatFlagIsPacked;

//CA tends to not set the magic cookie immediately, but we need it to open most codecs
//so check this to not open the codec until it's possible
static bool CodecRequiresExtradata(OSType formatID)
{
	switch (formatID) {
		case kAudioFormatWMA1MS:
		case kAudioFormatWMA2MS:
		case kAudioFormatXiphVorbis:
		case kAudioFormatTTA:
			return true;
	}
	
	return false;
}

// parse waveformatex in extradata back into the AVCodecContext
// block_align is then put into the ASBD when clients call FormatInfo (well, hopefully)
static void ParseWaveFormat(const WaveFormatEx *wEx, AVCodecContext *avContext)
{
	avContext->block_align = EndianU16_LtoN(wEx->nBlockAlign);
	avContext->bit_rate = EndianU32_LtoN(wEx->nAvgBytesPerSec) * 8;
}

FFissionDecoder::FFissionDecoder(AudioComponentInstance inInstance) : FFissionCodec(inInstance)
{
	magicCookie = NULL;
	magicCookieSize = 0;
	
	for (int i = 0; kAllInputFormats[i] != 0; i++) {
		CAStreamBasicDescription theInputFormat(kAudioStreamAnyRate, kAllInputFormats[i], 0, 1, 0, 0, 0, 0);
		AddInputFormat(theInputFormat);
	}
	
	// libavcodec outputs 16-bit native-endian integer pcm, so why do conversions ourselves?
	CAStreamBasicDescription theOutputFormat(kAudioStreamAnyRate, kAudioFormatLinearPCM, 0, 1, 0, 0, 16, kIntPCMOutFormatFlag);
	AddOutputFormat(theOutputFormat);
	
	inputBuffer.Initialize(4096); /* allocation hint, it will resize */
	outBufUsed = 0;
	outBufSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
	outputBuffer = new Byte[outBufSize];
	
	CFStringRef myApp = MY_APP_DOMAIN;
	CFPreferencesAppSynchronize(myApp);
	CFTypeRef pass = CFPreferencesCopyAppValue(PASSTHROUGH_KEY, myApp);
	if(pass != NULL)
	{
		CFTypeID type = CFGetTypeID(pass);
		if(type == CFStringGetTypeID())
			dtsPassthrough = CFStringGetIntValue((CFStringRef)pass);
		else if(type == CFNumberGetTypeID())
			CFNumberGetValue((CFNumberRef)pass, kCFNumberIntType, &dtsPassthrough);
		else
			dtsPassthrough = 0;
		CFRelease(pass);
	}
	else
		dtsPassthrough = 0;
	
	int initialMap[6] = {0, 1, 2, 3, 4, 5};
	memcpy(fullChannelMap, initialMap, sizeof(initialMap));
	
	// FIXME avcodec_get_context_defaults3
	avcodec_get_context_defaults2(avContext, AVMEDIA_TYPE_AUDIO);
	parser = NULL;
}

FFissionDecoder::~FFissionDecoder()
{
	CloseAVCodec();
	
	if (magicCookie)
		delete[] magicCookie;
	
	delete[] outputBuffer;
}

void FFissionDecoder::Initialize(const AudioStreamBasicDescription* inInputFormat, const AudioStreamBasicDescription* inOutputFormat, const void* inMagicCookie, UInt32 inMagicCookieByteSize)
{
	if (inMagicCookie)
		SetMagicCookie(inMagicCookie, inMagicCookieByteSize);
		
	FFissionCodec::Initialize(inInputFormat, inOutputFormat, inMagicCookie, inMagicCookieByteSize);
	
	OpenAVCodec();	
}

void FFissionDecoder::Uninitialize()
{
	outBufUsed = 0;
	inputBuffer.Zap(inputBuffer.GetDataAvailable());
	
	FFissionCodec::Uninitialize();
}

void FFissionDecoder::Reset()
{
	outBufUsed = 0;
	inputBuffer.Zap(inputBuffer.GetDataAvailable());
	
	FFissionCodec::Reset();
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
	
	CloseAVCodec();
	
	avContext->extradata_size = 0;
	avContext->extradata = NULL;
}

void FFissionDecoder::SetupExtradata()
{
	if (!magicCookie) return;
		
	switch (mInputFormat.mFormatID) {
		case kAudioFormatWMA1MS:
		case kAudioFormatWMA2MS:
		case kAudioFormatTTA:
			if (magicCookieSize < 12 + 18 + 8 + 8)
				return;
			
			ParseWaveFormat((WaveFormatEx*)(magicCookie + 12 + 8), avContext);
						
			avContext->extradata = magicCookie + 12 + 18 + 8;
			avContext->extradata_size = magicCookieSize - 12 - 18 - 8 - 8;
			break;
			
		case kAudioFormatXiphVorbis:
			if (!avContext->extradata) {
				avContext->extradata_size = ConvertXiphVorbisCookie();
				avContext->extradata = magicCookie;
			}
			break;
			
		default:
			return;
	}
	
	// this is safe because we always allocate this amount of additional memory for our copy of the magic cookie
	memset(avContext->extradata + avContext->extradata_size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
}

int FFissionDecoder::ConvertXiphVorbisCookie()
{
	Byte *ptr = magicCookie;
	Byte *cend = magicCookie + magicCookieSize;
	Byte *headerData[3] = {NULL};
	int headerSize[3] = {0};
	
	while (ptr < cend) {
		CookieAtomHeader *aheader = reinterpret_cast<CookieAtomHeader *>(ptr);
		int size = EndianU32_BtoN(aheader->size);
		ptr += size;
		if (ptr > cend || size <= 0)
			break;
		
		switch(EndianS32_BtoN(aheader->type)) {
			case kCookieTypeVorbisHeader:
				headerData[0] = aheader->data;
				headerSize[0] = size - 8;
				break;
				
			case kCookieTypeVorbisComments:
				headerData[1] = aheader->data;
				headerSize[1] = size - 8;
				break;
				
			case kCookieTypeVorbisCodebooks:
				headerData[2] = aheader->data;
				headerSize[2] = size - 8;
				break;
		}
	}
	
    if (headerSize[0] <= 0 || headerSize[1] <= 0 || headerSize[2] <= 0) {
		Codecprintf(NULL, "Invalid Vorbis extradata\n");
		return 0;
	}
	
	int len = headerSize[0] + headerSize[1] + headerSize[2];
	Byte *newCookie = new Byte[len + len/255 + 64 + FF_INPUT_BUFFER_PADDING_SIZE];
	ptr = newCookie;
	
	ptr[0] = 2;		// number of packets minus 1
	int offset = 1;
	offset += av_xiphlacing(&ptr[offset], headerSize[0]);
	offset += av_xiphlacing(&ptr[offset], headerSize[1]);
	for (int i = 0; i < 3; i++) {
		memcpy(&ptr[offset], headerData[i], headerSize[i]);
		offset += headerSize[i];
	}
	
	delete[] magicCookie;
	magicCookie = newCookie;
	magicCookieSize = offset;
	
	return offset;
}

void FFissionDecoder::GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData) {
	switch (inPropertyID) {
		case kAudioCodecPropertyNameCFString:
			if (ioPropertyDataSize != sizeof(CFStringRef))
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			break;
			
		case kAudioCodecPropertyInputBufferSize:
		case kAudioCodecPropertyUsedInputBufferSize:
			if (ioPropertyDataSize != sizeof(UInt32))
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			break;
			
		case kAudioCodecPropertyFormatInfo:
			if (ioPropertyDataSize != sizeof(AudioFormatInfo))
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			break;
	}
	
	switch (inPropertyID) {
		case kAudioCodecPropertyNameCFString:
			*(CFStringRef*)outPropertyData = CFCopyLocalizedStringFromTableInBundle(CFSTR("Perian FFmpeg audio decoder"), CFSTR("CodecNames"), GetCodecBundle(), CFSTR(""));
			break;
			
		case kAudioCodecPropertyFormatInfo:
		{
			AudioFormatInfo *info = (AudioFormatInfo*)outPropertyData;
			FFissionDecoder *dec = new FFissionDecoder(mComponentInstance); // XXX
			
			// We need to fake an output format, doesn't matter what it says as long as it inits
			static const AudioStreamBasicDescription DefaultOutputASBD =
			{ info->mASBD.mSampleRate, kAudioFormatLinearPCM, kIntPCMOutFormatFlag, 4, 1, 4, info->mASBD.mChannelsPerFrame, 16 };
			
			try {
				dec->Initialize(&info->mASBD, &DefaultOutputASBD, info->mMagicCookie, info->mMagicCookieSize);
				
				AudioStreamBasicDescription asbd = {};
				FFAVCodecContextToASBD(dec->avContext, &asbd);
				
				CAStreamBasicDescription::FillOutFormat(info->mASBD, asbd);
			} catch (ComponentResult r) {
				delete dec;
				CODEC_THROW(r);
			}
			
			delete dec;
		}
			break;
			
		default:
			FFissionCodec::GetProperty(inPropertyID, ioPropertyDataSize, outPropertyData);
	}
}

void FFissionDecoder::ReallocateInputBuffer(UInt32 inInputBufferByteSize)
{
	inputBuffer.Reset();
	inputBuffer.Reallocate(inInputBufferByteSize);
}

UInt32 FFissionDecoder::GetInputBufferByteSize() const
{
	return inputBuffer.GetBufferByteSize();
}

UInt32 FFissionDecoder::GetUsedInputBufferByteSize() const
{
	return inputBuffer.GetDataAvailable();
}

void FFissionDecoder::SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat)
{
	if(mIsInitialized) {
		CODEC_THROW(kAudioCodecStateError);
	}
	
	CloseAVCodec();

	CodecID codecID = FFFourCCToCodecID(inInputFormat.mFormatID);
	
	// check to make sure the input format is legal
	if (avcodec_find_decoder(codecID) == NULL) {
		Codecprintf(NULL, "Unsupported input format id %s\n", FourCCString(inInputFormat.mFormatID));
		CODEC_THROW(kAudioCodecUnsupportedFormatError);
	}
	
	// tell our base class about the new format
	FFissionCodec::SetCurrentInputFormat(inInputFormat);	
}
	
void FFissionDecoder::OpenAVCodec()
{
	if (!mIsInitialized)
		CODEC_THROW(kAudioCodecStateError);
		
	// it's initialized, but not enough. wait for more to be set
	// note it should never get a magicCookie but no input format
	if (!magicCookie && CodecRequiresExtradata(mInputFormat.mFormatID))
		return;
	
	CloseAVCodec();
	
	CodecID codecID = FFFourCCToCodecID(mInputFormat.mFormatID);
	avCodec = avcodec_find_decoder(codecID);
	
	FFASBDToAVCodecContext(&mInputFormat, avContext);
	avContext->codec_id = codecID;

	if (avContext->sample_rate == 0) {
		Codecprintf(NULL, "Invalid sample rate %d\n", avContext->sample_rate);
		avCodec = NULL;
		CODEC_THROW(kAudioCodecUnsupportedFormatError);
	}
	
	if (magicCookie)
		SetupExtradata();
	
	if (avcodec_open2(avContext, avCodec, NULL)) {
		Codecprintf(NULL, "error opening audio avcodec\n");
		avCodec = NULL;
		CODEC_THROW(kAudioCodecUnsupportedFormatError);
	}
	if(mInputFormat.mFormatID != kAudioFormatDTS)
		dtsPassthrough = 0;
	else
	{
		switch (mInputFormat.mChannelsPerFrame) {
			case 3:
			case 6:
			{
				int chanMap[6] = {1, 2, 0, 5, 3, 4};//L R C -> C L R  and  L R C LFE Ls Rs -> C L R Ls Rs LFE
				memcpy(fullChannelMap, chanMap, sizeof(chanMap));
				break;
			}
			case 4:
			case 5:
			{
				int chanMap[6] = {1, 2, 0, 3, 4, 5};//L R C Cs -> C L R Cs  and  L R C Ls Rs -> C L R Ls Rs
				memcpy(fullChannelMap, chanMap, sizeof(chanMap));
				break;
			}
			default:
				break;
		}
	}
	parser = av_parser_init(codecID);
}

void FFissionDecoder::CloseAVCodec()
{
	if (avCodec) {
		avcodec_close(avContext);
		avCodec = NULL;
	}
	if (parser) {
		av_parser_close(parser);
		parser = NULL;
	}
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

void FFissionDecoder::AppendInputData(const void* inInputData, UInt32& ioInputDataByteSize, 
									  UInt32& ioNumberPackets, const AudioStreamPacketDescription* inPacketDescription)
{
	const Byte *inData = (const Byte *)inInputData;
	
	if (inPacketDescription && ioNumberPackets) {
		for (int i = 0; i < ioNumberPackets; i++) {
			UInt32 packetSize = inPacketDescription[i].mDataByteSize;
			inputBuffer.In(inData + inPacketDescription[i].mStartOffset, packetSize);
		}
	} else if (mInputFormat.mBytesPerPacket != 0) {
		// no packet description, assume cbr
		UInt32 amountToCopy = FFMIN(mInputFormat.mBytesPerPacket * ioNumberPackets, ioInputDataByteSize);
		UInt32 numPackets = amountToCopy / mInputFormat.mBytesPerPacket;
		
		ioInputDataByteSize = amountToCopy;
		ioNumberPackets = numPackets;
		
		for (int i = 0; i < numPackets; i++) {
			UInt32 packetSize = mInputFormat.mBytesPerPacket;
			inputBuffer.In(inData, packetSize);
			inData += mInputFormat.mBytesPerPacket;
		}
	} else {
		// XiphQT throws this in this situation (we need packet descriptions, but don't get them)
		// is there a better error to throw?
		CODEC_THROW(kAudioCodecNotEnoughBufferSpaceError);
	}
}

#define AV_RL16(x) EndianU16_LtoN(*(uint16_t *)(x))
#define AV_RB16(x) EndianU16_BtoN(*(uint16_t *)(x))

int produceDTSPassthroughPackets(Byte *outputBuffer, int *outBufUsed, uint8_t *packet, int packetSize, int channelCount, int bigEndian)
{
	if(packetSize < 96)
		return 0;
	
	static const uint8_t p_sync_le[6] = { 0x72, 0xF8, 0x1F, 0x4E, 0x00, 0x00 };
	static const uint8_t p_sync_be[6] = { 0xF8, 0x72, 0x4E, 0x1F, 0x00, 0x00 };
	
	uint32_t mrk = AV_RB16(packet) << 16 | AV_RB16(packet + 2);
	unsigned int frameSize = 0;
	unsigned int blockCount = 0;

	switch (mrk) {
		case DCA_MARKER_RAW_BE:
			blockCount = (AV_RB16(packet + 4) >> 2) & 0x7f;
			frameSize = (AV_RB16(packet + 4) & 0x3) << 12 | ((AV_RB16(packet + 6) >> 4) & 0xfff);
			break;
		case DCA_MARKER_RAW_LE:
			blockCount = (AV_RL16(packet + 4) >> 2) & 0x7f;
			frameSize = (AV_RL16(packet + 4) & 0x3) << 12 | ((AV_RL16(packet + 6) >> 4) & 0xfff);
			break;
		case DCA_MARKER_14B_BE:
		case DCA_MARKER_14B_LE:
		default:
			return -1;
	}

	blockCount++;
	frameSize++;
	
	int spdif_type;
	switch (blockCount) {
		case 16:
			spdif_type = 0x0b; //512-sample bursts
			break;
		case 32:
			spdif_type = 0x0c; //1024-sample bursts
			break;
		case 64:
			spdif_type = 0x0d; //2048-sample bursts
			break;
		default:
			return -1;
			break;
	}
	
	int totalSize = blockCount * 256 * channelCount / 4;
	memset(outputBuffer, 0, totalSize);
	int offset = 0;
	
	if(bigEndian)
	{
		memcpy(outputBuffer+offset, p_sync_be, 4);
		offset += channelCount * 2;
		outputBuffer[offset] = 0;
		outputBuffer[offset+1] = spdif_type;
		outputBuffer[offset+2] = (frameSize >> 5) & 0xff;
		outputBuffer[offset+3] = (frameSize << 3) & 0xff;
	}
	else
	{
		memcpy(outputBuffer+offset, p_sync_le, 4);
		offset += channelCount * 2;
		outputBuffer[offset] = spdif_type;
		outputBuffer[offset+1] = 0;
		outputBuffer[offset+2] = (frameSize << 3) & 0xff;
		outputBuffer[offset+3] = (frameSize >> 5) & 0xff;
	}
	
	offset += channelCount * 2;
	
	if((mrk == DCA_MARKER_RAW_BE || mrk == DCA_MARKER_14B_BE) && !bigEndian)
	{
		int i;
		int count = frameSize & ~0x3;
		for(i=0; i<count; i+=4)
		{
			outputBuffer[offset] = packet[i+1];
			outputBuffer[offset+1] = packet[i];
			outputBuffer[offset+2] = packet[i+3];
			outputBuffer[offset+3] = packet[i+2];
			offset += channelCount * 2;
		}
		switch (frameSize & 0x3) {
			case 3:
				outputBuffer[offset+3] = packet[i+2];
			case 2:
				outputBuffer[offset] = packet[i+1];
			case 1:
				outputBuffer[offset+1] = packet[i];
			default:
				break;
		}
	}
	else
	{
		int i;
		for(i=0; i<frameSize; i+=4)
		{
			memcpy(outputBuffer + offset, packet+i, 4);
			offset += channelCount * 2;
		}
	}

	*outBufUsed = totalSize;
	return frameSize;
}

UInt32 FFissionDecoder::InterleaveSamples(void *outputDataUntyped, Byte *inputDataUntyped, int amountToCopy)
{
	SInt16 *outputData = (SInt16 *)outputDataUntyped;
	SInt16 *inputData = (SInt16 *)inputDataUntyped;
	int channelCount = avContext->channels;
	int framesToCopy = amountToCopy / channelCount / 2;
	for(int i=0; i<framesToCopy; i++)
	{
		for(int j=0; j<channelCount; j++)
		{
			outputData[fullChannelMap[j]] = inputData[j];
		}
		outputData += channelCount;
		inputData += channelCount;
	}
	
	return framesToCopy * channelCount * 2;
}

UInt32 FFissionDecoder::ProduceOutputPackets(void* outOutputData,
											 UInt32& ioOutputDataByteSize,	// number of bytes written to outOutputData
											 UInt32& ioNumberPackets, 
											 AudioStreamPacketDescription* outPacketDescription)
{
	UInt32 ans;
	
	if (!mIsInitialized)
		CODEC_THROW(kAudioCodecStateError);
	
	if (!avCodec)
		OpenAVCodec();
	
	if (!avCodec)
		CODEC_THROW(kAudioCodecStateError);
	
	UInt32 written = 0;
	Byte *outData = (Byte *) outOutputData;
	
	// we have leftovers from the last packet, use that first
	if (outBufUsed > 0) {
		int amountToCopy = FFMIN(outBufUsed, ioOutputDataByteSize);
		amountToCopy = InterleaveSamples(outData, outputBuffer, amountToCopy);
		outBufUsed -= amountToCopy;
		written += amountToCopy;
		
		if (outBufUsed > 0)
			memmove(outputBuffer, outputBuffer + amountToCopy, outBufUsed);
	}
	
	// loop until we satisfy the request or run out of input data
	while (written < ioOutputDataByteSize && inputBuffer.GetNumPackets() > 0) {
		int packetSize;
		if(parser)
			packetSize = inputBuffer.GetDataAvailable();
		else
			packetSize = inputBuffer.GetCurrentPacketSize();
		uint8_t *packet = inputBuffer.GetData();
		
		// decode one packet to our buffer
		outBufUsed = outBufSize;
		if(parser)
		{	
			int parserLen = av_parser_parse2(parser, avContext, &packet, &packetSize, packet, packetSize, 0, 0, 0);
			inputBuffer.Zap(parserLen);
		}
		int len;
		if(dtsPassthrough)
			len = produceDTSPassthroughPackets(outputBuffer, &outBufUsed, packet, packetSize, avContext->channels, mOutputFormat.mFormatFlags & kLinearPCMFormatFlagIsBigEndian);
		else
		{
			AVPacket pkt;
			av_init_packet(&pkt);
			pkt.data = packet;
			pkt.size = packetSize;
			// FIXME avcodec_decode_audio4
			len = avcodec_decode_audio3(avContext, (int16_t *)outputBuffer, &outBufUsed, &pkt);
		}
		
		if (len < 0) {
			Codecprintf(NULL, "Error decoding audio frame\n");
			if(!parser)
				inputBuffer.Zap(packetSize);
			outBufUsed = 0;
			ioOutputDataByteSize = written;
			ioNumberPackets = ioOutputDataByteSize / (2 * mOutputFormat.NumberChannels());
			return kAudioCodecProduceOutputPacketFailure;
		}
		if(!parser)
			inputBuffer.Zap(len);
		
		// copy up to the amount requested
		int amountToCopy = FFMIN(outBufUsed, ioOutputDataByteSize - written);
		amountToCopy = InterleaveSamples(outData + written, outputBuffer, amountToCopy);
		outBufUsed -= amountToCopy;
		written += amountToCopy;
		
		// and save what's left over
		if (outBufUsed > 0)
			memmove(outputBuffer, outputBuffer + amountToCopy, outBufUsed);
	}
	
	if (written < ioOutputDataByteSize)
		ans = kAudioCodecProduceOutputPacketNeedsMoreInputData;
	else if (inputBuffer.GetNumPackets() > 0)
		// we have an entire packet left to decode
		ans = kAudioCodecProduceOutputPacketSuccessHasMore;
	else
		ans = kAudioCodecProduceOutputPacketSuccess;
	
	ioOutputDataByteSize = written;
	ioNumberPackets = ioOutputDataByteSize / (2 * mOutputFormat.NumberChannels());
	
	return ans;
}

UInt32 FFissionDecoder::GetVersion() const
{
	return kFFusionCodecVersion;
}

void FFissionVBRDecoder::GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData)
{
	switch (inPropertyID) {
		case kAudioCodecPropertyHasVariablePacketByteSizes:
		case kAudioCodecPropertyRequiresPacketDescription:
			if (ioPropertyDataSize != sizeof(UInt32))
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			break;
	}
	
	switch (inPropertyID) {
		case kAudioCodecPropertyHasVariablePacketByteSizes:
		case kAudioCodecPropertyRequiresPacketDescription:
			*reinterpret_cast<UInt32*>(outPropertyData) = true;
			break;
			
		// FIXME implement priming

		default:
			FFissionDecoder::GetProperty(inPropertyID, ioPropertyDataSize, outPropertyData);
	}
}

#include "ACPlugInDispatch.h"

AUDIOCOMPONENT_ENTRY(AudioCodecFactory, FFissionDecoder)
AUDIOCOMPONENT_ENTRY(AudioCodecFactory, FFissionVBRDecoder)