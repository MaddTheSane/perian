/*
 * CommonUtils.h
 * Created by David Conrad on 10/13/06.
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


#include <Carbon/Carbon.h>
#include <pthread.h>
#include <dlfcn.h>
#include <fnmatch.h>

#include <libavcodec/avcodec.h>
#include <libavcodec/bytestream.h>
#include "CommonUtils.h"

typedef struct LanguageTriplet {
	char twoChar[3];
	char threeChar[4];	// (ISO 639-2 3 char code)
	short qtLang;
} LanguageTriplet;

// don't think there's a function already to do ISO 639-1/2 -> language code 
// that SetMediaLanguage() accepts
static const LanguageTriplet ISO_QTLanguages[] = {
	{ "",   "und", langUnspecified },
	{ "af", "afr", langAfrikaans },
	{ "sq", "alb", langAlbanian },
	{ "sq", "sqi", langAlbanian },
	{ "am", "amh", langAmharic },
	{ "ar", "ara", langArabic },
	{ "hy", "arm", langArmenian },
	{ "hy", "hye", langArmenian },
	{ "as", "asm", langAssamese }, 
	{ "ay", "aym", langAymara },
	{ "az", "aze", langAzerbaijani },
	{ "eu", "baq", langBasque },
	{ "eu", "eus", langBasque },
	{ "bn", "ben", langBengali },
	{ "br", "bre", langBreton },
	{ "bg", "bul", langBulgarian },
	{ "my", "bur", langBurmese },
	{ "my", "mya", langBurmese },
	{ "ca", "cat", langCatalan },
	{ "zh", "chi", langTradChinese },
	{ "zh", "zho", langTradChinese },
	{ "cs", "cze", langCzech },
	{ "cs", "ces", langCzech },
	{ "da", "dan", langDanish },
	{ "nl", "dut", langDutch },
	{ "nl", "nld", langDutch },
	{ "dz", "dzo", langDzongkha },
	{ "en", "eng", langEnglish },
	{ "eo", "epo", langEsperanto },
	{ "et", "est", langEstonian },
	{ "fo", "fao", langFaroese },
	{ "fi", "fin", langFinnish },
	{ "fr", "fre", langFrench },
	{ "fr", "fra", langFrench },
	{ "ka", "geo", langGeorgian },
	{ "ka", "kat", langGeorgian },
	{ "de", "ger", langGerman },
	{ "de", "deu", langGerman },
	{ "gl", "glg", langGalician },
	{ "gd", "gla", langScottishGaelic },
	{ "ga", "gle", langIrishGaelic },
	{ "gv", "glv", langManxGaelic },
	{ "",   "grc", langGreekAncient },
	{ "el", "gre", langGreek },
	{ "el", "ell", langGreek },
	{ "gn", "grn", langGuarani },
	{ "gu", "guj", langGujarati },
	{ "he", "heb", langHebrew },
	{ "hi", "hin", langHindi },
	{ "hu", "hun", langHungarian },
	{ "is", "ice", langIcelandic },
	{ "is", "isl", langIcelandic },
	{ "id", "ind", langIndonesian },
	{ "it", "ita", langItalian },
	{ "jv", "jav", langJavaneseRom },
	{ "ja", "jpn", langJapanese },
	{ "kl", "kal", langGreenlandic },
	{ "kn", "kan", langKannada },
	{ "ks", "kas", langKashmiri },
	{ "kk", "kaz", langKazakh },
	{ "km", "khm", langKhmer },
	{ "rw", "kin", langKinyarwanda },
	{ "ky", "kir", langKirghiz },
	{ "ko", "kor", langKorean },
	{ "ku", "kur", langKurdish },
	{ "lo", "lao", langLao },
	{ "la", "lat", langLatin },
	{ "lv", "lav", langLatvian },
	{ "lt", "lit", langLithuanian },
	{ "mk", "mac", langMacedonian },
	{ "mk", "mkd", langMacedonian },
	{ "ml", "mal", langMalayalam },
	{ "mr", "mar", langMarathi },
	{ "ms", "may", langMalayRoman },
	{ "ms", "msa", langMalayRoman },
	{ "mg", "mlg", langMalagasy },
	{ "mt", "mlt", langMaltese },
	{ "mo", "mol", langMoldavian },
	{ "mn", "mon", langMongolian },
	{ "ne", "nep", langNepali },
	{ "nb", "nob", langNorwegian },		// Norwegian Bokmal
	{ "no", "nor", langNorwegian },
	{ "nn", "nno", langNynorsk },
	{ "ny", "nya", langNyanja },
	{ "or", "ori", langOriya },
	{ "om", "orm", langOromo },
	{ "pa", "pan", langPunjabi },
	{ "fa", "per", langPersian },
	{ "fa", "fas", langPersian },
	{ "pl", "pol", langPolish },
	{ "pt", "por", langPortuguese },
	{ "qu", "que", langQuechua },
	{ "ro", "rum", langRomanian },
	{ "ro", "ron", langRomanian },
	{ "rn", "run", langRundi },
	{ "ru", "rus", langRussian },
	{ "sa", "san", langSanskrit },
	{ "sr", "scc", langSerbian },
	{ "sr", "srp", langSerbian },
	{ "hr", "scr", langCroatian },
	{ "hr", "hrv", langCroatian },
	{ "si", "sin", langSinhalese },
	{ "",   "sit", langTibetan },		// Sino-Tibetan (Other)
	{ "sk", "slo", langSlovak },
	{ "sk", "slk", langSlovak },
	{ "sl", "slv", langSlovenian },
	{ "se", "sme", langSami },
	{ "",   "smi", langSami },			// Sami languages (Other)
	{ "sd", "snd", langSindhi },
	{ "so", "som", langSomali },
	{ "es", "spa", langSpanish },
	{ "su", "sun", langSundaneseRom },
	{ "sw", "swa", langSwahili },
	{ "sv", "swe", langSwedish },
	{ "ta", "tam", langTamil },
	{ "tt", "tat", langTatar },
	{ "te", "tel", langTelugu },
	{ "tg", "tgk", langTajiki },
	{ "tl", "tgl", langTagalog },
	{ "th", "tha", langThai },
	{ "bo", "tib", langTibetan },
	{ "bo", "bod", langTibetan },
	{ "ti", "tir", langTigrinya },
	{ "",   "tog", langTongan },		// Tonga (Nyasa, Tonga Islands)
	{ "tr", "tur", langTurkish },
	{ "tk", "tuk", langTurkmen },
	{ "ug", "uig", langUighur },
	{ "uk", "ukr", langUkrainian },
	{ "ur", "urd", langUrdu },
	{ "uz", "uzb", langUzbek },
	{ "vi", "vie", langVietnamese },
	{ "cy", "wel", langWelsh },
	{ "cy", "cym", langWelsh },
	{ "yi", "yid", langYiddish }
};

short ISO639_1ToQTLangCode(const char *lang)
{
	int i;
	
	if (strlen(lang) != 2)
		return langUnspecified;
	
	for (i = 0; i < sizeof(ISO_QTLanguages) / sizeof(LanguageTriplet); i++) {
		if (strcasecmp(lang, ISO_QTLanguages[i].twoChar) == 0)
			return ISO_QTLanguages[i].qtLang;
	}
	
	return langUnspecified;
}

short ISO639_2ToQTLangCode(const char *lang)
{
	int i;
	
	if (strlen(lang) != 3)
		return langUnspecified;
	
	for (i = 0; i < sizeof(ISO_QTLanguages) / sizeof(LanguageTriplet); i++) {
		if (strcasecmp(lang, ISO_QTLanguages[i].threeChar) == 0)
			return ISO_QTLanguages[i].qtLang;
	}
	
	return langUnspecified;
}


#define MP4ESDescrTag                   0x03
#define MP4DecConfigDescrTag            0x04
#define MP4DecSpecificDescrTag          0x05

// based off of mov_mp4_read_descr_len from mov.c in ffmpeg's libavformat
static unsigned readDescrLen(GetByteContext *g)
{
	unsigned len = 0;
	int count = 4;
	while (count--) {
		uint8_t c = bytestream2_get_byte(g);
		len = (len << 7) | (c & 0x7f);
		if (!(c & 0x80))
			break;
	}
	return len;
}

// based off of mov_mp4_read_descr from mov.c in ffmpeg's libavformat
static unsigned readDescr(GetByteContext *g, int *tag)
{
	*tag = bytestream2_get_byte(g);
	return readDescrLen(g);
}

// based off of mov_read_esds from mov.c in ffmpeg's libavformat
ComponentResult ReadESDSDescExt(Handle descExt, UInt8 **buffer, int *size)
{
	UInt8 *esds = (UInt8 *)*descExt;
	UInt8 *extradata;
	int tag, len;

	GetByteContext g;
	bytestream2_init(&g, esds, GetHandleSize(descExt));
	
	bytestream2_skip(&g, 4); // version + flags
	readDescr(&g, &tag);
	bytestream2_skip(&g, 2); // ID
	if (tag == MP4ESDescrTag)
		bytestream2_skip(&g, 1); // priority
	
	readDescr(&g, &tag);
	if (tag == MP4DecConfigDescrTag) {
		bytestream2_skip(&g, 1);	// object type id
		bytestream2_skip(&g, 1);	// stream type
		bytestream2_skip(&g, 3);	// buffer size db
		bytestream2_skip(&g, 4);	// max bitrate
		bytestream2_skip(&g, 4);	// average bitrate
		
		len = readDescr(&g, &tag);
		if (tag == MP4DecSpecificDescrTag) {
			extradata = av_mallocz(len + FF_INPUT_BUFFER_PADDING_SIZE);
			if (extradata) {
				if (bytestream2_get_buffer(&g, extradata, len) != len) {
					av_free(extradata);
					return paramErr;
				}
				
				*buffer = extradata;
				*size = len;
			}
		}
	}
	
	return noErr;
}

int isImageDescriptionExtensionPresent(ImageDescriptionHandle desc, FourCharCode type)
{
	ImageDescriptionPtr d = *desc;
	uint8_t *p = (uint8_t *)d;
	
	GetByteContext g;
	bytestream2_init(&g, p, GetHandleSize((Handle)desc));
	bytestream2_skip(&g, sizeof(ImageDescription));
	
	//read next description, need 8 bytes for size and type
	while(bytestream2_get_bytes_left(&g))
	{
		unsigned len;
		FourCharCode rtype;
		
		len   = bytestream2_get_be32(&g);
		rtype = bytestream2_get_be32(&g);
		
		if(rtype == type && (len - 8) <= bytestream2_get_bytes_left(&g))
			return 1;
		
		bytestream2_skip(&g, len - 8);
	}
	return 0;
}

static const CFStringRef defaultFrameDroppingList[] = {
	CFSTR("Finder"),
	CFSTR("Front Row"),
	CFSTR("Movie Time"),
	CFSTR("Movist"),
	CFSTR("NicePlayer"),
	CFSTR("QTKitServer"),
	CFSTR("QuickTime Player"),
	CFSTR("Spiral")
};

static const CFStringRef defaultTransparentSubtitleList_10_6[] = {
	CFSTR("CoreMediaAuthoringSessionHelper"),
	CFSTR("CoreMediaAuthoringSourcePropertyHelper"),
	CFSTR("Front Row"),
	CFSTR("QTPlayerHelper")
};

static const CFStringRef defaultTransparentSubtitleList_10_5[] = {
	CFSTR("Front Row")
};

static const CFStringRef defaultForcedAppList[] = {
	CFSTR("iChat")
};

static int findNameInList(CFStringRef loadingApp, const CFStringRef *names, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		if (CFGetTypeID(names[i]) != CFStringGetTypeID())
			continue;
		if (CFStringCompare(loadingApp, names[i], 0) == kCFCompareEqualTo) return 1;
	}

	return 0;
}

static CFDictionaryRef copyMyProcessInformation()
{
	ProcessSerialNumber myProcess;
	GetCurrentProcess(&myProcess);
	CFDictionaryRef processInformation;
	
	processInformation = ProcessInformationCopyDictionary(&myProcess, kProcessDictionaryIncludeAllInformationMask);
	return processInformation;
}

static CFStringRef copyProcessName(CFDictionaryRef processInformation)
{
	CFStringRef path = CFDictionaryGetValue(processInformation, kCFBundleExecutableKey);
	CFRange entireRange = CFRangeMake(0, CFStringGetLength(path)), basename;
	
	CFStringFindWithOptions(path, CFSTR("/"), entireRange, kCFCompareBackwards, &basename);
	
	basename.location += 1; //advance past "/"
	basename.length = entireRange.length - basename.location;
	
	CFStringRef myProcessName = CFStringCreateWithSubstring(NULL, path, basename);
	return myProcessName;
}

static int isApplicationNameInList(CFStringRef prefOverride, const CFStringRef *defaultList, unsigned int defaultListCount)
{
	CFDictionaryRef processInformation = copyMyProcessInformation();
	
	if (!processInformation)
		return FALSE;
	
	CFArrayRef list = CopyPreferencesValueTyped(prefOverride, CFArrayGetTypeID());
	CFStringRef myProcessName = copyProcessName(processInformation);
	int ret;
	
	if (list) {
		int count = CFArrayGetCount(list);
		CFStringRef names[count];
		
		CFArrayGetValues(list, CFRangeMake(0, count), (void *)names);
		ret = findNameInList(myProcessName, names, count);
		CFRelease(list);
	} else {
		ret = findNameInList(myProcessName, defaultList, defaultListCount);
	}
	CFRelease(myProcessName);
	CFRelease(processInformation);
	
	return ret;
}

int IsFrameDroppingEnabled()
{
	static int enabled = -1;
	
	if (enabled == -1)
		enabled = isApplicationNameInList(CFSTR("FrameDroppingWhiteList"),
										  defaultFrameDroppingList,
										  sizeof(defaultFrameDroppingList)/sizeof(defaultFrameDroppingList[0]));
	return enabled;
}

int IsForcedDecodeEnabled()
{
	static int forced = -1;
	
	if(forced == -1)
		forced = isApplicationNameInList(CFSTR("ForcePerianAppList"),
										 defaultForcedAppList,
										 sizeof(defaultForcedAppList)/sizeof(defaultForcedAppList[0]));
	return forced;
}

static int GetSystemMinorVersion()
{
	static SInt32 minorVersion = -1;
	if (minorVersion == -1)
		Gestalt(gestaltSystemVersionMinor, &minorVersion);
	
	return minorVersion;
}

static int GetSystemMicroVersion()
{
	static SInt32 microVersion = -1;
	if (microVersion == -1)
		Gestalt(gestaltSystemVersionBugFix, &microVersion);
	
	return microVersion;
}

int IsTransparentSubtitleHackEnabled()
{
	static int forced = -1;
	
	if(forced == -1)
	{
		int minorVersion = GetSystemMinorVersion();
				
		if (minorVersion == 5)
			forced = isApplicationNameInList(CFSTR("TransparentModeSubtitleAppList"),
											 defaultTransparentSubtitleList_10_5,
											 sizeof(defaultTransparentSubtitleList_10_5)/sizeof(defaultTransparentSubtitleList_10_5[0]));
		else if (minorVersion == 6)
			forced = isApplicationNameInList(CFSTR("TransparentModeSubtitleAppList"),
											 defaultTransparentSubtitleList_10_6,
											 sizeof(defaultTransparentSubtitleList_10_6)/sizeof(defaultTransparentSubtitleList_10_6[0]));
		else
			forced = 0;
	}
	
	return forced;
}

// this could be a defaults setting, but no real call for it yet
int ShouldImportFontFileName(const char *filename)
{
	// match DynaFont Labs (1997) fonts, which are in many files
	// and completely break ATSUI on different OS versions
	// FIXME: This font works when in ~/Library/Fonts (!). Check it again with CoreText.
	return !(GetSystemMinorVersion() >= 6 && fnmatch("DF*.ttc", filename, 0) == 0);
}

// does the system support HE-AAC with a base frequency over 32khz?
// 10.6.3 does, nothing else does. this may be conflated with some encoder bugs.
int ShouldPlayHighFreqSBR()
{
	return 0;
}

CFPropertyListRef CopyPreferencesValueTyped(CFStringRef key, CFTypeID type)
{
	CFPropertyListRef val = CFPreferencesCopyAppValue(key, PERIAN_PREF_DOMAIN);
	
	if (val && CFGetTypeID(val) != type) {
		CFRelease(val);
		val = NULL;
	}
	
	return val;
}

void *fast_realloc_with_padding(void *ptr, unsigned int *size, unsigned int min_size)
{
	void *res = ptr;
	av_fast_malloc(&res, size, min_size + FF_INPUT_BUFFER_PADDING_SIZE);
	if (res) memset(res + min_size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
	return res;
}

// Map 8-bit alpha (graphicsModePreBlackAlpha) to 1-bit alpha (transparent)
// Pretty much this is just mapping all opaque black to (1,1,1,255)
// Leaves ugly borders where AAing turned into opaque colors, but that's harder to deal with
void ConvertImageToQDTransparent(Ptr baseAddr, OSType pixelFormat, int rowBytes, int width, int height)
{
	UInt32 alphaMask = EndianU32_BtoN((pixelFormat == k32ARGBPixelFormat) ? 0xFF000000 : 0xFF),
		 replacement = EndianU32_BtoN((pixelFormat == k32ARGBPixelFormat) ? 0xFF010101 : 0x010101FF);
	Ptr p = baseAddr;
	int y, x;
	
	for (y = 0; y < height; y++) {
		UInt32 *p32 = (UInt32*)p;
		for (x = 0; x < width; x++) {
			UInt32 px = *p32;
			
			// if px is black, and opaque (alpha == 255)
			if (!(px & ~alphaMask) && ((px & alphaMask) == alphaMask)) {
				// then set it to not-quite-black so it'll show up
				*p32 = replacement;
			}
			
			p32++;
		}
		
		p += rowBytes;
	}
}