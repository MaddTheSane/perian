/*****************************************************************************
*
*  Avi Import Component Resources
*
*  Copyright(C) 2006 Christoph Naegeli <chn1@mac.com>
*
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*  
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*  
*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*
****************************************************************************/

#define kPerianManufacturer 'Peri'

#define thng_RezTemplateVersion 2

#include "PerianResourceIDs.h"
#include <Carbon/Carbon.r>
#include <QuickTime/QuickTime.r>
#undef __CARBON_R__
#undef __CORESERVICES_R__
#undef __CARBONCORE_R__
#undef __COMPONENTS_R__

#define kAVIthngResID 512
#define kFLVthngResID 515
#define kTTAthngResID 517
#define kNUVthngResID 518

#define kAVIName "Perian AVI Movie Importer"
#define kFLVName "Perian Flash Video Importer"
#define kTTAName "Perian True Audio Importer"
#define kNUVName "Perian NuppelVideo Importer"

#define kFFAvi_MovieImportFlags \
( canMovieImportFiles | canMovieImportInPlace | canMovieImportDataReferences | canMovieImportValidateFile \
		| canMovieImportValidateDataReferences | canMovieImportWithIdle | hasMovieImportMIMEList \
		| movieImportMustGetDestinationMediaType | canMovieImportAvoidBlocking | cmpThreadSafe | canMovieImportPartial )

/* Component Manager Things - 
	AVI */
resource 'thng' (kAVIthngResID, kAVIName) {
	'eat ',					// Type
	'VfW ',					// SubType
	'vide',                 // Manufacturer
	0,
	0,
	0,
	0,
	'STR ',						// Name Type
	kAVIthngResID,				// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
	kFFAviComponentVersion,		// Version
	componentHasMultiplePlatforms +
	componentDoAutoVersion,		// Registratin Flags
	0,							// Resource ID of Icon Family
{
	kFFAvi_MovieImportFlags,
	'dlle',					// Code Resource type
	kAVIthngResID,
	platformIA32NativeEntryPoint,		// IA32
	kFFAvi_MovieImportFlags,
	'dlle',
	kAVIthngResID,
	platformPowerPCNativeEntryPoint,	// PowerPC
},
    'thnr', kAVIthngResID
};

resource 'thga' (kAVIthngResID + 1, kAVIName) {
	'eat ',					// Type
	'VFW ',					// SubType
	'vide',                 // Manufacturer
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	0,
	0,
	0,
	'STR ',						// Name Type
	kAVIthngResID,				// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
								// TARGET COMPONENT ---------------
	'eat ',						// Type
	'VfW ',						// SubType
	'vide',                     // Manufacturer
	0,							// Component Flags
	0,							// Component Flags Mask
	'thnr', kAVIthngResID,		// Component public resource identifier
	cmpAliasOnlyThisFile
};

resource 'thga' (kAVIthngResID + 2, kAVIName) {
	'eat ',					// Type
	'AVI ',					// SubType
	'vide',                 // Manufacturer
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	0,
	0,
	0,
	'STR ',						// Name Type
	kAVIthngResID,				// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
								// TARGET COMPONENT ---------------
	'eat ',						// Type
	'VfW ',						// SubType
	'vide',                     // Manufacturer
	0,							// Component Flags
	0,							// Component Flags Mask
	'thnr', kAVIthngResID,		// Component public resource identifier
	cmpAliasOnlyThisFile
};

resource 'thga' (kAVIthngResID + 3, kAVIName) {
	'eat ',					// Type
	'DIVX',					// SubType
	'vide',                 // Manufacturer
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	0,
	0,
	0,
	'STR ',						// Name Type
	kAVIthngResID,				// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
								// TARGET COMPONENT ---------------
	'eat ',						// Type
	'VfW ',						// SubType
	'vide',                     // Manufacturer
	0,							// Component Flags
	0,							// Component Flags Mask
	'thnr', kAVIthngResID,		// Component public resource identifier
	cmpAliasOnlyThisFile
};

resource 'thga' (kAVIthngResID + 4, kAVIName) {
	'eat ',					// Type
	'GVI ',					// SubType
	'vide',                 // Manufacturer
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	0,
	0,
	0,
	'STR ',						// Name Type
	kAVIthngResID,				// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
								// TARGET COMPONENT ---------------
	'eat ',						// Type
	'VfW ',						// SubType
	'vide',                     // Manufacturer
	0,							// Component Flags
	0,							// Component Flags Mask
	'thnr', kAVIthngResID,		// Component public resource identifier
	cmpAliasOnlyThisFile
};

resource 'thga' (kAVIthngResID + 5, kAVIName) {
	'eat ',					// Type
	'VP6 ',					// SubType
	'vide',                 // Manufacturer
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	0,
	0,
	0,
	'STR ',						// Name Type
	kAVIthngResID,				// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
								// TARGET COMPONENT ---------------
	'eat ',						// Type
	'VfW ',						// SubType
	'vide',                     // Manufacturer
	0,							// Component Flags
	0,							// Component Flags Mask
	'thnr', kAVIthngResID,		// Component public resource identifier
	cmpAliasOnlyThisFile
};

resource 'thnr' (kAVIthngResID, kAVIName) {
{
	'mime', 1, 0, 'mime', kAVIthngResID, 0,
    'mcfg', 1, 0, 'mcfg', kAVIthngResID, 0,
}
};

resource 'mime' (kAVIthngResID, kAVIName) {
{
	kMimeInfoMimeTypeTag,		1, "video/x-msvideo";
	kMimeInfoMimeTypeTag,		2, "video/msvideo";
	kMimeInfoMimeTypeTag,		3, "video/avi";
    kMimeInfoMimeTypeTag,		4, "video/avi";
	kMimeInfoMimeTypeTag,		5, "video/avi";
	kMimeInfoMimeTypeTag,		6, "video/avi";
	kMimeInfoFileExtensionTag,	1, "avi";
	kMimeInfoFileExtensionTag,	2, "avi";
	kMimeInfoFileExtensionTag,	3, "avi";
    kMimeInfoFileExtensionTag,	4, "gvi";
	kMimeInfoFileExtensionTag,	5, "divx";
	kMimeInfoFileExtensionTag,	6, "vp6";
	kMimeInfoDescriptionTag,	1, "AVI Movie File";
	kMimeInfoDescriptionTag,	2, "AVI Movie File";
	kMimeInfoDescriptionTag,	3, "AVI Movie File";
    kMimeInfoDescriptionTag,	4, "AVI Movie File";
	kMimeInfoDescriptionTag,	5, "AVI Movie File";
	kMimeInfoDescriptionTag,	6, "AVI Movie File";
};
};

resource 'mcfg' (kAVIthngResID, kAVIName) {
    kVersionDoesntMatter,
    {
        kQTMediaConfigVideoGroupID,
        kQTMediaConfigUsePluginByDefault | kQTMediaConfigCanUseApp |
        kQTMediaConfigCanUsePlugin | kQTMediaConfigBinaryFile |
        kQTMediaConfigTakeFileAssociationByDefault,
        'VfW ',
        'TVOD',
        'eat ',
        'VfW ',
        'vide',
        0, 0,
        
        'AVI ',
        kQTMediaInfoWinGroup,
        
        {
        },
        
        {
            "AVI file",
            "vfw,avi,gvi,divx,vp6",
            "QuickTime Player",
            "Perian AVI Movie Importer",
            "",
        },
        
        {
            "video/x-msvideo",
            "video/msvideo",
            "video/avi",
        },
    }
};

resource 'STR ' (kAVIthngResID, kAVIName) {
	kAVIName
};


/* Component Manager Things - 
	FLV */
resource 'thng' (kFLVthngResID, kFLVName) {
	'eat ',					// Type
	'FLV ',					// SubType
	'vide',					// Manufacturer
	0,
	0,
	0,
	0,
	'STR ',						// Name Type
	kFLVthngResID,				// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
	kFFAviComponentVersion,		// Version
	componentHasMultiplePlatforms +
	componentDoAutoVersion,		// Registratin Flags
	0,							// Resource ID of Icon Family
{
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	'dlle',                     // Code Resource type
	kAVIthngResID,
	platformIA32NativeEntryPoint,		// IA32
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	'dlle',
	kAVIthngResID,
	platformPowerPCNativeEntryPoint,	// PowerPC
},
'thnr', kFLVthngResID
};

resource 'thnr' (kFLVthngResID, kFLVName) {
{
	'mime', 1, 0, 'mime', kFLVthngResID, 0,
    'mcfg', 1, 0, 'mcfg', kFLVthngResID, 0,
}
};

resource 'mime' (kFLVthngResID, kFLVName) {
{
	kMimeInfoMimeTypeTag,		1, "video/x-flv";
	kMimeInfoFileExtensionTag,	1, "flv";
	kMimeInfoDescriptionTag,	1, "Flash Video";
};
};

resource 'mcfg' (kFLVthngResID, kFLVName) {
    kVersionDoesntMatter,
    {
        kQTMediaConfigVideoGroupID,
        kQTMediaConfigUsePluginByDefault | kQTMediaConfigCanUseApp |
        kQTMediaConfigCanUsePlugin | kQTMediaConfigBinaryFile |
        kQTMediaConfigTakeFileAssociationByDefault,
        'FLV ',
        'TVOD',
        'eat ',
        'FLV ',
        'vide',
        0, 0,
        
        'FLV ',
        kQTMediaInfoNetGroup,
        
        {
        },
        
        {
            "Flash Video",
            "flv",
            "QuickTime Player",
            kFLVName,
            "",
        },
        
        {
            "video/x-flv",
        },
    }
};

resource 'STR ' (kFLVthngResID, kFLVName) {
	kFLVName
};

/* Component Manager Things - 
	TTA */
resource 'thng' (kTTAthngResID, kTTAName) {
	'eat ',					// Type
	'TTA ',					// SubType
	'soun',					// Manufacturer
	0,
	0,
	0,
	0,
	'STR ',						// Name Type
	kTTAthngResID,				// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
	kFFAviComponentVersion,		// Version
	componentHasMultiplePlatforms +
	componentDoAutoVersion,		// Registratin Flags
	0,							// Resource ID of Icon Family
{
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	'dlle',                     // Code Resource type
	kAVIthngResID,
	platformIA32NativeEntryPoint,		// IA32
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	'dlle',
	kAVIthngResID,
	platformPowerPCNativeEntryPoint,	// PowerPC
},
'thnr', kTTAthngResID
};

resource 'thnr' (kTTAthngResID, kTTAName) {
{
	'mime', 1, 0, 'mime', kTTAthngResID, 0,
    'mcfg', 1, 0, 'mcfg', kTTAthngResID, 0,
}
};

resource 'mime' (kTTAthngResID, kTTAName) {
{
	kMimeInfoMimeTypeTag,		1, "audio/x-tta";
	kMimeInfoFileExtensionTag,	1, "tta";
	kMimeInfoDescriptionTag,	1, "True Audio";
};
};

resource 'mcfg' (kTTAthngResID, kTTAName) {
    kVersionDoesntMatter,
    {
        kQTMediaConfigAudioGroupID,
        kQTMediaConfigUsePluginByDefault | kQTMediaConfigCanUseApp |
        kQTMediaConfigCanUsePlugin | kQTMediaConfigBinaryFile |
        kQTMediaConfigTakeFileAssociationByDefault,
        'TTA ',
        'TVOD',
        'eat ',
        'TTA ',
        'soun',
        0, 0,
        
        'TTA ',
        kQTMediaInfoNetGroup,
        
        {
        },
        
        {
            "True Audio",
            "tta",
            "QuickTime Player",
            "Perian True Audio Importer",
            "",
        },
        
        {
            "audio/x-tta",
        },
    }
};

resource 'STR ' (kTTAthngResID, kTTAName) {
	kTTAName
};

/* Component Manager Things - 
	Nuv */
resource 'thng' (kNUVthngResID, kNUVName) {
	'eat ',					// Type
	'NUV ',					// SubType
	'vide',					// Manufacturer
	0,
	0,
	0,
	0,
	'STR ',						// Name Type
	kNUVthngResID,				// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
	kFFAviComponentVersion,		// Version
	componentHasMultiplePlatforms +
	componentDoAutoVersion,		// Registratin Flags
	0,							// Resource ID of Icon Family
{
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	'dlle',                     // Code Resource type
	kAVIthngResID,
	platformIA32NativeEntryPoint,		// IA32
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	'dlle',
	kAVIthngResID,
	platformPowerPCNativeEntryPoint,	// PowerPC
},
'thnr', kNUVthngResID
};

resource 'thnr' (kNUVthngResID, kNUVName) {
    {
        'mime', 1, 0, 'mime', kNUVthngResID, 0,
        'mcfg', 1, 0, 'mcfg', kNUVthngResID, 0,
    }
};

resource 'mime' (kNUVthngResID, kNUVName) {
{
	kMimeInfoMimeTypeTag,		1, "video/x-nuv";
	kMimeInfoFileExtensionTag,	1, "nuv";
	kMimeInfoDescriptionTag,	1, "NuppelVideo";
};
};

resource 'mcfg' (kNUVthngResID, kNUVName) {
    kVersionDoesntMatter,
    {
        kQTMediaConfigVideoGroupID,
        kQTMediaConfigUsePluginByDefault | kQTMediaConfigCanUseApp |
        kQTMediaConfigCanUsePlugin | kQTMediaConfigBinaryFile |
        kQTMediaConfigTakeFileAssociationByDefault,
        'NUV ',
        'TVOD',
        'eat ',
        'NUV ',
        'vide',
        0, 0,
        
        'NUV ',
        kQTMediaInfoNetGroup,
        
        {
        },
        
        {
            "NuppelVideo",
            "nuv",
            "QuickTime Player",
            kNUVName,
            "",
        },
        
        {
            "video/x-nuv",
        },
    }
};

resource 'STR ' (kNUVthngResID, kNUVName) {
	kNUVName
};

resource 'dlle' (kAVIthngResID, kAVIName) {
	"FFAvi_MovieImportComponentDispatch"
};
