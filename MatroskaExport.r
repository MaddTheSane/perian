/*
 *  MatroskaExport.r
 *
 *    MatroskaExport.r - Component resources for the export component
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

#if TARGET_REZ_CARBON_MACHO
    #if defined(ppc_YES)
        // PPC architecture
        #define TARGET_REZ_MAC_PPC 1
    #else
        #define TARGET_REZ_MAC_PPC 0
    #endif

    #if defined(i386_YES)
        // x86 architecture
        #define TARGET_REZ_MAC_X86 1
    #else
        #define TARGET_REZ_MAC_X86 0
    #endif

    #define TARGET_REZ_WIN32 0
#else
    // Must be building on Windows
    #define TARGET_REZ_WIN32 1
#endif

#if TARGET_REZ_CARBON_MACHO
    #include <Carbon/Carbon.r>
    #include <QuickTime/QuickTime.r>
	#undef __CARBON_R__
	#undef __CORESERVICES_R__
	#undef __CARBONCORE_R__
	#undef __COMPONENTS_R__
#else
    #include "ConditionalMacros.r"
    #include "Components.r"
    #include "QuickTimeComponents.r"
    #include "Types.r"
    #include "MacTypes.r"
    #include "Components.r"
    #include "CodeFragments.r"
	#undef __COMPONENTS_R__
#endif

#include "MatroskaExportVersion.h"

#define kMatroskaExportThingRes					1024	
#define	kMatroskaExportCodeRes					1024
#define	kMatroskaExportNameRes					1024
#define	kMatroskaExportInfoRes					1025

#define kMatroskaExportFlags \
	(canMovieExportFiles | canMovieExportFromProcedures | movieExportMustGetSourceMediaType | \
	 canMovieExportValidateMovie | cmpThreadSafe)
//   hasMovieExportUserInterface 

// Component Manager Thing
resource 'thng' (kMatroskaExportThingRes) {
	'spit',									// Type
	'MkvF',									// SubType
	'Yuvi',									// Manufacturer
											// - use componentHasMultiplePlatforms
	0,										// not used Component flags
	0,										// not used Component flags Mask
	0,										// not used Code Type
	0,										// not used Code ID
	'STR ',									// Name Type
	kMatroskaExportNameRes,                 // Name ID
	'STR ',									// Info Type
	kMatroskaExportInfoRes,                 // Info ID
	0,										// Icon Type
	0,										// Icon ID
	kMatroskaExportVersion,					// Version
    componentHasMultiplePlatforms +			// Registration Flags
	componentDoAutoVersion,
	0,										// Resource ID of Icon Family
	{
#if TARGET_OS_MAC							// COMPONENT PLATFORM INFORMATION ----------------------
	#if TARGET_REZ_CARBON_MACHO
		#if !(TARGET_REZ_MAC_PPC || TARGET_REZ_MAC_X86)
        	#error "Platform architecture not defined, TARGET_REZ_MAC_PPC and/or TARGET_REZ_MAC_X86 must be defined!"
        #endif
		#if TARGET_REZ_MAC_PPC
            kMatroskaExportFlags, 
            'dlle',								// Code Resource type - Entry point found by symbol name 'dlle' resource
            kMatroskaExportCodeRes,				// ID of 'dlle' resource
            platformPowerPCNativeEntryPoint,	// PPC
        #endif
        #if TARGET_REZ_MAC_X86
            kMatroskaExportFlags, 
            'dlle',
            kMatroskaExportCodeRes,
            platformIA32NativeEntryPoint,		// INTEL
        #endif
	#else
		#error "At least one non-obsolete TARGET_REZ_XXX_XXX platform must be defined."
	#endif
#endif
#if TARGET_OS_WIN32
	kMatroskaExportFlags, 
	'dlle',
	kMatroskaExportCodeRes,
	platformWin32,
#endif
	},
	'thnr', kMatroskaExportThingRes			// Component public resource identifier
};

// Component Name used in Movie Export Dialog
resource 'STR ' (kMatroskaExportNameRes) {
	"Matroska"
};

// Component Information
resource 'STR ' (kMatroskaExportInfoRes) {
	"Exports a QuickTime Movie file to an Matroska Video file."
};

resource 'STR#' (kMatroskaExportShortFileTypeNamesResID) {
	{
		"MkvF"
	}
};

// Component public resource
resource 'thnr' (kMatroskaExportThingRes) {
	{
		'src#', 1, 0,
		'src#', kMatroskaExportThingRes, 0,

		'trk#', 2, 0,
		'trk#', kMatroskaExportThingRes, 0,
	}
};

// http://developer.apple.com/techpubs/quicktime/qtdevdocs/REF/refDataExchange.23.htm#pgfId=13688

// Lists a movie exporter component's supported media types and the minimum and maximum number of sources for each.
//
// A'src#' resource may be associated with export components that implement MovieExportFromProceduresToDataRef. The
// resource is used to indicate the types of data sources the export component can support. Moreover, for each type,
// it also indicates the minimum and maximum number of data sources of that type. Clients can use this information to
// determine if the number of data sources they want to export can be handled directly by the exporter. If the data
// source type is supported by fewer sources are allowed, the client application must either export a fewer number of
// sources or combine the data from its candidate sources itself to meet the limitation imposed by the exporter. 
resource 'src#' (kMatroskaExportThingRes) {
	{
		'vide', 0, 127, isSourceType,
		'soun', 0, 127, isSourceType
	}
};

// Whereas 'src#' is meant to describe the number of data sources supported for use with MovieExportFromProceduresToDataRef,
// the 'trk#' resource is meant to indicate the number of tracks of the given types that can be exported. The resource is
// identical to the resource for data sources. The difference is that the flags will have one of two values: 
// 		isSourceType  - A media type such as 'vide' for video tracks, 'soun' for sound tracks, or 'musi' for QuickTime music tracks.
//      isMediaCharacteristic - Indicates that mediaType corresponds to a media characteristic such a 'eyes' for visual tracks or
//								'ears' for tracks with sound.
resource 'trk#' (kMatroskaExportThingRes) {
	{
		'vide', 0, 127, isSourceType,
		'soun', 0, 127, isSourceType
	}
};

#if	TARGET_REZ_CARBON_MACHO || TARGET_REZ_WIN32
// Code Entry Point for Mach-O and Windows
	resource 'dlle' (kMatroskaExportCodeRes) {
		"MatroskaExportComponentDispatch"
	};
#endif
