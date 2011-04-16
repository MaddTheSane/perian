/*
 *  MatroskaImport.r
 *
 *    MatroskaImport.r - Component resources for the import component
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

/*
    thng_RezTemplateVersion:
        0 - original 'thng' template    <-- default
        1 - extended 'thng' template	<-- used for multiplatform things
        2 - extended 'thng' template including resource map id
*/
#define thng_RezTemplateVersion 2

/*
    cfrg_RezTemplateVersion:
        0 - original					<-- default
        1 - extended
*/
#define cfrg_RezTemplateVersion 1

#include <Carbon/Carbon.r>
#include <QuickTime/QuickTime.r>

#include "PerianResourceIDs.h"

#define kMkvImportResource	510
#define kMkvImportName "Perian Matroska Importer"

// These flags specify information about the capabilities of the component
// Can import from files
// Can create a movie from a file without having to write to a separate disk file
// Can accept a data reference (such as a handle) as the source for the import
#define kMatroskaImportFlags \
		(canMovieImportFiles | canMovieImportInPlace | canMovieImportDataReferences | \
		 canMovieImportValidateFile | canMovieImportValidateDataReferences | canMovieImportWithIdle | \
		 hasMovieImportMIMEList | canMovieImportAvoidBlocking | cmpThreadSafe | canMovieImportPartial)

// Component Manager Thing
resource 'thng' (kMkvImportResource, kMkvImportName) {
	'eat ',									// Type
	'MkvF',									// SubType
	'vide',									// Manufacturer
	                                        // For 'eat ' the media type supported by the component
											// - use componentHasMultiplePlatforms
	0,										// not used Component flags
	0,										// not used Component flags Mask
	0,										// not used Code Type
	0,										// not used Code ID
	'STR ',									// Name Type
	kMkvImportResource,						// Name ID
	0,										// Info Type
	0,										// Info ID
	0,										// Icon Type
	0,										// Icon ID
	kMatroskaImportVersion,					// Version
	componentHasMultiplePlatforms +			// Registration Flags
	componentDoAutoVersion,
	0,										// Resource ID of Icon Family
	{
        kMatroskaImportFlags, 
        'dlle',								// Code Resource type - Entry point found by symbol name 'dlle' resource
        kMkvImportResource,					// ID of 'dlle' resource
        platformPowerPCNativeEntryPoint,	// PPC
        kMatroskaImportFlags,
        'dlle',
        kMkvImportResource,
        platformIA32NativeEntryPoint,		// INTEL
	},
	'thnr', kMkvImportResource				// Component public resource identifier
};

// Component Alias
resource 'thga' (kMkvImportResource + 1, kMkvImportName) {
	'eat ',								// Type
	'MKV ', 							// Subtype - this must be in uppercase. It will match an ".eim" suffix case-insensitively.
	'vide',								// Manufacturer - for 'eat ' the media type supported by the component
	kMatroskaImportFlags |				// Component Flags
	movieImportSubTypeIsFileExtension,	// The subtype is a file name suffix
	0,									// Component Flags Mask
	0, 									// Code Type
	0,									// Code ID
	'STR ',								// Name Type
	kMkvImportResource,					// Name ID
	0,									// Info Type
	0,									// Info ID 
	0,									// Icon Type 
	0,									// Icon ID
										// TARGET COMPONENT ---------------
	'eat ',								// Type
	'MkvF',								// SubType
	'vide',								// Manufacturer
	0,									// Component Flags
	0,									// Component Flags Mask
	'thnr', kMkvImportResource,			// Component public resource identifier
	cmpAliasOnlyThisFile
};

// Component Alias
resource 'thga' (kMkvImportResource + 2, kMkvImportName) {
	'eat ',								// Type
	'MKA ', 							// Subtype - this must be in uppercase. It will match an ".eim" suffix case-insensitively.
	'soun',								// Manufacturer - for 'eat ' the media type supported by the component
	kMatroskaImportFlags |				// Component Flags
	movieImportSubTypeIsFileExtension,	// The subtype is a file name suffix
	0,									// Component Flags Mask
	0, 									// Code Type
	0,									// Code ID
	'STR ',								// Name Type
	kMkvImportResource,					// Name ID
	0,									// Info Type
	0,									// Info ID 
	0,									// Icon Type 
	0,									// Icon ID
										// TARGET COMPONENT ---------------
	'eat ',								// Type
	'MkvF',								// SubType
	'vide',								// Manufacturer
	0,									// Component Flags
	0,									// Component Flags Mask
	'thnr', kMkvImportResource,			// Component public resource identifier
	cmpAliasOnlyThisFile
};

// Component Alias
resource 'thga' (518, kMkvImportName) {
	'eat ',								// Type
	'WEBM', 							// Subtype - this must be in uppercase. It will match an ".eim" suffix case-insensitively.
	'vide',								// Manufacturer - for 'eat ' the media type supported by the component
	kMatroskaImportFlags |				// Component Flags
	movieImportSubTypeIsFileExtension,	// The subtype is a file name suffix
	0,									// Component Flags Mask
	0, 									// Code Type
	0,									// Code ID
	'STR ',								// Name Type
	kMkvImportResource,					// Name ID
	0,									// Info Type
	0,									// Info ID 
	0,									// Icon Type 
	0,									// Icon ID
    // TARGET COMPONENT ---------------
	'eat ',								// Type
	'MkvF',								// SubType
	'vide',								// Manufacturer
	0,									// Component Flags
	0,									// Component Flags Mask
	'thnr', kMkvImportResource,			// Component public resource identifier
	cmpAliasOnlyThisFile
};

// Import components should include a public component resource holding the same data that
// MovieImportGetMIMETypeList would return. This public resource's type and ID should be 'mime' and 1,
// respectively. By including this public resource, QuickTime and applications don't need to open the
// import component and call MovieImportGetMIMETypeList to determine the MIME types the importer supports.
// In the absence of this resource, QuickTime and applications will use MovieImportGetMIMETypeList  
//
// Component public resource
resource 'thnr' (kMkvImportResource, kMkvImportName) {
	{
		'mime', 1, 0, 
		'mime', kMkvImportResource, 0,
		
		'mcfg', 1, 0,
		'mcfg', kMkvImportResource, 0
	} 
};

// QuickTime Media Configuration Resources ('mcfg' aka kQTMediaConfigResourceType) are used by the QuickTime MIME
// Configuration Control Panel to build and configure its User Interface. The 'mcfg' resource is also used by the
// QuickTime Plug-In to build a list of MIME types it registers for, and to figure out how to open files.
// In a future version of QuickTime for Windows the 'mcfg' resource will be used for the File Type Registration Control Panel.
// Some, but not all of the information contained within the 'mcfg' resources is available in other places in a component.
// However not everything is available, and not all in one place for example, Group ID, Plug-In, Application Flags and so on.
//
// Every Movie Importer ('eat ') and Graphics Importer ('grip') component should really have one.
//
// If either or both of the kQTMediaConfigCanUseApp and kQTMediaConfigCanUsePlugin flags are set, the MIME type will
// automatically show up in the MIME Configuration Control Panel allowing a user to choose how they want QuickTime to handle
// the file, if at all.
// 
// If the kQTMediaConfigUsePluginByDefault flag is set, QuickTime will automatically register the MIME type for the
// QuickTime plug-in with all browsers on both platforms.
//
// Added in QuickTime 6
resource 'mcfg' (kMkvImportResource, kMkvImportName)
{
	kVersionDoesntMatter,					// Version of the component this applies to
	
	{
		// The ID of the group this type belongs with, (OSType, one of kQTMediaConfigStreamGroupID, etc.)
		// This flag determines which group this MIME type will be listed under in the MIME Configuration Control Panel
		kQTMediaConfigVideoGroupID,
		
		// MIME config flags (unsigned long, one or more of kQTMediaConfigCanUseApp, etc.)
		kQTMediaConfigUsePluginByDefault	// By default, associate with application specified below instead of the QuickTime plug-in
			| kQTMediaConfigCanUseApp		// This type can be associated with an application
			| kQTMediaConfigCanUsePlugin	// This type can be associated with the QuickTime plug-in
			| kQTMediaConfigBinaryFile      // The file is binary, not just text
            | kQTMediaConfigTakeFileAssociationByDefault,		

		'MkvF',								// MacOS file type when saved (OSType)
		'TVOD',								// MacOS file creator when saved (OSType)

		// Component information, used by the QuickTime plug-in to find the component to open this type of file
		'eat ',								// Component type (OSType)
		'MkvF',								// Component subtype (OSType)
		'vide',								// Component manufacturer (OSType)
		0,                                  // Component flags
		0, 									// Flags mask

		'MKV ',								// Default file extension (OSType) - this must be in uppercase. It will match an ".eim" suffix case-insensitively.
		kQTMediaInfoNetGroup,				// QT file type group (OSType, one of kQTMediaInfoNetGroup, etc.)

		// Media type synonyms, an array of zero or more Pascal strings - none here
		{
		},

		{
			"Matroska/WebM file",			// Media type description for MIME configuration panel and browser
			"mkv,mka,webm",					// File extension(s), comma delimited if more than one
			"QuickTime Player",				// Opening application name for MIME configuration panel and browser
			"Perian Matroska Importer",	    // Missing software description for the missing software dialog
			"",                             // Vendor info string (copyright, version, etc)
		},
		
		// Array of one or more MIME types that describe this media type (eg. audio/mpeg, audio/x-mpeg, etc.)
		{
			"video/x-matroska",
			"audio/x-matroska",
            "audio/webm",
            "video/webm",
		},
	}
};

// Component Name
resource 'STR ' (kMkvImportResource, kMkvImportName) {
	"Perian Matroska Importer"
};

/* 
	  This is an example of how to build an atom container resource to hold mime types.
	  This component's GetMIMETypeList implementation simply loads this resource and returns it.
	  Please note that atoms of the same type MUST be grouped together within an atom container.
	  (Also note that "video/electric-image" may not have been registered with the IETF.)
*/
resource 'mime' (kMkvImportResource, kMkvImportName) {
	{
		kMimeInfoMimeTypeTag,      1, "video/x-matroska";
		kMimeInfoMimeTypeTag,      2, "audio/x-matroska";
        kMimeInfoMimeTypeTag,      3, "video/webm";
        kMimeInfoMimeTypeTag,      4, "audio/webm";
		kMimeInfoFileExtensionTag, 1, "mkv";
		kMimeInfoFileExtensionTag, 2, "mka";
        kMimeInfoFileExtensionTag, 3, "webm";
        kMimeInfoFileExtensionTag, 4, "webm";
		kMimeInfoDescriptionTag,   1, "Matroska";
		kMimeInfoDescriptionTag,   2, "Matroska audio";
        kMimeInfoDescriptionTag,   3, "WebM";
        kMimeInfoDescriptionTag,   4, "WebM audio";
	};
};

// Code Entry Point for Mach-O and Windows
resource 'dlle' (kMkvImportResource, kMkvImportName) {
		"MatroskaImportComponentDispatch"
};
