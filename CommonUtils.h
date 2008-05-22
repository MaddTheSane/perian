/*
 *  CommonUtils.h
 *  Perian
 *
 *  Created by David Conrad on 10/13/06.
 *  Copyright 2006 Perian Project. All rights reserved.
 *
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

int IsFrameDroppingEnabled();

#ifdef __cplusplus
}
#endif

#endif
