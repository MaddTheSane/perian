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

#ifndef __COMMONUTILS_H__
#define __COMMONUTILS_H__

#include <QuickTime/QuickTime.h>

#ifdef __cplusplus
extern "C"
{
#endif

// ISO 639-1 to language ID expected by SetMediaLanguage
short ISO639_1ToQTLangCode(const char *lang);

// ISO 639-2 to language ID expected by SetMediaLanguage
short ISO639_2ToQTLangCode(const char *lang);

/* write the int32_t data to target & then return a pointer which points after that data */
uint8_t *write_int32(uint8_t *target, int32_t data);

/* write the int16_t data to target & then return a pointer which points after that data */
uint8_t *write_int16(uint8_t *target, int16_t data);

/* write the data to the target adress & then return a pointer which points after the written data */
uint8_t *write_data(uint8_t *target, uint8_t* data, int32_t data_size);

// mallocs the buffer and copies the codec-specific description to it, in the same format
// as is specified in Matroska and is used in libavcodec
ComponentResult ReadESDSDescExt(Handle descExt, UInt8 **buffer, int *size);

int isImageDescriptionExtensionPresent(ImageDescriptionHandle desc, long type);

// does the current process break if we signal droppable frames?
int IsFrameDroppingEnabled();

// does the current process break if we return errors in Preflight?
int IsForcedDecodeEnabled();

// does the current process break if we use graphicsModePreBlackAlpha?
int IsTransparentSubtitleHackEnabled();

int IsAltivecSupported();

// is this font name known to be incompatible with ATSUI?
int ShouldImportFontFileName(const char *filename);
	
// can we safely create an HE-AAC track with a frequency of more than 48khz?
int ShouldPlayHighFreqSBR();
	
// CFPreferencesCopyAppValue() wrapper which checks the type of the value returned
CFPropertyListRef CopyPreferencesValueTyped(CFStringRef key, CFTypeID type);

// critical region for initializing stuff
// component/ffmpeg initialization should be the only thing that really needs a mutex
int PerianInitEnter(volatile Boolean *inited);
void PerianInitExit(int unlock);

void *fast_realloc_with_padding(void *ptr, unsigned int *size, unsigned int min_size);

// return an sRGB colorspace. safe to use on 10.4.
CGColorSpaceRef GetSRGBColorSpace();

// postprocess RGBA+8-bit to not look terrible when displayed with 'transparent' blend mode
void ConvertImageToQDTransparent(Ptr baseAddr, OSType pixelFormat, int rowBytes, int width, int height);
	
#define PERIAN_PREF_DOMAIN CFSTR("org.perian.Perian")

#ifdef __cplusplus
}
#endif

#endif
