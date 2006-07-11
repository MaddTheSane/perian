/*****************************************************************************
*
*  Avi Import Component Resources
*
*  Copyright(C) 2006 Christoph Naegeli <chn1@mac.com>
*
*  This program is free software ; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation ; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY ; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program ; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*
****************************************************************************/

#define kChristophManufacturer 'Rafz'

#define thng_RezTemplateVersion 2

#include <Carbon/Carbon.r>
#include <QuickTime/QuickTime.r>
#undef __CARBON_R__
#undef __CORESERVICES_R__
#undef __CARBONCORE_R__
#undef __COMPONENTS_R__

#define kFFAvi_MovieImportFlags \
( canMovieImportFiles | canMovieImportInPlace | canMovieImportDataReferences | canMovieImportValidateFile \
		| canMovieImportValidateDataReferences | hasMovieImportMIMEList | movieImportMustGetDestinationMediaType \
		| cmpWantsRegisterMessage)


/* Component Manager Things:
* The Component Manager alias doesn't seem to work so we have
* More than one thing 
*/
resource 'thng' (512) {
	'eat ',					// Type
	'VfW ',					// SubType
	kChristophManufacturer,	// Manufacturer
	0,
	0,
	0,
	0,
	'STR ',						// Name Type
	512,						// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
	0,							// Version
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
'thnr', 512
};

resource 'thng' (513) {
	'eat ',					// Type
	'VFW ',					// SubType
	kChristophManufacturer,	// Manufacturer
	0,
	0,
	0,
	0,
	'STR ',						// Name Type
	512,						// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
	0,							// Version
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
'thnr', 512
};

resource 'thng' (514) {
	'eat ',					// Type
	'AVI ',					// SubType
	kChristophManufacturer,	// Manufacturer
	0,
	0,
	0,
	0,
	'STR ',						// Name Type
	512,						// Name ID
	0,							// Info Type
	0,							// Info ID
	0,							// Icon Type
	0,							// Icon ID
	0,							// Version
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
'thnr', 512
};


/* Perhaps in a later version, we have to use "QuickTime Media Configuration Resources" */
resource 'thnr' (512) {
{
	'mime', 1, 0,
	'mime', 512, 0,
}
};

resource 'mime' (512) {
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

resource 'STR ' (512) {
	"FFAvi Movie Importer"
};

resource 'dlle' (512) {
	"FFAvi_MovieImportComponentDispatch"
};
