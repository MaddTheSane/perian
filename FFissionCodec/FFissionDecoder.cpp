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

typedef struct CookieAtomHeader {
    long           size;
    long           type;
    unsigned char  data[1];
} CookieAtomHeader;

struct CodecPair {
	OSType mFormatID;
	CodecID codecID;
};

static const CodecPair kAllInputFormats[] = 
{
	{ kAudioFormatWMA1MS, CODEC_ID_WMAV1 },
	{ kAudioFormatWMA2MS, CODEC_ID_WMAV2 },
	{ kAudioFormatFlashADPCM, CODEC_ID_ADPCM_SWF },
	{ kAudioFormatXiphVorbis, CODEC_ID_VORBIS },
	{ kAudioFormatMPEGLayer2, CODEC_ID_MP2 },
	{ kAudioFormatMPEGLayer1, CODEC_ID_MP2 },
	{ 0x6d730050, CODEC_ID_MP2 },
	{ kAudioFormatTTA, CODEC_ID_TTA },
	{ kAudioFormatDTS, CODEC_ID_DTS },
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


FFissionDecoder::FFissionDecoder(UInt32 inInputBufferByteSize) : FFissionCodec(0)
{
	kIntPCMOutFormatFlag = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagsNativeEndian | kLinearPCMFormatFlagIsPacked;
	magicCookie = NULL;
	magicCookieSize = 0;
	
	for (int i = 0; kAllInputFormats[i].codecID != CODEC_ID_NONE; i++) {
		CAStreamBasicDescription theInputFormat(kAudioStreamAnyRate, kAllInputFormats[i].mFormatID, 0, 1, 0, 0, 0, 0);
		AddInputFormat(theInputFormat);
	}
	
	// libavcodec outputs 16-bit native-endian integer pcm, so why do conversions ourselves?
	CAStreamBasicDescription theOutputFormat(kAudioStreamAnyRate, kAudioFormatLinearPCM, 0, 1, 0, 0, 16, kIntPCMOutFormatFlag);
	AddOutputFormat(theOutputFormat);
	
	inputBuffer.Initialize(inInputBufferByteSize);
	outBufUsed = 0;
	outBufSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
	outputBuffer = new Byte[outBufSize];
}

FFissionDecoder::~FFissionDecoder()
{
	if (magicCookie)
		delete[] magicCookie;
	
	delete[] outputBuffer;
}

void FFissionDecoder::Initialize(const AudioStreamBasicDescription* inInputFormat, const AudioStreamBasicDescription* inOutputFormat, const void* inMagicCookie, UInt32 inMagicCookieByteSize)
{
	if (inMagicCookieByteSize > 0)
		SetMagicCookie(inMagicCookie, inMagicCookieByteSize);
	
	FFissionCodec::Initialize(inInputFormat, inOutputFormat, inMagicCookie, inMagicCookieByteSize);
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
}

void FFissionDecoder::SetupExtradata(OSType formatID)
{
	if (!magicCookie) return;
	
	switch (formatID) {
		case kAudioFormatWMA1MS:
		case kAudioFormatWMA2MS:
		case kAudioFormatTTA:
			if (magicCookieSize < 12 + 18 + 8 + 8)
				return;
			
			avContext->extradata = magicCookie + 12 + 18 + 8;
			avContext->extradata_size = magicCookieSize - 12 - 18 - 8 - 8;
			break;
			
		case kAudioFormatXiphVorbis:
			avContext->extradata_size = ConvertXiphVorbisCookie();
			avContext->extradata = magicCookie;
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
	Byte *newCookie = new Byte[len + len/255 + 64];
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
	}
	
	switch (inPropertyID) {
		case kAudioCodecPropertyNameCFString:
			CFStringRef name = CFCopyLocalizedStringFromTableInBundle(CFSTR("Perian FFmpeg audio decoder"), CFSTR("CodecNames"), GetCodecBundle(), CFSTR(""));
			*(CFStringRef*)outPropertyData = name;
			break; 
			
		case kAudioCodecPropertyInputBufferSize:
			*reinterpret_cast<UInt32*>(outPropertyData) = inputBuffer.GetBufferByteSize();
			break;
			
		case kAudioCodecPropertyUsedInputBufferSize:
			*reinterpret_cast<UInt32*>(outPropertyData) = inputBuffer.GetDataAvailable();
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
	
	// check to make sure the input format is legal
	if (avcodec_find_decoder(codecID) == NULL) {
		Codecprintf(NULL, "Unsupported input format id %4.4s\n", &inInputFormat.mFormatID);
		CODEC_THROW(kAudioCodecUnsupportedFormatError);
	}
	
	// tell our base class about the new format
	FFissionCodec::SetCurrentInputFormat(inInputFormat);
}
	
void FFissionDecoder::OpenAVCodec()
{
	if (!mIsInitialized)
		CODEC_THROW(kAudioCodecStateError);
	
	CodecID codecID = GetCodecID(mInputFormat.mFormatID);
	
	avCodec = avcodec_find_decoder(codecID);
	
	avcodec_get_context_defaults(avContext);
	
	avContext->sample_rate = mInputFormat.mSampleRate;
	avContext->channels = mInputFormat.mChannelsPerFrame;
	avContext->block_align = mInputFormat.mBytesPerPacket;
	avContext->frame_size = mInputFormat.mFramesPerPacket;
	avContext->bits_per_sample = mInputFormat.mBitsPerChannel;
	
	if (avContext->sample_rate == 0) {
		Codecprintf(NULL, "Invalid sample rate %d\n", avContext->sample_rate);
		avCodec = NULL;
		return;
	}
	
	if (magicCookie) {
		SetupExtradata(mInputFormat.mFormatID);
	}
	
	if (avcodec_open(avContext, avCodec)) {
		Codecprintf(NULL, "error opening audio avcodec\n");
		avCodec = NULL;
		CODEC_THROW(kAudioCodecUnsupportedFormatError);
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

UInt32 FFissionDecoder::ProduceOutputPackets(void* outOutputData,
											 UInt32& ioOutputDataByteSize,	// number of bytes written to outOutputData
											 UInt32& ioNumberPackets, 
											 AudioStreamPacketDescription* outPacketDescription)
{
	UInt32 ans;
	
	if (!avCodec)
		OpenAVCodec();
	
	if (!mIsInitialized || !avCodec)
		CODEC_THROW(kAudioCodecStateError);
	
	UInt32 written = 0;
	Byte *outData = (Byte *) outOutputData;
	
	// we have leftovers from the last packet, use that first
	if (outBufUsed > 0) {
		int amountToCopy = FFMIN(outBufUsed, ioOutputDataByteSize);
		memcpy(outData, outputBuffer, amountToCopy);
		outBufUsed -= amountToCopy;
		written += amountToCopy;
		
		if (outBufUsed > 0)
			memmove(outputBuffer, outputBuffer + amountToCopy, outBufUsed);
	}
	
	// loop until we satisfy the request or run out of input data
	while (written < ioOutputDataByteSize && inputBuffer.GetNumPackets() > 0) {
		int packetSize = inputBuffer.GetCurrentPacketSize();
		uint8_t *packet = inputBuffer.GetData();
		
		// decode one packet to our buffer
		outBufUsed = outBufSize;
		int len = avcodec_decode_audio2(avContext, (int16_t *)outputBuffer, &outBufUsed, packet, packetSize);
		
		if (len < 0) {
			Codecprintf(NULL, "Error decoding audio frame\n");
			inputBuffer.Zap(packetSize);
			outBufUsed = 0;
			ioOutputDataByteSize = written;
			ioNumberPackets = ioOutputDataByteSize / (2 * mOutputFormat.NumberChannels());
			return kAudioCodecProduceOutputPacketFailure;
		}
		inputBuffer.Zap(len);
		
		// copy up to the amount requested
		int amountToCopy = FFMIN(outBufUsed, ioOutputDataByteSize - written);
		memcpy(outData + written, outputBuffer, amountToCopy);
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

// comment from XiphQT (variable frames per packet means FrameSize should be reported 0, 
// but apparently needs 1 on Intel?):
/* The following line has been changed according to Apple engineers' suggestion
   I received via Steve Nicolai (in response to *my* bugreport, I think...).
   (Why don't they just implement the VBR-VFPP properly? *sigh*) */
#ifdef TARGET_CPU_X86
#define SHOULD_BE_ZERO 1
#else
#define SHOULD_BE_ZERO 0
#endif

void FFissionVBRDecoder::GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData)
{
	switch (inPropertyID) {
		case kAudioCodecPropertyPacketFrameSize:
		case kAudioCodecPropertyHasVariablePacketByteSizes:
		case kAudioCodecPropertyRequiresPacketDescription:
			if (ioPropertyDataSize != sizeof(UInt32))
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			break;
	}
	
	switch (inPropertyID) {
		case kAudioCodecPropertyPacketFrameSize:
			*reinterpret_cast<UInt32*>(outPropertyData) = SHOULD_BE_ZERO;
			break;
			
		case kAudioCodecPropertyHasVariablePacketByteSizes:
		case kAudioCodecPropertyRequiresPacketDescription:
			*reinterpret_cast<UInt32*>(outPropertyData) = true;
			break;
			
		default:
			FFissionDecoder::GetProperty(inPropertyID, ioPropertyDataSize, outPropertyData);
	}
}


extern "C"
ComponentResult	FFissionDecoderEntry(ComponentParameters* inParameters, FFissionDecoder* inThis)
{
	return ACCodecDispatch(inParameters, inThis);
}

extern "C"
ComponentResult	FFissionVBRDecoderEntry(ComponentParameters* inParameters, FFissionVBRDecoder* inThis)
{
	return ACCodecDispatch(inParameters, inThis);
}
