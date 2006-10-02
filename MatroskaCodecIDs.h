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
    kSampleDescriptionExtensionTheora = 'XdxT'
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
	{ kSubFormatSSA, "S_TEXT/SSA" },
	{ kSubFormatASS, "S_TEXT/ASS" },
	{ kSubFormatUSF, "S_TEXT/USF" },
#endif
	{ kSubFormatUTF8, "S_TEXT/UTF8" },
	{ kSubFormatVobSub, "S_VOBSUB" },
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
	{ kSubFormatVobSub, DescExt_VobSub }
};



struct LanguagePair {
	char mkvLang[4];	// (ISO 639-2 3 char code)
	long qtLang;
};

// don't think there's a function already to do ISO 639-2 -> language code 
// that SetMediaLanguage accepts
static const struct LanguagePair MkvAndQTLanguagePairs[] = {
	{ "afr", langAfrikaans },
	{ "alb", langAlbanian },
	{ "amh", langAmharic },
	{ "ara", langArabic },
	{ "arm", langArmenian },
	{ "asm", langAssamese }, 
	{ "aym", langAymara },
	{ "aze", langAzerbaijani },
	{ "baq", langBasque },
	{ "ben", langBengali },
	{ "bre", langBreton },
	{ "bul", langBulgarian },
	{ "bur", langBurmese },
	{ "cat", langCatalan },
	{ "chi", langTradChinese },
	{ "cze", langCzech },
	{ "dan", langDanish },
	{ "dut", langDutch },
	{ "dzo", langDzongkha },
	{ "eng", langEnglish },
	{ "epo", langEsperanto },
	{ "est", langEstonian },
	{ "fao", langFaroese },
	{ "fin", langFinnish },
	{ "fre", langFrench },
	{ "geo", langGeorgian },
	{ "ger", langGerman },
	{ "gig", langGalician },
	{ "gla", langScottishGaelic },
	{ "gle", langIrishGaelic },
	{ "glv", langManxGaelic },
	{ "grc", langGreekAncient },
	{ "gre", langGreek },
	{ "grn", langGuarani },
	{ "guj", langGujarati },
	{ "heb", langHebrew },
	{ "hin", langHindi },
	{ "hmn", langHungarian },
	{ "ice", langIcelandic },
	{ "ind", langIndonesian },
	{ "ita", langItalian },
	{ "jav", langJavaneseRom },
	{ "jpn", langJapanese },
	{ "kal", langGreenlandic },
	{ "kan", langKannada },
	{ "kas", langKashmiri },
	{ "kaz", langKazakh },
	{ "khm", langKhmer },
	{ "kin", langKinyarwanda },
	{ "kir", langKirghiz },
	{ "kor", langKorean },
	{ "kur", langKurdish },
	{ "lao", langLao },
	{ "lat", langLatin },
	{ "lav", langLatvian },
	{ "lit", langLithuanian },
	{ "mac", langMacedonian },
	{ "mal", langMalayalam },
	{ "mar", langMarathi },
	{ "may", langMalayRoman },
	{ "mlg", langMalagasy },
	{ "mlt", langMaltese },
	{ "mol", langMoldavian },
	{ "mon", langMongolian },
	{ "nep", langNepali },
	{ "nob", langNorwegian },	// Norwegian Bokmål
	{ "nor", langNorwegian },
	{ "nno", langNynorsk },
	{ "nya", langNyanja },
	{ "ori", langOriya },
	{ "orm", langOromo },
	{ "pan", langPunjabi },
	{ "per", langPersian },
	{ "pol", langPolish },
	{ "por", langPortuguese },
	{ "que", langQuechua },
	{ "rum", langRomanian },
	{ "run", langRundi },
	{ "rus", langRussian },
	{ "san", langSanskrit },
	{ "scc", langSerbian },
	{ "scr", langCroatian },
	{ "sin", langSinhalese },
	{ "sit", langTibetan },		// Sino-Tibetan (Other)
	{ "slo", langSlovak },
	{ "slv", langSlovenian },
	{ "sme", langSami },
	{ "smi", langSami },		// Sami languages (Other)
	{ "snd", langSindhi },
	{ "som", langSomali },
	{ "spa", langSpanish },
	{ "sun", langSundaneseRom },
	{ "swa", langSwahili },
	{ "swe", langSwedish },
	{ "tam", langTamil },
	{ "tat", langTatar },
	{ "tel", langTelugu },
	{ "tgk", langTajiki },
	{ "tgl", langTagalog },
	{ "tha", langThai },
	{ "tib", langTibetan },
	{ "tir", langTigrinya },
	{ "tog", langTongan },		// Tonga (Nyasa)
	{ "tog", langTongan },		// Tonga (Tonga Islands)
	{ "tur", langTurkish },
	{ "tuk", langTurkmen },
	{ "uig", langUighur },
	{ "ukr", langUkrainian },
	{ "und", langUnspecified },
	{ "urd", langUrdu },
	{ "uzb", langUzbek },
	{ "vie", langVietnamese },
	{ "wel", langWelsh },
	{ "yid", langYiddish }
};

long GetTrackLanguage(KaxTrackEntry *tr_entry);

#endif