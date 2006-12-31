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

#define kChristophManufacturer 'Rafz'

#define thng_RezTemplateVersion 2

#include "ff_MovieImportVersion.h"
#include <Carbon/Carbon.r>
#include <QuickTime/QuickTime.r>
#undef __CARBON_R__
#undef __CORESERVICES_R__
#undef __CARBONCORE_R__
#undef __COMPONENTS_R__

#define kFFAvi_MovieImportFlags \
( canMovieImportFiles | canMovieImportInPlace | canMovieImportDataReferences | canMovieImportValidateFile \
		| canMovieImportValidateDataReferences | canMovieImportWithIdle | hasMovieImportMIMEList \
		| movieImportMustGetDestinationMediaType | cmpThreadSafe )

/* Component Manager Things - 
	AVI */
resource 'thng' (kAVIthngResID) {
	'eat ',					// Type
	'VfW ',					// SubType
	kChristophManufacturer,	// Manufacturer
	0,
	0,
	0,
	0,
	'STR ',						// Name Type
	kAVIthngResID,						// Name ID
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
	512,
	platformIA32NativeEntryPoint,		// IA32
	kFFAvi_MovieImportFlags,
	'dlle',
	512,
	platformPowerPCNativeEntryPoint,	// PowerPC
},
'thnr', kAVIthngResID
};

resource 'thga' (513) {
	'eat ',					// Type
	'VFW ',					// SubType
	kChristophManufacturer,	// Manufacturer
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	0,
	0,
	0,
	'STR ',						// Name Type
	kAVIthngResID,						// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
								// TARGET COMPONENT ---------------
	'eat ',						// Type
	'VfW ',						// SubType
	kChristophManufacturer,		// Manufaturer
	0,							// Component Flags
	0,							// Component Flags Mask
	'thnr', kAVIthngResID,				// Component public resource identifier
	cmpAliasOnlyThisFile
};

resource 'thga' (514) {
	'eat ',					// Type
	'AVI ',					// SubType
	kChristophManufacturer,	// Manufacturer
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	0,
	0,
	0,
	'STR ',						// Name Type
	kAVIthngResID,						// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
								// TARGET COMPONENT ---------------
	'eat ',						// Type
	'VfW ',						// SubType
	kChristophManufacturer,		// Manufaturer
	0,							// Component Flags
	0,							// Component Flags Mask
	'thnr', kAVIthngResID,				// Component public resource identifier
	cmpAliasOnlyThisFile
};

resource 'thga' (515) {
	'eat ',					// Type
	'DIVX',					// SubType
	kChristophManufacturer,	// Manufacturer
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	0,
	0,
	0,
	'STR ',						// Name Type
	kAVIthngResID,						// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
								// TARGET COMPONENT ---------------
	'eat ',						// Type
	'VfW ',						// SubType
	kChristophManufacturer,		// Manufaturer
	0,							// Component Flags
	0,							// Component Flags Mask
	'thnr', kAVIthngResID,				// Component public resource identifier
	cmpAliasOnlyThisFile
};

resource 'thga' (516) {
	'eat ',					// Type
	'GVI ',					// SubType
	kChristophManufacturer,	// Manufacturer
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	0,
	0,
	0,
	'STR ',						// Name Type
	kAVIthngResID,						// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
								// TARGET COMPONENT ---------------
	'eat ',						// Type
	'VfW ',						// SubType
	kChristophManufacturer,		// Manufaturer
	0,							// Component Flags
	0,							// Component Flags Mask
	'thnr', kAVIthngResID,				// Component public resource identifier
	cmpAliasOnlyThisFile
};

/* Perhaps in a later version, we have to use "QuickTime Media Configuration Resources" */
resource 'thnr' (kAVIthngResID) {
{
	'mime', 1, 0,
	'mime', kAVIthngResID, 0,
}
};

resource 'mime' (kAVIthngResID) {
{
	kMimeInfoMimeTypeTag,		1,	"video/x-msvideo";
	kMimeInfoMimeTypeTag,		2,	"video/msvideo";
	kMimeInfoMimeTypeTag,		3,	"video/avi";
	kMimeInfoFileExtensionTag,	1, "avi";
	kMimeInfoFileExtensionTag,	2, "avi";
	kMimeInfoFileExtensionTag,	3, "avi";
	kMimeInfoDescriptionTag,	1, "XviD Avi Import";
	kMimeInfoDescriptionTag,	2, "XviD Avi Import";
	kMimeInfoDescriptionTag,	3, "XviD Avi Import";
};
};

resource 'STR ' (kAVIthngResID) {
	"FFAvi Movie Importer"
};


/* Component Manager Things - 
	FLV */
resource 'thng' (kFLVthngResID) {
	'eat ',					// Type
	'FLV ',					// SubType
	'vide',					// Manufacturer
	0,
	0,
	0,
	0,
	'STR ',						// Name Type
	kFLVthngResID,						// Name ID
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
	'dlle',					// Code Resource type
	512,
	platformIA32NativeEntryPoint,		// IA32
	kFFAvi_MovieImportFlags | movieImportSubTypeIsFileExtension,
	'dlle',
	512,
	platformPowerPCNativeEntryPoint,	// PowerPC
},
'thnr', kFLVthngResID
};

resource 'thnr' (kFLVthngResID) {
{
	'mime', 1, 0,
	'mime', kFLVthngResID, 0,
}
};

resource 'mime' (kFLVthngResID) {
{
	kMimeInfoMimeTypeTag,		1, "video/x-flv";
	kMimeInfoFileExtensionTag,	1, "flv";
	kMimeInfoDescriptionTag,	1, "Flash Video";
};
};

resource 'STR ' (kFLVthngResID) {
	"Flash Video Importer"
};

resource 'dlle' (512) {
	"FFAvi_MovieImportComponentDispatch"
};
