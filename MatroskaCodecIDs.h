/*
 *  MatroskaCodecIDs.h
 *
 *    MatroskaCodecIDs.h - Codec and language IDs for conversion between Matroska and QuickTime
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

#ifndef __MATROSKACODECIDS_H__
#define __MATROSKACODECIDS_H__

#include <QuickTime/QuickTime.h>
#include <AudioToolbox/AudioToolbox.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxTrackEntryData.h>
using namespace libmatroska;


// Description Extensions
enum {
    kCookieTypeOggSerialNo = 'oCtN'
};

// format specific cookie types
enum {
    kCookieTypeVorbisHeader = 'vCtH',
    kCookieTypeVorbisComments = 'vCt#',
    kCookieTypeVorbisCodebooks = 'vCtC',
	kCookieTypeVorbisFirstPageNo = 'vCtN'
};

enum {
    kCookieTypeSpeexHeader = 'sCtH',
    kCookieTypeSpeexComments = 'sCt#',
    kCookieTypeSpeexExtraHeader	= 'sCtX'
};

enum {
    kCookieTypeTheoraHeader = 'tCtH',
    kCookieTypeTheoraComments = 'tCt#',
    kCookieTypeTheoraCodebooks = 'tCtC'
};

enum {
    kCookieTypeFLACStreaminfo = 'fCtS',
    kCookieTypeFLACMetadata = 'fCtM'
};

enum {
	kSampleDescriptionExtensionTheora = 'XdxT',
	kSampleDescriptionExtensionVobSubIdx = '.IDX',
};


enum {
	// unofficial QuickTime FourCCs
    kAudioFormatXiphVorbis                  = 'XiVs',
    kAudioFormatXiphSpeex                   = 'XiSp',
    kAudioFormatXiphFLAC                    = 'XiFL',
    kVideoFormatXiphTheora                  = 'XiTh',
	kAudioFormatAC3MS                       = 0x6D732000,
	kVideoFormatMSMPEG4v3                   = 'MP43',
	
	kSubFormatUTF8                          = 'SRT ',
	kSubFormatSSA                           = 'SSA ',
	kSubFormatASS                           = 'ASS ',
	kSubFormatUSF                           = 'USF ',
	kSubFormatVobSub                        = 'SPU ',
	
	// the following 4CCs aren't official, nor have any decoder support
	kMPEG1VisualCodecType                   = 'mp1v',
	kMPEG2VisualCodecType                   = 'mp2v',
	kAudioFormatDTS                         = 'DTS ', 
	kAudioFormatTTA                         = 'TTA1',
	kAudioFormatWavepack                    = 'WVPK',
	kVideoFormatReal5                       = 'RV10',
	kVideoFormatRealG2                      = 'RV20',
	kVideoFormatReal8                       = 'RV30',
	kVideoFormatReal9                       = 'RV40',
	kAudioFormatReal1                       = '14_4',
	kAudioFormatReal2                       = '28_8',
	kAudioFormatRealCook                    = 'COOK',
	kAudioFormatRealSipro                   = 'SIPR',
	kAudioFormatRealLossless                = 'RALF',
	kAudioFormatRealAtrac3                  = 'ATRC'
};



// these CodecIDs need special handling since they correspond to many fourccs
#define MKV_V_MS "V_MS/VFW/FOURCC"
#define MKV_V_QT "V_QUICKTIME"

// these codecs have their profile as a part of the CodecID
#define MKV_A_PCM_BIG "A_PCM/INT/BIG"
#define MKV_A_PCM_LIT "A_PCM/INT/LIT"
#define MKV_A_PCM_FLOAT "A_PCM/FLOAT/IEEE"



// these functions modify the AudioStreamBasicDescription properly for the audio format
ComponentResult ASBDExt_AC3(KaxTrackEntry *tr_entry, AudioStreamBasicDescription *asbd);
ComponentResult ASBDExt_LPCM(KaxTrackEntry *tr_entry, AudioStreamBasicDescription *asbd);
ComponentResult ASBDExt_AAC(KaxTrackEntry *tr_entry, AudioStreamBasicDescription *asbd);

struct ASBDExtensionFunc {
	OSType cType;
	ComponentResult (*finishASBD) (KaxTrackEntry *tr_entry, AudioStreamBasicDescription *asbd);
};

static const struct ASBDExtensionFunc kMatroskaASBDExtensionFuncs[] = {
	{ kAudioFormatMPEG4AAC, ASBDExt_AAC },
	{ kAudioFormatAC3, ASBDExt_AC3 },
	{ kAudioFormatLinearPCM, ASBDExt_LPCM }
};


typedef enum {
	kToKaxTrackEntry,
	kToSampleDescription
} DescExtDirection;

// these functions set/read extensions to the ImageDescription/SoundDescription
ComponentResult DescExt_H264(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir);
ComponentResult DescExt_XiphVorbis(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir);
ComponentResult DescExt_XiphFLAC(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir);
ComponentResult DescExt_VobSub(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir);

struct CodecDescExtFunc {
	OSType cType;
	ComponentResult (*descExtension) (KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir);
};

static const struct CodecDescExtFunc kMatroskaSampleDescExtFuncs[] = {
	{ kH264CodecType, DescExt_H264 },
	{ kAudioFormatXiphVorbis, DescExt_XiphVorbis },
	{ kAudioFormatXiphFLAC, DescExt_XiphFLAC },
	{ kSubFormatVobSub, DescExt_VobSub },
};

short GetTrackLanguage(KaxTrackEntry *tr_entry);
FourCharCode GetFourCC(KaxTrackEntry *tr_entry);

#endif
