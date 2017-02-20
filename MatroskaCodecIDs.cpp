/*
 *  MatroskaCodecIDs.h
 *
 *    MatroskaCodecIDs.h - Codec description extension conversion utilities between MKV and QT
 *
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

#include <QuickTime/QuickTime.h>
#include <AudioToolbox/AudioToolbox.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxTrackEntryData.h>
#include <matroska/KaxTrackAudio.h>
#include <matroska/KaxContentEncoding.h>
#include <string>
#include "CommonUtils.h"
#include "Codecprintf.h"
#include "MatroskaCodecIDs.h"

extern "C" {
#include <libavcodec/bytestream.h>
}

using namespace std;
using namespace libmatroska;

/*
 * Used for codecs whose extradata can be copied straight into a sample description extension.
 * fallbackType is for formats with lots of invalid files (avi-style H264 or MPEG4) which we have
 * an extradataless-equivalent fourcc for.
 */
ComponentResult DescExt_Default(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir, OSType extType, OSType fallbackType)
{
	if (!tr_entry || !desc) return paramErr;
	ImageDescriptionHandle imgDesc = (ImageDescriptionHandle) desc;
	
	if (dir == kToSampleDescription) {
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL) {
			if (fallbackType) {
				(*desc)->dataFormat = fallbackType;
				return noErr;
			} else {
				Codecprintf(NULL, "Missing extradata in track\n");
				return invalidAtomErr;
			}
		}
		
		Handle imgDescExt;
		PtrToHand(codecPrivate->GetBuffer(), &imgDescExt, codecPrivate->GetSize());
		
		AddImageDescriptionExtension(imgDesc, imgDescExt, extType);
		
		DisposeHandle(imgDescExt);
	}
	return noErr;
}

// xiph-qt expects these this sound extension to have been created from first 3 packets
// which are stored in CodecPrivate in Matroska
ComponentResult DescExt_XiphVorbis(KaxTrackEntry *tr_entry, Handle *cookie, DescExtDirection dir)
{
	if (!tr_entry || !cookie) return paramErr;
	
	if (dir == kToSampleDescription) {
		Handle sndDescExt;
		unsigned char *privateBuf;
		size_t privateSize;
		uint8_t numPackets;
		int offset = 1, i;
		UInt32 uid = 0;
		
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL)
			return invalidAtomErr;
		
		KaxTrackUID *trackUID = FindChild<KaxTrackUID>(*tr_entry);
		if (trackUID != NULL)
			uid = uint32(*trackUID);
		
		privateSize = codecPrivate->GetSize();
		privateBuf = (unsigned char *) codecPrivate->GetBuffer();
		
		if (!privateBuf)
			return invalidAtomErr;
		
		GetByteContext g;
		bytestream2_init(&g, privateBuf, privateSize);
		
		numPackets = bytestream2_get_byte(&g) + 1;
		
		if (numPackets != 3) {
			return invalidAtomErr;
		}
		
		int packetSizes[3] = {};
		
		// get the sizes of the packets
		packetSizes[numPackets - 1] = privateSize - 1;
		int packetNum = 0;
		for (i = 1; packetNum < numPackets - 1; i++) {
			packetSizes[packetNum] += bytestream2_get_byte(&g);
			if (privateBuf[i] < 255) {
				packetSizes[numPackets - 1] -= packetSizes[packetNum];
				packetNum++;
			}
			offset++;
		}
		packetSizes[numPackets - 1] -= offset - 1;
		
		unsigned packetSize = offset+packetSizes[0]+packetSizes[1]+packetSizes[2];
		
		if (packetSize > privateSize || packetSizes[0] < 0 || packetSizes[1] < 0 || packetSizes[2] < 0) {
			return invalidAtomErr;
		}

		// first packet
		uint32_t serial_header_atoms[3+2] = { EndianU32_NtoB(3*4), 
			EndianU32_NtoB(kCookieTypeOggSerialNo), 
			EndianU32_NtoB(uid),
			EndianU32_NtoB(packetSizes[0] + 2*4), 
			EndianU32_NtoB(kCookieTypeVorbisHeader) };
		
		PtrToHand(serial_header_atoms, &sndDescExt, sizeof(serial_header_atoms));
		PtrAndHand(&privateBuf[offset], sndDescExt, packetSizes[0]);
		
		// second packet
		uint32_t atomhead2[2] = { EndianU32_NtoB(packetSizes[1] + sizeof(atomhead2)), 
			EndianU32_NtoB(kCookieTypeVorbisComments) };
		PtrAndHand(atomhead2, sndDescExt, sizeof(atomhead2));
		PtrAndHand(&privateBuf[offset + packetSizes[0]], sndDescExt, packetSizes[1]);
		
		// third packet
		uint32_t atomhead3[2] = { EndianU32_NtoB(packetSizes[2] + sizeof(atomhead3)), 
			EndianU32_NtoB(kCookieTypeVorbisCodebooks) };
		PtrAndHand(atomhead3, sndDescExt, sizeof(atomhead3));
		PtrAndHand(&privateBuf[offset + packetSizes[1] + packetSizes[0]], sndDescExt, packetSizes[2]);

		*cookie = sndDescExt;
	}
	return noErr;
	
bail:
	return invalidAtomErr;
}

// xiph-qt expects these this sound extension to have been created in this way
// from the packets which are stored in the CodecPrivate element in Matroska
ComponentResult DescExt_XiphFLAC(KaxTrackEntry *tr_entry, Handle *cookie, DescExtDirection dir)
{
	if (!tr_entry || !cookie) return paramErr;
	
	if (dir == kToSampleDescription) {
		Handle sndDescExt;
		UInt32 uid = 0;
		
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL)
			return invalidAtomErr;
		
		KaxTrackUID *trackUID = FindChild<KaxTrackUID>(*tr_entry);
		if (trackUID != NULL)
			uid = uint32(*trackUID);
		
		size_t privateSize = codecPrivate->GetSize();
		UInt8 *privateBuf = (unsigned char *) codecPrivate->GetBuffer();
		
		unsigned long serialnoatom[3] = { EndianU32_NtoB(sizeof(serialnoatom)), 
			EndianU32_NtoB(kCookieTypeOggSerialNo), 
			EndianU32_NtoB(uid) };
		
		PtrToHand(serialnoatom, (Handle*)&sndDescExt, sizeof(serialnoatom));
		
		GetByteContext g;
		bytestream2_init(&g, privateBuf, privateSize);
		
		bytestream2_skip(&g, 4);
		
		bool lastPacket = false;
		
		while (!lastPacket) {
			uint32_t packetHeader = bytestream2_peek_be32(&g);
			lastPacket = packetHeader >> 31;
			int blockType = (packetHeader >> 24) & 0x7F;
			uint32_t packetSize = (packetHeader & 0xFFFFFF) + 4;
			uint32_t xiphHeader[2] = {EndianU32_NtoB(packetSize + sizeof(xiphHeader)),
				EndianU32_NtoB(blockType ? kCookieTypeFLACMetadata : kCookieTypeFLACStreaminfo)};
			
			//Codecprintf(NULL, "FLAC packet type '%s' bt %d size %d, offset %d, %d left\n", FourCCString(xiphHeader[1]), blockType,
			//			packetSize, bytestream2_tell(&g), bytestream2_get_bytes_left(&g));
			
			if (packetSize > bytestream2_get_bytes_left(&g))
				break;
			
			PtrAndHand(xiphHeader, sndDescExt, sizeof(xiphHeader));
			PtrAndHand(g.buffer, sndDescExt, packetSize);
			
			bytestream2_skip(&g, packetSize);
		}
		
		*cookie = sndDescExt;	
	}
	return noErr;
}

// c.f. http://wiki.multimedia.cx/index.php?title=Understanding_AAC

struct MatroskaQT_AACProfileName
{
	const char *name;
	char profile;
};

static const MatroskaQT_AACProfileName kMatroskaAACProfiles[] = {
	{"A_AAC/MPEG2/MAIN", 1},
	{"A_AAC/MPEG2/LC", 2},
	{"A_AAC/MPEG2/LC/SBR", 2},
	{"A_AAC/MPEG2/SSR", 3},
	{"A_AAC/MPEG4/MAIN", 1},
	{"A_AAC/MPEG4/LC", 2},
	{"A_AAC/MPEG4/LC/SBR", 2},
	{"A_AAC/MPEG4/SSR", 3},
	{"A_AAC/MPEG4/LTP", 4}
};

static const unsigned kAACFrequencyIndexes[] = {
	96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000,
	11025, 8000, 7350
};

static bool ShouldWriteSBRExt(unsigned output_freq_index)
{
	if (output_freq_index < 3)
		return ShouldPlayHighFreqSBR();
	
	return true;
}

static int FindAACFreqIndex(double freq)
{
	unsigned ifreq = freq;
	
	for (int i = 0; i < sizeof(kAACFrequencyIndexes)/sizeof(unsigned); i++) {
		if (kAACFrequencyIndexes[i] == ifreq) return i;
	}
	
	return 15;
}

static void RecreateAACVOS(KaxTrackEntry *tr_entry, uint8_t *vosBuf, size_t *vosLen)
{
	KaxCodecID *tr_codec = FindChild<KaxCodecID>(*tr_entry);
	KaxTrackAudio & audioTrack = GetChild<KaxTrackAudio>(*tr_entry);
	KaxAudioSamplingFreq & sampleFreq = GetChild<KaxAudioSamplingFreq>(audioTrack);
	KaxAudioChannels & numChannels = GetChild<KaxAudioChannels>(audioTrack);
	KaxAudioOutputSamplingFreq * outputFreq = FindChild<KaxAudioOutputSamplingFreq>(audioTrack);
	unsigned channels = unsigned(numChannels);
	unsigned profile = 2, freq_index = FindAACFreqIndex(sampleFreq);
	uint8_t *vosStart = vosBuf;
	string codecString(*tr_codec);

	for (int i = 0; i < sizeof(kMatroskaAACProfiles)/sizeof(MatroskaQT_AACProfileName); i++) {
		const MatroskaQT_AACProfileName *prof = &kMatroskaAACProfiles[i];
		if (codecString == prof->name) {profile = prof->profile; break;}
	}
	
	// FIXME use a bitstream writer
	
	*vosBuf++ = (profile << 3) | (freq_index >> 1);
	*vosBuf++ = (freq_index << 7) | (channels << 3);
	
	if (freq_index == 15)
		Codecprintf(NULL, "unrecognized AAC frequency not supported\n");
	
	if (outputFreq) {
		unsigned output_freq_index = FindAACFreqIndex(*outputFreq);
				
		if (ShouldWriteSBRExt(output_freq_index)) {
			*vosBuf++ = 0x56;
			*vosBuf++ = 0xE5;
			*vosBuf++ = 0x80 | (output_freq_index << 3);
		}
	}
	
	*vosLen = vosBuf - vosStart;
}

// the esds atom creation is based off of the routines for it in ffmpeg's movenc.c
static unsigned int descrLength(unsigned int len)
{
    int i;
    for(i=1; len>>(7*i); i++)
		;
    return len + 1 + i;
}

static void putDescr(PutByteContext *p, int tag, unsigned int size)
{
    int i= descrLength(size) - size - 2;
	bytestream2_put_byte(p, tag);
    for(; i>0; i--)
		bytestream2_put_byte(p, size>>(7*i) | 0x80);
	bytestream2_put_byte(p, size & 0x7F);
}

// ESDS layout:
//  + version             (4 bytes)
//  + ES descriptor 
//   + Track ID            (2 bytes)
//   + Flags               (1 byte)
//   + DecoderConfig descriptor
//    + Object Type         (1 byte)
//    + Stream Type         (1 byte)
//    + Buffersize DB       (3 bytes)
//    + Max bitrate         (4 bytes)
//    + VBR/Avg bitrate     (4 bytes)
//    + DecoderSpecific info descriptor
//     + codecPrivate        (codecPrivate->GetSize())
//   + SL descriptor
//    + dunno               (1 byte)

uint8_t *CreateEsdsFromSetupData(uint8_t *codecPrivate, size_t vosLen, size_t *esdsLen, int trackID, bool audio, bool write_version)
{
	int decoderSpecificInfoLen = vosLen ? descrLength(vosLen) : 0;
	int versionLen = write_version ? 4 : 0;
	size_t _esdsLen = versionLen + descrLength(3 + descrLength(13 + decoderSpecificInfoLen) + descrLength(1));
	
	uint8_t *esds = (uint8_t*)av_malloc(_esdsLen);
	if (!esds)
		return NULL;
	
	PutByteContext p;
	bytestream2_init_writer(&p, esds, _esdsLen);
	
	// esds atom version (only needed for ImageDescription extension)
	if (write_version)
		bytestream2_put_be32(&p, 0);
	
	// ES Descriptor
	putDescr(&p, 0x03, 3 + descrLength(13 + decoderSpecificInfoLen) + descrLength(1));
	bytestream2_put_be16(&p, trackID);
	bytestream2_put_byte(&p, 0); // no flags
	
	// DecoderConfig descriptor
	putDescr(&p, 0x04, 13 + decoderSpecificInfoLen);
	
	// Object type indication, see http://gpac.sourceforge.net/tutorial/mediatypes.htm
	bytestream2_put_byte(&p, audio ? 0x40 : 0x20);
	
	// streamtype
	bytestream2_put_byte(&p, audio ? 0x15 : 0x11);
	
	// 3 bytes: buffersize DB (not sure how to get easily)
	bytestream2_put_byte(&p, 0);
	bytestream2_put_be16(&p, 0);
	
	// max bitrate, not sure how to get easily
	bytestream2_put_be32(&p, 0);
	
	// vbr
	bytestream2_put_be32(&p, 0);
	
	if (vosLen) {
		putDescr(&p, 0x05, vosLen);
		bytestream2_put_buffer(&p, codecPrivate, vosLen);
	}
	
	// SL descriptor
	putDescr(&p, 0x06, 1);
	bytestream2_put_byte(&p, 0x02);
	
	*esdsLen = _esdsLen;
	return esds;
}

static Handle CreateEsdsExt(KaxTrackEntry *tr_entry, bool audio)
{
	KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
	KaxTrackNumber *trackNum = FindChild<KaxTrackNumber>(*tr_entry);
	
	size_t vosLen = codecPrivate ? codecPrivate->GetSize() : 0;
	int trackID = trackNum ? uint16(*trackNum) : 1;
	uint8_t aacBuf[5] = {0};
	uint8_t *vosBuf = codecPrivate ? codecPrivate->GetBuffer() : NULL;
	size_t esdsLen = 0;
	
	// vosLen > 2 means SBR; some of those must be rewritten to avoid QT bugs(?)
	// FIXME: remove when QT works with them
	if (audio && (!vosBuf || vosLen > 2)) {
		RecreateAACVOS(tr_entry, aacBuf, &vosLen);
		vosBuf = aacBuf;
	} else if (!audio && !vosBuf)
		return NULL;
	
	uint8_t *esds = CreateEsdsFromSetupData(vosBuf, vosLen, &esdsLen, trackID, audio, !audio);
	
	if (!esds)
		return NULL;
	
	Handle esdsExt;
	PtrToHand(esds, &esdsExt, esdsLen);
	free(esds);
	
	return esdsExt;
}

ComponentResult DescExt_aac(KaxTrackEntry *tr_entry, Handle *cookie, DescExtDirection dir)
{
	if (!tr_entry || !cookie) return paramErr;
	
	if (dir == kToSampleDescription) {
		*cookie = CreateEsdsExt(tr_entry, true);
	}
	
	return noErr;
}

ComponentResult ASBDExt_LPCM(KaxTrackEntry *tr_entry, AudioStreamBasicDescription *asbd)
{
	if (!tr_entry || !asbd) return paramErr;
	
	KaxCodecID *tr_codec = FindChild<KaxCodecID>(*tr_entry);
	if (!tr_codec) return paramErr;
	string codecid(*tr_codec);
	bool isFloat = codecid == MKV_A_PCM_FLOAT;
	
	asbd->mFormatFlags = CalculateLPCMFlags(asbd->mBitsPerChannel, asbd->mBitsPerChannel, isFloat, isFloat ? false : (codecid == MKV_A_PCM_BIG), false);
	if (asbd->mBitsPerChannel == 8)
		asbd->mFormatFlags &= ~kLinearPCMFormatFlagIsSignedInteger;
	
	return noErr;
}

ComponentResult ASBDExt_AAC(KaxTrackEntry *tr_entry, Handle cookie, AudioStreamBasicDescription *asbd, AudioChannelLayout *acl)
{
	if (!tr_entry || !asbd) return paramErr;
	ByteCount cookieSize = GetHandleSize(cookie), aclSize = sizeof(*acl);
	OSStatus err = noErr;

	err = AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutFromESDS,
								 cookieSize,
								 *cookie,
								 &aclSize,
								 acl);
	if (err != noErr) 
		FourCCprintf("MatroskaQT: Error creating ACL from AAC esds ", err);
	
	return err;
}



ComponentResult MkvFinishSampleDescription(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir)
{
	KaxCodecID & tr_codec = GetChild<KaxCodecID>(*tr_entry);
	
	string codecString(tr_codec);
	
	if (codecString == MKV_V_MS) {
		// BITMAPINFOHEADER is stored in the private data, and some codecs (WMV)
		// need it to decode
		KaxCodecPrivate & codecPrivate = GetChild<KaxCodecPrivate>(*tr_entry);
		
		Handle imgDescExt;
		PtrToHand(codecPrivate.GetBuffer(), &imgDescExt, codecPrivate.GetSize());
		
		AddImageDescriptionExtension((ImageDescriptionHandle) desc, imgDescExt, 'strf');
		
	} else if (codecString == MKV_V_QT) {
		// This seems to work fine, but there's something it's missing to get the 
		// image description to match perfectly (last 2 bytes are different)
		// Figure it out later...
		KaxCodecPrivate & codecPrivate = GetChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate.GetSize() < sizeof(ImageDescription)) {
			Codecprintf(NULL, "MatroskaQT: QuickTime track %hu doesn't have needed stsd data\n", 
						uint16(tr_entry->TrackNumber()));
			return -1;
		}
		
		ImageDescriptionHandle imgDesc = (ImageDescriptionHandle) desc;
		SetHandleSize((Handle) imgDesc, codecPrivate.GetSize());
		memcpy(&(*imgDesc)->cType, codecPrivate.GetBuffer(), codecPrivate.GetSize());
		// it's stored in big endian, so flip endian to native
		// I think we have to do this, need to check on Intel without them
		(*imgDesc)->idSize = codecPrivate.GetSize();
		(*imgDesc)->cType = EndianU32_BtoN((*imgDesc)->cType);
		(*imgDesc)->resvd1 = EndianS32_BtoN((*imgDesc)->resvd1);
		(*imgDesc)->resvd2 = EndianS16_BtoN((*imgDesc)->resvd2);
		(*imgDesc)->dataRefIndex = EndianS16_BtoN((*imgDesc)->dataRefIndex);
		(*imgDesc)->version = EndianS16_BtoN((*imgDesc)->version);
		(*imgDesc)->revisionLevel = EndianS16_BtoN((*imgDesc)->revisionLevel);
		(*imgDesc)->vendor = EndianS32_BtoN((*imgDesc)->vendor);
		(*imgDesc)->temporalQuality = EndianU32_BtoN((*imgDesc)->temporalQuality);
		(*imgDesc)->spatialQuality = EndianU32_BtoN((*imgDesc)->spatialQuality);
		(*imgDesc)->width = EndianS16_BtoN((*imgDesc)->width);
		(*imgDesc)->height = EndianS16_BtoN((*imgDesc)->height);
		(*imgDesc)->vRes = EndianS32_BtoN((*imgDesc)->vRes);
		(*imgDesc)->hRes = EndianS32_BtoN((*imgDesc)->hRes);
		(*imgDesc)->dataSize = EndianS32_BtoN((*imgDesc)->dataSize);
		(*imgDesc)->frameCount = EndianS16_BtoN((*imgDesc)->frameCount);
		(*imgDesc)->depth = EndianS16_BtoN((*imgDesc)->depth);
		(*imgDesc)->clutID = EndianS16_BtoN((*imgDesc)->clutID);

	} else {
		OSType extType = 0, fallbackType = 0;
		switch ((*desc)->dataFormat) {
			case kH264CodecType:
				extType = 'avcC';
				fallbackType = 'H264';
				break;

			case kSubFormatVobSub:
				// VobSub stores the .idx file in the codec private, pass it as an .IDX extension
				extType = kVobSubIdxExtension;
				break;
				
			case kVideoFormatReal5:
			case kVideoFormatRealG2:
			case kVideoFormatReal8:
			case kVideoFormatReal9:
				extType = kRealVideoExtension;
				break;
				
			case kMPEG4VisualCodecType:
				extType = 'esds';
				fallbackType = 'XVID';
				break;
				
			case kVideoFormatTheora:
			case kSubFormatSSA:
				extType = (*desc)->dataFormat;
		}
		
		if (extType)
			return DescExt_Default(tr_entry, desc, dir, extType, fallbackType);
	}
	return noErr;
}

// some default channel layouts for 3 to 8 channels
// vorbis, flac and aac should be correct unless extradata specifices something else for aac
static const AudioChannelLayout vorbisChannelLayouts[6] = {
	{ kAudioChannelLayoutTag_UseChannelBitmap, kAudioChannelBit_Left | kAudioChannelBit_Right | kAudioChannelBit_CenterSurround },
	{ kAudioChannelLayoutTag_Quadraphonic },		// L R Ls Rs
	{ kAudioChannelLayoutTag_MPEG_5_0_C },			// L C R Ls Rs
	{ kAudioChannelLayoutTag_MPEG_5_1_C },			// L C R Ls Rs LFE
};

// these should be the default for the number of channels; esds can specify other mappings
static const AudioChannelLayout aacChannelLayouts[6] = {
	{ kAudioChannelLayoutTag_MPEG_3_0_B },			// C L R according to wiki.multimedia.cx
	{ kAudioChannelLayoutTag_AAC_4_0 },				// C L R Cs
	{ kAudioChannelLayoutTag_AAC_5_0 },				// C L R Ls Rs
	{ kAudioChannelLayoutTag_AAC_5_1 },				// C L R Ls Rs Lfe
	{ kAudioChannelLayoutTag_AAC_6_1 },				// C L R Ls Rs Cs Lfe
	{ kAudioChannelLayoutTag_AAC_7_1 },				// C Lc Rc L R Ls Rs Lfe
};

static const AudioChannelLayout ac3ChannelLayouts[6] = {
	{ kAudioChannelLayoutTag_ITU_3_0 },				// L R C
	{ kAudioChannelLayoutTag_ITU_3_1 },				// L R C Cs
	{ kAudioChannelLayoutTag_ITU_3_2 },				// L R C Ls Rs
	{ kAudioChannelLayoutTag_ITU_3_2_1 },			// L R C LFE Ls Rs
};

static const AudioChannelLayout flacChannelLayouts[6] = {
	{ kAudioChannelLayoutTag_ITU_3_0 },				// L R C
	{ kAudioChannelLayoutTag_Quadraphonic },		// L R Ls Rs
	{ kAudioChannelLayoutTag_ITU_3_2 },				// L R C Ls Rs
	{ kAudioChannelLayoutTag_ITU_3_2_1 },			// L R C LFE Ls Rs
	{ kAudioChannelLayoutTag_MPEG_6_1_A },			// L R C LFE Ls Rs Cs
	{ kAudioChannelLayoutTag_MPEG_7_1_C },			// L R C LFE Ls Rs Rls Rrs
};

// 5.1 is tested, the others are just guesses
static const AudioChannelLayout dtsChannelLayouts[6] = {
	{ kAudioChannelLayoutTag_MPEG_3_0_B },			// C L R
	{ kAudioChannelLayoutTag_MPEG_4_0_B },			// C L R Cs
	{ kAudioChannelLayoutTag_MPEG_5_0_D },			// C L R Ls Rs
	{ kAudioChannelLayoutTag_MPEG_5_1_D },			// C L R Ls Rs Lfe
};

// Taken from Apple's ALAC source code
static const AudioChannelLayout	ALACChannelLayoutTags[] =
{
	{ kAudioChannelLayoutTag_Mono },        // C
	{ kAudioChannelLayoutTag_Stereo },		// L R
	{ kAudioChannelLayoutTag_MPEG_3_0_B },	// C L R
	{ kAudioChannelLayoutTag_MPEG_4_0_B },	// C L R Cs
	{ kAudioChannelLayoutTag_MPEG_5_0_D },	// C L R Ls Rs
	{ kAudioChannelLayoutTag_MPEG_5_1_D },	// C L R Ls Rs LFE
	{ kAudioChannelLayoutTag_AAC_6_1 },		// C L R Ls Rs Cs LFE
	{ kAudioChannelLayoutTag_MPEG_7_1_B }	// C Lc Rc L R Ls Rs LFE    (doc: IS-13818-7 MPEG2-AAC)
};


AudioChannelLayout GetDefaultChannelLayout(AudioStreamBasicDescription *asbd)
{
	AudioChannelLayout acl = {0};
	int channelIndex = asbd->mChannelsPerFrame - 3;
	
	if (channelIndex >= 0 && channelIndex < 6) {
		switch (asbd->mFormatID) {
			case kAudioFormatXiphVorbis:
				acl = vorbisChannelLayouts[channelIndex];
				break;
				
			case kAudioFormatXiphFLAC:
				acl = flacChannelLayouts[channelIndex];
				break;
				
			case kAudioFormatMPEG4AAC:
				// TODO: use extradata to make ACL
				acl = aacChannelLayouts[channelIndex];
				break;
				
			case kAudioFormatAC3:
			case kAudioFormatAC3MS:
				acl = ac3ChannelLayouts[channelIndex];
				break;
				
			case kAudioFormatDTS:
				acl = dtsChannelLayouts[channelIndex];
				break;
				
			case kAudioFormatAppleLossless:
				acl = ALACChannelLayoutTags[channelIndex];
				break;
		}
	}
	
	return acl;
}


ComponentResult MkvFinishAudioDescription(KaxTrackEntry *tr_entry, Handle *cookie, AudioStreamBasicDescription *asbd, AudioChannelLayout *acl)
{
	KaxCodecID & tr_codec = GetChild<KaxCodecID>(*tr_entry);
	KaxCodecPrivate & codecPrivate = GetChild<KaxCodecPrivate>(*tr_entry);
	string codecString(tr_codec);
	
	if (codecString == MKV_A_MS) {
		PtrToHand(codecPrivate.GetBuffer(), cookie, codecPrivate.GetSize());
	}
	
	switch (asbd->mFormatID) {
		case kAudioFormatXiphVorbis:
			DescExt_XiphVorbis(tr_entry, cookie, kToSampleDescription);
			break;
			
		case kAudioFormatXiphFLAC:
			DescExt_XiphFLAC(tr_entry, cookie, kToSampleDescription);
			break;
			
		case kAudioFormatMPEG4AAC:
		case kMPEG4AudioFormat:
			DescExt_aac(tr_entry, cookie, kToSampleDescription);
			break;
	}
	
	switch (asbd->mFormatID) {
		case kAudioFormatMPEG4AAC:
			if (!cookie || !*cookie) return paramErr;
			ASBDExt_AAC(tr_entry, *cookie, asbd, acl);
			break;
			
		case kAudioFormatLinearPCM:
			ASBDExt_LPCM(tr_entry, asbd);
			break;
	}
	return noErr;
}

typedef struct {
	OSType cType;
	int twocc;
} WavCodec;

static const WavCodec kWavCodecIDs[] = {
	{ kAudioFormatMPEGLayer2, 0x50 },
	{ kAudioFormatMPEGLayer3, 0x55 },
	{ kAudioFormatAC3, 0x2000 },
	{ kAudioFormatDTS, 0x2001 },
	{ kAudioFormatMPEG4AAC, 0xff },
	{ kAudioFormatXiphFLAC, 0xf1ac },
	{ kAudioFormatWMA1MS, 0x0160 },
	{ kAudioFormatWMA2MS, 0x0161 },
	{ 0, 0 }
};

typedef struct {
	OSType cType;
	const char *mkvID;
} MatroskaQT_Codec;

// the first matching pair is used for conversion
static const MatroskaQT_Codec kMatroskaCodecIDs[] = {
	{ kRawCodecType, "V_UNCOMPRESSED" },
	{ kMPEG4VisualCodecType, "V_MPEG4/ISO/ASP" },
	{ kMPEG4VisualCodecType, "V_MPEG4/ISO/SP" },
	{ kMPEG4VisualCodecType, "V_MPEG4/ISO/AP" },
	{ kH264CodecType, "V_MPEG4/ISO/AVC" },
	{ kVideoFormatHEVC, "V_MPEGH/ISO/HEVC" },
	{ kVideoFormatMSMPEG4v3, "V_MPEG4/MS/V3" },
	{ kMPEG1VisualCodecType, "V_MPEG1" },
	{ kMPEG2VisualCodecType, "V_MPEG2" },
	{ kVideoFormatReal5, "V_REAL/RV10" },
	{ kVideoFormatRealG2, "V_REAL/RV20" },
	{ kVideoFormatReal8, "V_REAL/RV30" },
	{ kVideoFormatReal9, "V_REAL/RV40" },
	{ kVideoFormatTheora, "V_THEORA" },
	{ kVideoFormatSnow, "V_SNOW" },
	{ kVideoFormatVP8, "V_VP8" },
	{ kVideoFormatVP9, "V_VP9" },
	//{ kVideoFormatProRes, "V_PRORES" },
	
	{ kAudioFormatMPEG4AAC, "A_AAC" },
	{ kAudioFormatMPEG4AAC, "A_AAC/MPEG4/LC" },
	{ kAudioFormatMPEG4AAC, "A_AAC/MPEG4/MAIN" },
	{ kAudioFormatMPEG4AAC, "A_AAC/MPEG4/LC/SBR" },
	{ kAudioFormatMPEG4AAC, "A_AAC/MPEG4/SSR" },
	{ kAudioFormatMPEG4AAC, "A_AAC/MPEG4/LTP" },
	{ kAudioFormatMPEG4AAC, "A_AAC/MPEG2/LC" },
	{ kAudioFormatMPEG4AAC, "A_AAC/MPEG2/MAIN" },
	{ kAudioFormatMPEG4AAC, "A_AAC/MPEG2/LC/SBR" },
	{ kAudioFormatMPEG4AAC, "A_AAC/MPEG2/SSR" },
	{ kAudioFormatMPEGLayer1, "A_MPEG/L1" },
	{ kAudioFormatMPEGLayer2, "A_MPEG/L2" },
	{ kAudioFormatMPEGLayer3, "A_MPEG/L3" },
	{ kAudioFormatAC3, "A_AC3" },
	{ kAudioFormatAC3MS, "A_AC3" },
	// anything special for these two?
	{ kAudioFormatAC3, "A_AC3/BSID9" },
	{ kAudioFormatAC3, "A_AC3/BSID10" },
	{ kAudioFormatXiphVorbis, "A_VORBIS" },
	{ kAudioFormatXiphFLAC, "A_FLAC" },
	{ kAudioFormatLinearPCM, "A_PCM/INT/LIT" },
	{ kAudioFormatLinearPCM, "A_PCM/INT/BIG" },
	{ kAudioFormatLinearPCM, "A_PCM/FLOAT/IEEE" },
	{ kAudioFormatDTS, "A_DTS" },
	{ kAudioFormatDTS, "A_DTS/EXPRESS" },
	{ kAudioFormatDTS, "A_DTS/LOSSLESS" },
	{ kAudioFormatTTA, "A_TTA1" },
	{ kAudioFormatWavepack, "A_WAVPACK4" },
	{ kAudioFormatReal1, "A_REAL/14_4" },
	{ kAudioFormatReal2, "A_REAL/28_8" },
	{ kAudioFormatRealCook, "A_REAL/COOK" },
	{ kAudioFormatRealSipro, "A_REAL/SIPR" },
	{ kAudioFormatRealLossless, "A_REAL/RALF" },
	{ kAudioFormatRealAtrac3, "A_REAL/ATRC" },
	{ kAudioFormatOpus, "A_OPUS" },
	{ kAudioFormatAppleLossless, "A_ALAC" },
	
#if 0
	{ kBMPCodecType, "S_IMAGE/BMP" },

	{ kSubFormatUSF, "S_TEXT/USF" },
#endif
	{ kSubFormatSSA, "S_TEXT/SSA" },
    { kSubFormatSSA, "S_SSA" },
	{ kSubFormatASS, "S_TEXT/ASS" },
	{ kSubFormatASS, "S_ASS" },
	{ kSubFormatUTF8, "S_TEXT/UTF8" },
	{ kSubFormatUTF8, "S_TEXT/ASCII" },
	{ kSubFormatVobSub, "S_VOBSUB" },
};


FourCharCode MkvGetFourCC(KaxTrackEntry *tr_entry)
{
	KaxCodecID *tr_codec = FindChild<KaxCodecID>(*tr_entry);
	if (tr_codec == NULL)
		return 0;
	
	string codecString(*tr_codec);
	
	if (codecString == MKV_V_MS) {
		// avi compatibility mode, 4cc is in private info
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL || codecPrivate->GetSize() <= (16+3))
			return 0;
		
		// offset to biCompression in BITMAPINFO
		unsigned char *p = (unsigned char *) codecPrivate->GetBuffer() + 16;
		return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		
	} else if (codecString == MKV_A_MS) {
		// acm compatibility mode, twocc is in private info
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL || codecPrivate->GetSize() <= 2)
			return 0;
		
		unsigned char *p = (unsigned char *) codecPrivate->GetBuffer();
		int twocc = p[0] | (p[1] << 8);
		
		for (int i = 0; kWavCodecIDs[i].cType; i++) {
			if (kWavCodecIDs[i].twocc == twocc)
				return kWavCodecIDs[i].cType;
		}
		return 'ms\0\0' | twocc;
		
	} else if (codecString == MKV_V_QT) {
		// QT compatibility mode, private info is the ImageDescription structure, big endian
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL || codecPrivate->GetSize() <= 4)
			return 0;
		
		// starts at the 4CC
		unsigned char *p = (unsigned char *) codecPrivate->GetBuffer();
		return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		
	} else {
		for (int i = 0; i < sizeof(kMatroskaCodecIDs) / sizeof(MatroskaQT_Codec); i++) {
			if (codecString == kMatroskaCodecIDs[i].mkvID)
				return kMatroskaCodecIDs[i].cType;
		}
	}
	return 0;
}
