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

#ifdef __cplusplus
extern "C"
{
#endif

// ISO 639-1 to language ID expected by SetMediaLanguage
short TwoCharLangCodeToQTLangCode(char *lang);

// ISO 639-2 to language ID expected by SetMediaLanguage
short ThreeCharLangCodeToQTLangCode(char *lang);

#ifdef __cplusplus
}
#endif

#endif
