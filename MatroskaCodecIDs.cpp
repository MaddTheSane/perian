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
#include <string>
using namespace std;
using namespace libmatroska;

ComponentResult DescExt_H264(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir)
{
	if (!tr_entry || !desc) return paramErr;
	ImageDescriptionHandle imgDesc = (ImageDescriptionHandle) desc;
	
	if (dir == kToSampleDescription) {
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL)
			return invalidAtomErr;
		
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

ComponentResult ASBDExt_AC3(KaxTrackEntry *tr_entry, AudioStreamBasicDescription *asbd)
{
	if (!tr_entry || !asbd) return paramErr;
	
	// Older AC3 codecs for QuickTime only claim the 'ms ' 4cc that DivX
	// defined for AVIs, so check to see if that's all we have and set the
	// 4cc appropriately based on decoder availbility.
	Component c = NULL;
	ComponentDescription cd = {0};
	cd.componentType = kSoundDecompressor;
	cd.componentSubType = kAudioFormatAC3;
	
	c = FindNextComponent(c, &cd);
	if (c != NULL) {
		// newer AC3 codecs claim this
		asbd->mFormatID = kAudioFormatAC3;
	} else {
		cd.componentSubType = kAudioFormatAC3MS;
		c = FindNextComponent(c, &cd);
		
		if (c != NULL) {
			// older AC3 codecs claim this
			asbd->mFormatID = kAudioFormatAC3MS;
			asbd->mChannelsPerFrame = 2;
		} else
			// if we don't have an AC3 codec, might as well use the "proper" 4cc
			asbd->mFormatID = kAudioFormatAC3;
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

ComponentResult ASBDExt_AAC(KaxTrackEntry *tr_entry, AudioStreamBasicDescription *asbd)
{
	if (!tr_entry || !asbd) return paramErr;
#if 0
	// newer Matroska files have the esds atom stored in the codec private
	// use it for our AudioStreamBasicDescription and AudioChannelLayout if possible
	KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
	
	if (codecPrivate != NULL) {
		// the magic cookie for AAC is the esds atom
		// but only the AAC-specific part; Apple seems to want the entire thing for this stuff
		// so this block doesn't really do anything at the moment
		magicCookie = (Ptr) codecPrivate->GetBuffer();
		cookieSize = codecPrivate->GetSize();
		
		err = AudioFormatGetProperty(kAudioFormatProperty_ASBDFromESDS,
									 cookieSize,
									 magicCookie,
									 &ioSize,
									 &asbd);
		if (err != noErr) 
			dprintf("MatroskaQT: Error creating ASBD from esds atom %ld\n", err);
		
		err = AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutFromESDS,
									 cookieSize,
									 magicCookie,
									 &acl_size,
									 &acl);
		if (err != noErr) {
			dprintf("MatroskaQT: Error creating ACL from esds atom %ld\n", err);
		} else {
			aclIsFromESDS = true;
		}
		
	} else {
		// if we don't have a esds atom, all we can do is get the profile of the AAC
		// and hope there isn't a custom channel configuration 
		// (how would we get the esds in that case?)
		asbd.mFormatFlags = GetAACProfile(tr_entry);
		dprintf("MatroskaQT: AAC track, but no esds atom\n");
	}
#endif
	return noErr;
}

long GetTrackLanguage(KaxTrackEntry *tr_entry) {
	KaxTrackLanguage *trLang = FindChild<KaxTrackLanguage>(*tr_entry);
	if (trLang != NULL) {
		string lang(*trLang);
		int i;
		
		for (i = 0; i < sizeof(MkvAndQTLanguagePairs) / sizeof(LanguagePair); i++) {
			if (lang.compare(MkvAndQTLanguagePairs[i].mkvLang) == 0)
				return MkvAndQTLanguagePairs[i].qtLang;
		}
	}
	return langUnspecified;
}
