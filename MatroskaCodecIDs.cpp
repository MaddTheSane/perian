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
#include "MatroskaCodecIDs.h"
#include "CommonUtils.h"
#include "Codecprintf.h"
#include <string>

using namespace std;
using namespace libmatroska;

ComponentResult DescExt_H264(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir)
{
	if (!tr_entry || !desc) return paramErr;
	ImageDescriptionHandle imgDesc = (ImageDescriptionHandle) desc;
	
	if (dir == kToSampleDescription) {
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL) {
			// technically invalid file, assume that the h.264 is in VfW format (no pts, etc).
			(*desc)->dataFormat = 'H264';
			return noErr;
		}
		
		Handle imgDescExt = NewHandle(codecPrivate->GetSize());
		memcpy(*imgDescExt, codecPrivate->GetBuffer(), codecPrivate->GetSize());
		
		AddImageDescriptionExtension(imgDesc, imgDescExt, 'avcC');
		
		DisposeHandle((Handle) imgDescExt);
	}
	return noErr;
}

// xiph-qt expects these this sound extension to have been created from first 3 packets
// which are stored in CodecPrivate in Matroska
ComponentResult DescExt_XiphVorbis(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir)
{
	if (!tr_entry || !desc) return paramErr;
	SoundDescriptionHandle sndDesc = (SoundDescriptionHandle) desc;
	
	if (dir == kToSampleDescription) {
		Handle sndDescExt = NewHandle(0);
		unsigned char *privateBuf;
		int i;
		int numPackets;
		int *packetSizes;
		int offset = 1;
		UInt32 uid = 0;
		
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL)
			return invalidAtomErr;
		
		KaxTrackUID *trackUID = FindChild<KaxTrackUID>(*tr_entry);
		if (trackUID != NULL)
			uid = uint32(*trackUID);
		
		privateBuf = (unsigned char *) codecPrivate->GetBuffer();
		numPackets = privateBuf[0] + 1;
		packetSizes = (int *) NewPtrClear(sizeof(int) * numPackets);
		
		// get the sizes of the packets
		packetSizes[numPackets - 1] = codecPrivate->GetSize() - 1;
		int packetNum = 0;
		for (i = 1; packetNum < numPackets - 1; i++) {
			packetSizes[packetNum] += privateBuf[i];
			if (privateBuf[i] < 255) {
				packetSizes[numPackets - 1] -= packetSizes[packetNum];
				packetNum++;
			}
			offset++;
		}
		
		// first packet
		unsigned long serialnoatom[3] = { EndianU32_NtoB(sizeof(serialnoatom)), 
			EndianU32_NtoB(kCookieTypeOggSerialNo), 
			EndianU32_NtoB(uid) };
		unsigned long atomhead[2] = { EndianU32_NtoB(packetSizes[0] + sizeof(atomhead)), 
			EndianU32_NtoB(kCookieTypeVorbisHeader) };
		
		PtrAndHand(serialnoatom, sndDescExt, sizeof(serialnoatom)); //check errors?
		PtrAndHand(atomhead, sndDescExt, sizeof(atomhead)); //check errors?
		PtrAndHand(&privateBuf[offset], sndDescExt, packetSizes[0]);
		
		// second packet
		unsigned long atomhead2[2] = { EndianU32_NtoB(packetSizes[1] + sizeof(atomhead)), 
			EndianU32_NtoB(kCookieTypeVorbisComments) };
		PtrAndHand(atomhead2, sndDescExt, sizeof(atomhead2));
		PtrAndHand(&privateBuf[offset + packetSizes[0]], sndDescExt, packetSizes[1]);
		
		// third packet
		unsigned long atomhead3[2] = { EndianU32_NtoB(packetSizes[2] + sizeof(atomhead)), 
			EndianU32_NtoB(kCookieTypeVorbisCodebooks) };
		PtrAndHand(atomhead3, sndDescExt, sizeof(atomhead3));
		PtrAndHand(&privateBuf[offset + packetSizes[1] + packetSizes[0]], sndDescExt, packetSizes[2]);
		
		// add the extension
		unsigned long endAtom[2] = { EndianU32_NtoB(sizeof(endAtom)), 
			EndianU32_NtoB(kAudioTerminatorAtomType) };
		PtrAndHand(endAtom, sndDescExt, sizeof(endAtom));
		
		AddSoundDescriptionExtension(sndDesc, sndDescExt, siDecompressionParams);
		
		DisposePtr((Ptr)packetSizes);
		DisposeHandle(sndDescExt);
	}
	return noErr;
}

// xiph-qt expects these this sound extension to have been created in this way
// from the packets which are stored in the CodecPrivate element in Matroska
ComponentResult DescExt_XiphFLAC(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir)
{
	if (!tr_entry || !desc) return paramErr;
	SoundDescriptionHandle sndDesc = (SoundDescriptionHandle) desc;
	
	if (dir == kToSampleDescription) {
		Handle sndDescExt = NewHandle(0);
		unsigned char *privateBuf;
		int i;
		int numPackets;
		int *packetSizes;
		int offset = 1;
		UInt32 uid = 0;
		
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL)
			return invalidAtomErr;
		
		KaxTrackUID *trackUID = FindChild<KaxTrackUID>(*tr_entry);
		if (trackUID != NULL)
			uid = uint32(*trackUID);
		
		privateBuf = (unsigned char *) codecPrivate->GetBuffer();
		numPackets = privateBuf[0] + 1;
		packetSizes = (int *) NewPtrClear(sizeof(int) * numPackets);
		
		// get the sizes of the packets
		packetSizes[numPackets - 1] = codecPrivate->GetSize() - 1;
		int packetNum = 0;
		for (i = 1; packetNum < numPackets - 1; i++) {
			packetSizes[packetNum] += privateBuf[i];
			if (privateBuf[i] < 255) {
				packetSizes[numPackets - 1] -= packetSizes[packetNum];
				packetNum++;
			}
			offset++;
		}
		
		// first packet
		unsigned long serialnoatom[3] = { EndianU32_NtoB(sizeof(serialnoatom)), 
			EndianU32_NtoB(kCookieTypeOggSerialNo), 
			EndianU32_NtoB(uid) };
		unsigned long atomhead[2] = { EndianU32_NtoB(packetSizes[0] + sizeof(atomhead)), 
			EndianU32_NtoB(kCookieTypeFLACStreaminfo) };
		
		PtrAndHand(serialnoatom, sndDescExt, sizeof(serialnoatom)); //check errors?
		PtrAndHand(atomhead, sndDescExt, sizeof(atomhead)); //check errors?
		PtrAndHand(&privateBuf[offset], sndDescExt, packetSizes[0]);
		
		// metadata packets
		for (i = 1; i < numPackets; i++) {
			int j;
			int additionalOffset = 0;
			for (j = 0; j < i; j++) {
				additionalOffset += packetSizes[j];
			}
			unsigned long atomhead2[2] = { EndianU32_NtoB(packetSizes[1] + sizeof(atomhead)), 
				EndianU32_NtoB(kCookieTypeFLACMetadata) };
			PtrAndHand(atomhead2, sndDescExt, sizeof(atomhead2));
			PtrAndHand(&privateBuf[offset + additionalOffset], sndDescExt, packetSizes[1]);
		}
		// add the extension
		unsigned long endAtom[2] = { EndianU32_NtoB(sizeof(endAtom)), 
			EndianU32_NtoB(kAudioTerminatorAtomType) };
		PtrAndHand(endAtom, sndDescExt, sizeof(endAtom));
		
		AddSoundDescriptionExtension(sndDesc, sndDescExt, siDecompressionParams);
		
		DisposePtr((Ptr)packetSizes);
		DisposeHandle(sndDescExt);
	}
	return noErr;
}

ComponentResult DescExt_XiphTheora(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir)
{
	if (!tr_entry || !desc) return paramErr;
	ImageDescriptionHandle imgDesc = (ImageDescriptionHandle) desc;
	
	if (dir == kToSampleDescription) {
		Handle imgDescExt = NewHandle(0);
		unsigned char *privateBuf;
		int i;
		int numPackets;
		int *packetSizes;
		int offset = 1;
		UInt32 uid = 0;
		
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL)
			return invalidAtomErr;
		
		KaxTrackUID *trackUID = FindChild<KaxTrackUID>(*tr_entry);
		if (trackUID != NULL)
			uid = uint32(*trackUID);
		
		privateBuf = (unsigned char *) codecPrivate->GetBuffer();
		numPackets = privateBuf[0] + 1;
		packetSizes = (int *) NewPtrClear(sizeof(int) * numPackets);
		
		// get the sizes of the packets
		packetSizes[numPackets - 1] = codecPrivate->GetSize() - 1;
		int packetNum = 0;
		for (i = 1; packetNum < numPackets - 1; i++) {
			packetSizes[packetNum] += privateBuf[i];
			if (privateBuf[i] < 255) {
				packetSizes[numPackets - 1] -= packetSizes[packetNum];
				packetNum++;
			}
			offset++;
		}
		
		// first packet
		unsigned long serialnoatom[3] = { EndianU32_NtoB(sizeof(serialnoatom)), 
			EndianU32_NtoB(kCookieTypeOggSerialNo), EndianU32_NtoB(uid) };
		unsigned long atomhead[2] = { EndianU32_NtoB(packetSizes[0] + sizeof(atomhead)), 
			EndianU32_NtoB(kCookieTypeTheoraHeader) };
		
		PtrAndHand(serialnoatom, imgDescExt, sizeof(serialnoatom)); //check errors?
		PtrAndHand(atomhead, imgDescExt, sizeof(atomhead)); //check errors?
		PtrAndHand(&privateBuf[offset], imgDescExt, packetSizes[0]);
		
		// second packet
		unsigned long atomhead2[2] = { EndianU32_NtoB(packetSizes[1] + sizeof(atomhead)), 
			EndianU32_NtoB(kCookieTypeTheoraComments) };
		PtrAndHand(atomhead2, imgDescExt, sizeof(atomhead2));
		PtrAndHand(&privateBuf[offset + packetSizes[0]], imgDescExt, packetSizes[1]);
		
		// third packet
		unsigned long atomhead3[2] = { EndianU32_NtoB(packetSizes[2] + sizeof(atomhead)), 
			EndianU32_NtoB(kCookieTypeTheoraCodebooks) };
		PtrAndHand(atomhead3, imgDescExt, sizeof(atomhead3));
		PtrAndHand(&privateBuf[offset + packetSizes[1] + packetSizes[0]], imgDescExt, packetSizes[2]);
		
		// add the extension
		unsigned long endAtom[2] = { EndianU32_NtoB(sizeof(endAtom)), EndianU32_NtoB(kAudioTerminatorAtomType) };
		PtrAndHand(endAtom, imgDescExt, sizeof(endAtom));
		
		AddImageDescriptionExtension(imgDesc, imgDescExt, kTheoraDescExtension);
		
		DisposePtr((Ptr)packetSizes);
		DisposeHandle(imgDescExt);
	}
	return noErr;
}

// VobSub stores the .idx file in the codec private, pass it as an .IDX extension
ComponentResult DescExt_VobSub(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir)
{
	if (!tr_entry || !desc) return paramErr;
	ImageDescriptionHandle imgDesc = (ImageDescriptionHandle) desc;
	
	if (dir == kToSampleDescription) {
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL)
			return invalidAtomErr;
		
		Handle imgDescExt = NewHandle(codecPrivate->GetSize());
		memcpy(*imgDescExt, codecPrivate->GetBuffer(), codecPrivate->GetSize());
		
		AddImageDescriptionExtension(imgDesc, imgDescExt, kVobSubIdxExtension);
		
		DisposeHandle((Handle) imgDescExt);
	}
	return noErr;
}

ComponentResult DescExt_SSA(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir)
{
	if (!tr_entry || !desc) return paramErr;
	ImageDescriptionHandle imgDesc = (ImageDescriptionHandle) desc;
	
	if (dir == kToSampleDescription) {
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL)
			return invalidAtomErr;
		
		Handle imgDescExt = NewHandle(codecPrivate->GetSize());
		memcpy(*imgDescExt, codecPrivate->GetBuffer(), codecPrivate->GetSize());
		
		AddImageDescriptionExtension(imgDesc, imgDescExt, kSubFormatSSA);
		
		DisposeHandle((Handle) imgDescExt);
	}
	return noErr;
}

ComponentResult DescExt_Real(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir)
{
	if (!tr_entry || !desc) return paramErr;
	ImageDescriptionHandle imgDesc = (ImageDescriptionHandle) desc;
	
	if (dir == kToSampleDescription) {
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL)
			return invalidAtomErr;
		
		Handle imgDescExt = NewHandle(codecPrivate->GetSize());
		memcpy(*imgDescExt, codecPrivate->GetBuffer(), codecPrivate->GetSize());
		
		AddImageDescriptionExtension(imgDesc, imgDescExt, kRealVideoExtension);
		
		DisposeHandle((Handle) imgDescExt);
	}
	return noErr;
}

// the esds atom creation is based off of the routines for it in ffmpeg's movenc.c
static unsigned int descrLength(unsigned int len)
{
    int i;
    for(i=1; len>>(7*i); i++);
    return len + 1 + i;
}

static uint8_t* putDescr(uint8_t *buffer, int tag, unsigned int size)
{
    int i= descrLength(size) - size - 2;
    *buffer++ = tag;
    for(; i>0; i--)
       *buffer++ = (size>>(7*i)) | 0x80;
    *buffer++ = size & 0x7F;
	return buffer;
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

uint8_t *CreateEsdsFromSetupData(uint8_t *codecPrivate, size_t vosLen, size_t *esdsLen, int trackID, bool audio)
{
	int decoderSpecificInfoLen = vosLen ? descrLength(vosLen) : 0;
	
	*esdsLen = descrLength(3 + descrLength(13 + decoderSpecificInfoLen) + descrLength(1));
	uint8_t *esds = (uint8_t*)malloc(*esdsLen);
	UInt8 *pos = (UInt8 *) esds;
		
	// ES Descriptor
	pos = putDescr(pos, 0x03, 3 + descrLength(13 + decoderSpecificInfoLen) + descrLength(1));
	pos = write_int16(pos, EndianS16_NtoB(trackID));
	*pos++ = 0;		// no flags
	
	// DecoderConfig descriptor
	pos = putDescr(pos, 0x04, 13 + decoderSpecificInfoLen);
	
	// Object type indication, see http://gpac.sourceforge.net/tutorial/mediatypes.htm
	if (audio)
		*pos++ = 0x40;		// aac
	else
		*pos++ = 0x20;		// mpeg4 part 2
	
	// streamtype
	if (audio)
		*pos++ = 0x15;
	else
		*pos++ = 0x11;
	
	// 3 bytes: buffersize DB (not sure how to get easily)
	*pos++ = 0;
	pos = write_int16(pos, 0);
	
	// max bitrate, not sure how to get easily
	pos = write_int32(pos, 0);
	
	// vbr
	pos = write_int32(pos, 0);
	
	if (vosLen) {
		pos = putDescr(pos, 0x05, vosLen);
		pos = write_data(pos, codecPrivate, vosLen);
	}
	
	// SL descriptor
	pos = putDescr(pos, 0x06, 1);
	*pos++ = 0x02;
	
	return esds;
}

static Handle CreateEsdsExt(KaxTrackEntry *tr_entry, bool audio)
{
	KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
	KaxTrackNumber *trackNum = FindChild<KaxTrackNumber>(*tr_entry);
	
	int vosLen = codecPrivate ? codecPrivate->GetSize() : 0;
	int trackID = trackNum ? uint16(*trackNum) : 1;
    uint8_t *vosBuf = codecPrivate ? codecPrivate->GetBuffer() : NULL;
	size_t esdsLen;

	Handle esdsExt = NewHandleClear(4);
	uint8_t *esds = CreateEsdsFromSetupData(vosBuf, vosLen, &esdsLen, trackID, audio);
	
	PtrAndHand(esds, esdsExt, esdsLen);
	free((char*)esds);
	
	return esdsExt;
}

ComponentResult DescExt_mp4v(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir)
{
	if (!tr_entry || !desc) return paramErr;
	ImageDescriptionHandle imgDesc = (ImageDescriptionHandle) desc;
	
	if (dir == kToSampleDescription) {
		Handle imgDescExt = CreateEsdsExt(tr_entry, false);
		
		if (imgDescExt == NULL) {
			// missing extradata -> probably missing pts too, so force VfW mode
			(*desc)->dataFormat = 'XVID';
			return noErr;
		}
		
		AddImageDescriptionExtension(imgDesc, imgDescExt, 'esds');
		
		DisposeHandle((Handle) imgDescExt);
	}
	return noErr;
}

ComponentResult DescExt_aac(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir)
{
	if (!tr_entry || !desc) return paramErr;
	SoundDescriptionHandle sndDesc = (SoundDescriptionHandle) desc;
	
	if (dir == kToSampleDescription) {
		Handle esdsExt = CreateEsdsExt(tr_entry, true);
				
		QTSoundDescriptionSetProperty(sndDesc, kQTPropertyClass_SoundDescription, kQTSoundDescriptionPropertyID_MagicCookie, GetHandleSize(esdsExt) - 4, (*esdsExt) + 4);
		DisposeHandle((Handle) esdsExt);
	}

	return noErr;
}

ComponentResult ASBDExt_LPCM(KaxTrackEntry *tr_entry, AudioStreamBasicDescription *asbd)
{
	if (!tr_entry || !asbd) return paramErr;
	
	KaxCodecID *tr_codec = FindChild<KaxCodecID>(*tr_entry);
	if (!tr_codec) return paramErr;
	string codecid(*tr_codec);
	
	// is this correct here?
	asbd->mBytesPerPacket = asbd->mFramesPerPacket = asbd->mChannelsPerFrame * asbd->mBitsPerChannel / 8;
	
	if (codecid == MKV_A_PCM_BIG)
		asbd->mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
	// not sure about signedness; I think it should all be unsigned, 
	// but saying 16 bits unsigned doesn't work, nor does signed 8 bits
	else if (codecid == MKV_A_PCM_LIT && asbd->mBitsPerChannel > 8)
		asbd->mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
	else if (codecid == MKV_A_PCM_FLOAT)
		asbd->mFormatFlags |= kLinearPCMFormatFlagIsFloat;
	
	return noErr;
}

static void PrintCoreAudioError(const char *msg, OSStatus err)
{
	OSStatus swap_err = EndianU32_NtoB(err);
	Codecprintf(NULL, "%s '%.4s'\n", msg, &swap_err);
}

ComponentResult ASBDExt_AAC(KaxTrackEntry *tr_entry, AudioStreamBasicDescription *asbd, AudioChannelLayout *acl)
{
	if (!tr_entry || !asbd) return paramErr;
	// newer Matroska files have the esds atom stored in the codec private
	// use it for our AudioStreamBasicDescription and AudioChannelLayout if possible
	Handle esdsExt = CreateEsdsExt(tr_entry, true);
	OSStatus err = noErr;
	
	if (esdsExt) {
		// the magic cookie for AAC is the esds atom
		// but only the AAC-specific part; Apple seems to want the entire thing for this stuff
		// so this block doesn't really do anything at the moment
		// actually, it wants to skip the 4 byte version at the front
		Ptr magicCookie = *esdsExt + 4;
		ByteCount cookieSize = GetHandleSize(esdsExt) - 4, asbdSize = sizeof(*asbd), aclSize = sizeof(*acl);
		
		err = AudioFormatGetProperty(kAudioFormatProperty_ASBDFromESDS,
									 cookieSize,
									 magicCookie,
									 &asbdSize,
									 asbd);
		if (err != noErr) 
			PrintCoreAudioError("MatroskaQT: Error creating ASBD from AAC esds", err);
		//else Codecprintf(NULL, "AAC: profile %d, nch %d\n", asbd->mFormatFlags, asbd->mChannelsPerFrame);
		
		err = AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutFromESDS,
									 cookieSize,
									 magicCookie,
									 &aclSize,
									 acl);
		if (err != noErr) 
			PrintCoreAudioError("MatroskaQT: Error creating ACL from AAC esds", err);
		
		DisposeHandle(esdsExt);
	} else {
		// if we don't have a esds atom, all we can do is get the profile of the AAC
		// and hope there isn't a custom channel configuration 
		// (how would we get the esds in that case?)
		// FIXME check for codec string
		// and OutputSamplingFrequency (for SBR)
		
		asbd->mFormatFlags = kMPEG4Object_AAC_LC;
		Codecprintf(NULL, "MatroskaQT: AAC track, but no esds atom\n");
	}

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
		
		Handle imgDescExt = NewHandle(codecPrivate.GetSize());
		memcpy(*imgDescExt, codecPrivate.GetBuffer(), codecPrivate.GetSize());
		
		AddImageDescriptionExtension((ImageDescriptionHandle) desc, imgDescExt, 'strf');
		
	} else if (codecString == MKV_A_MS) {
		// WAVFORMATEX is stored in the private data, and some codecs (WMA) need it to decode
		KaxCodecPrivate & codecPrivate = GetChild<KaxCodecPrivate>(*tr_entry);
		
		QTSoundDescriptionSetProperty((SoundDescriptionHandle) desc, 
		                              kQTPropertyClass_SoundDescription, 
		                              kQTSoundDescriptionPropertyID_MagicCookie, 
		                              codecPrivate.GetSize(), codecPrivate.GetBuffer());
		
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
		switch ((*desc)->dataFormat) {
			case kH264CodecType:
				return DescExt_H264(tr_entry, desc, dir);
				
			case kAudioFormatXiphVorbis:
				return DescExt_XiphVorbis(tr_entry, desc, dir);
				
//			case kAudioFormatXiphFLAC:
//				return DescExt_XiphFLAC(tr_entry, desc, dir);
				
			case kVideoFormatXiphTheora:
				return DescExt_XiphTheora(tr_entry, desc, dir);
				
			case kSubFormatVobSub:
				return DescExt_VobSub(tr_entry, desc, dir);
				
			case kVideoFormatReal5:
			case kVideoFormatRealG2:
			case kVideoFormatReal8:
			case kVideoFormatReal9:
				return DescExt_Real(tr_entry, desc, dir);
				
			case kMPEG4VisualCodecType:
				return DescExt_mp4v(tr_entry, desc, dir);
				
			case kAudioFormatMPEG4AAC:
			case kMPEG4AudioFormat:
				return DescExt_aac(tr_entry, desc, dir);
				
			case kSubFormatSSA:
				return DescExt_SSA(tr_entry, desc, dir);
		}
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
		}
	}
	
	return acl;
}


ComponentResult MkvFinishAudioDescriptions(KaxTrackEntry *tr_entry, AudioStreamBasicDescription *asbd, AudioChannelLayout *acl)
{
	switch (asbd->mFormatID) {
		case kAudioFormatMPEG4AAC:
			return ASBDExt_AAC(tr_entry, asbd, acl);
			
		case kAudioFormatLinearPCM:
			return ASBDExt_LPCM(tr_entry, asbd);
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
	{ kAudioFormatAC3MS, 0x2000 },
	{ kAudioFormatDTS, 0x2001 },
	{ kAudioFormatMPEG4AAC, 0xff },
	{ kAudioFormatXiphFLAC, 0xf1ac },
	{ 0, 0 }
};

typedef struct {
	OSType cType;
	char *mkvID;
} MatroskaQT_Codec;

// the first matching pair is used for conversion
static const MatroskaQT_Codec kMatroskaCodecIDs[] = {
	{ kRawCodecType, "V_UNCOMPRESSED" },
	{ kMPEG4VisualCodecType, "V_MPEG4/ISO/ASP" },
	{ kMPEG4VisualCodecType, "V_MPEG4/ISO/SP" },
	{ kMPEG4VisualCodecType, "V_MPEG4/ISO/AP" },
	{ kH264CodecType, "V_MPEG4/ISO/AVC" },
	{ kVideoFormatMSMPEG4v3, "V_MPEG4/MS/V3" },
	{ kMPEG1VisualCodecType, "V_MPEG1" },
	{ kMPEG2VisualCodecType, "V_MPEG2" },
	{ kVideoFormatReal5, "V_REAL/RV10" },
	{ kVideoFormatRealG2, "V_REAL/RV20" },
	{ kVideoFormatReal8, "V_REAL/RV30" },
	{ kVideoFormatReal9, "V_REAL/RV40" },
	{ kVideoFormatXiphTheora, "V_THEORA" },
	{ kVideoFormatSnow, "V_SNOW" },
	
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
	{ kAudioFormatAC3MS, "A_AC3" },
	{ kAudioFormatAC3MS, "A_AC3" },
	// anything special for these two?
	{ kAudioFormatAC3MS, "A_AC3/BSID9" },
	{ kAudioFormatAC3MS, "A_AC3/BSID10" },
	{ kAudioFormatXiphVorbis, "A_VORBIS" },
	{ kAudioFormatXiphFLAC, "A_FLAC" },
	{ kAudioFormatLinearPCM, "A_PCM/INT/LIT" },
	{ kAudioFormatLinearPCM, "A_PCM/INT/BIG" },
	{ kAudioFormatLinearPCM, "A_PCM/FLOAT/IEEE" },
	{ kAudioFormatDTS, "A_DTS" },
	{ kAudioFormatTTA, "A_TTA1" },
	{ kAudioFormatWavepack, "A_WAVPACK4" },
	{ kAudioFormatReal1, "A_REAL/14_4" },
	{ kAudioFormatReal2, "A_REAL/28_8" },
	{ kAudioFormatRealCook, "A_REAL/COOK" },
	{ kAudioFormatRealSipro, "A_REAL/SIPR" },
	{ kAudioFormatRealLossless, "A_REAL/RALF" },
	{ kAudioFormatRealAtrac3, "A_REAL/ATRC" },
	
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
		if (codecPrivate == NULL)
			return 0;
		
		// offset to biCompression in BITMAPINFO
		unsigned char *p = (unsigned char *) codecPrivate->GetBuffer() + 16;
		return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		
	} else if (codecString == MKV_A_MS) {
		// acm compatibility mode, twocc is in private info
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL)
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
		if (codecPrivate == NULL)
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
