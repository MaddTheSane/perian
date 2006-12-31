/*
 *  XCAResources.r
 *
 *    'thng' (and other) resource template for XiphQT CoreAudio components.
 *
 *
 *  Copyright (c) 2005-2006  Arek Korbik
 *
 *  This file is part of XiphQT, the Xiph QuickTime Components.
 *
 *  XiphQT is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  XiphQT is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with XiphQT; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  Last modified: $Id: XCAResources.r 10748 2006-01-23 18:00:56Z arek $
 *
 */



#define UseExtendedThingResource 1


#ifndef __HAVE_INCLUDES_ALREADY__

#include <CoreServices/CoreServices.r>

#ifndef thng_RezTemplateVersion
  #define thng_RezTemplateVersion	2
#endif

#define __HAVE_INCLUDES_ALREADY__
#endif /* __HAVE_INCLUDES_ALREADY__ */


/* How do I do this properly... anybody? */
#if defined(BUILD_UNIVERSAL)
  #define TARGET_CPU_PPC 1
  #define TARGET_CPU_X86 1
#endif


#ifndef GEN_MISSING
  #define GEN_MISSING 0
#endif

#if TARGET_OS_MAC
  #if TARGET_CPU_PPC && TARGET_CPU_X86
    #define TARGET_REZ_FAT_COMPONENTS 1
    #define Target_PlatformType       platformPowerPCNativeEntryPoint
    #define Target_SecondPlatformType platformIA32NativeEntryPoint
  #elif TARGET_CPU_X86
    #define Target_PlatformType       platformIA32NativeEntryPoint
  #else
    #define Target_PlatformType       platformPowerPCNativeEntryPoint
  #endif

//  #define Target_CodeResType		'dlle'
//  #define TARGET_REZ_USE_DLLE		1
#elif TARGET_OS_WIN32
  #define Target_PlatformType platformWin32
#else
  #error get a real platform type
#endif /* TARGET_OS_MAC */

#if !defined(TARGET_REZ_FAT_COMPONENTS)
  #define TARGET_REZ_FAT_COMPONENTS 0
#endif

#if kComponentIsThreadSafe
  #ifndef cmpThreadSafeOnMac
    #define cmpThreadSafeOnMac 0x10000000
  #endif
  #define COMPONENT_FLAGS cmpThreadSafeOnMac
#else
  #define COMPONENT_FLAGS 0
#endif


resource 'strn' (kPrimaryResourceID, purgeable)
{
    kComponentName
};

resource 'stri' (kPrimaryResourceID, purgeable)
{
    kComponentInfo
};


#if !GEN_MISSING

resource 'dlle' (kPrimaryResourceID)
{
    kComponentEntryPoint
};

#define kComponentRegistrationFlags	componentHasMultiplePlatforms | componentDoAutoVersion | componentLoadResident


resource 'thng' (kPrimaryResourceID, kComponentName)
{
    kComponentType,
    kComponentSubtype,
    kComponentManufacturer,
    kComponentFlags,
    0, //	Component flags mask
    0, 0, //	Code type, Code ID
    'strn', kPrimaryResourceID, //	Name resource type, resource ID
    'stri', kPrimaryResourceID, //	Info resource type, resource ID
    0, 0, //	Icon resource type, resource ID
    kComponentVersion,
    kComponentRegistrationFlags,
    0, //	Icon family resource ID
    { //	Beginning of platform info
        COMPONENT_FLAGS, //	Component flags
        'dlle', kPrimaryResourceID,
        Target_PlatformType,
#if TARGET_REZ_FAT_COMPONENTS
        COMPONENT_FLAGS,
        'dlle', kPrimaryResourceID,
        Target_SecondPlatformType,
#endif
    },
#if thng_RezTemplateVersion >= 2
    kComponentPublicResourceMapType, kPrimaryResourceID
#endif
};
#else /* GEN_MISSING */

resource 'thga' (kPrimaryResourceID) {
    kComponentType,
    kComponentSubtype,
    kComponentManufacturer,
    kComponentFlags,
    0, //	Component flags mask
    0, 0, //	Code type, Code ID
    'strn', kPrimaryResourceID, //	Name resource type, resource ID
    'stri', kPrimaryResourceID, //	Info resource type, resource ID
    0, 0, //	Icon resource type, resource ID
    'miss', //	Alias component type
    'base', //	Alias component subtype
    0, //	Alias component manufacturer
    0, //	Alias component flags
    0, //	Alias component flags mask
#if thng_RezTemplateVersion >= 2
    kComponentPublicResourceMapType, kPrimaryResourceID,
    cmpAliasNoFlags //	Alias flags
#endif
};
#endif /* GEN_MISSING */


#undef kPrimaryResourceID
#undef kComponentType
#undef kComponentSubtype
//#undef kComponentManufacturer
//#undef kComponentFlags
//#undef kComponentVersion
#undef kComponentRegistrationFlags
#undef kComponentName
#undef kComponentInfo
//#undef kComponentEntryPoint
//#undef kComponentPublicResourceMapType
//#undef kComponentIsThreadSafe

#undef Target_PlatformType
#undef Target_CodeResType
//#undef kUseDLLEResource

#undef TARGET_REZ_FAT_COMPONENTS
#undef Target_SecondPlatformType
#undef GEN_MISSING
