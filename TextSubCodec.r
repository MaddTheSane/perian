
#include "TextSubCodec.h"

/*
 thng_RezTemplateVersion:
 0 - original 'thng' template    <-- default
 1 - extended 'thng' template	<-- used for multiplatform things
 2 - extended 'thng' template including resource map id
 */
#define thng_RezTemplateVersion 1

/*
 cfrg_RezTemplateVersion:
 0 - original					<-- default
 1 - extended					<-- we use the extended version
 */
#define cfrg_RezTemplateVersion 1

#define TARGET_REZ_CARBON_MACHO 1

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
    #include "MacTypes.r"
    #include "Components.r"
    #include "ImageCodec.r"
    #include "CodeFragments.r"
	#undef __COMPONENTS_R__
#endif


// These flags specify information about the capabilities of the component
// Works with 1-bit, 8-bit, 16-bit and 32-bit Pixel Maps
#define kTextSubDecoFlags ( codecInfoDoes32 | codecInfoDoes16 | codecInfoDoes8 | codecInfoDoes1 | codecInfoDoesSpool )

// These flags specify the possible format of compressed data produced by the component
// and the format of compressed files that the component can handle during decompression
// The component can decompress from files at 1-bit, 8-bit, 16-bit, 24-bit and 32-bit depths
#define kTextSubFormatFlags	( codecInfoDepth32 | codecInfoDepth24 | codecInfoDepth16 | codecInfoDepth8 | codecInfoDepth1 )

// Component Description
resource 'cdci' (kTextSubCodecResource) {
	kTextSubCodecFormatName,	// Type
	1,						// Version
	1,						// Revision level
	kManufacturer,			// Manufacturer
	kTextSubDecoFlags,		// Decompression Flags
	0,						// Compression Flags
	kTextSubFormatFlags,	// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,						// Reserved
	1,						// Minimum Height
	1,						// Minimum Width
	0,						// Decompression Pipeline Latency
	0,						// Compression Pipeline Latency
	0						// Private Data
};

resource 'thng' (kTextSubCodecResource) {
	decompressorComponentType,				// Type			
	kSubFormatUTF8,							// SubType
	kManufacturer,							// Manufacturer
											// - use componentHasMultiplePlatforms
	0,										// not used Component flags
	0,										// not used Component flags Mask
	0,										// not used Code Type
	0,										// not used Code ID
	'STR ',									// Name Type
	kTextSubCodecResource,					// Name ID
	'STR ',									// Info Type
	kTextSubCodecResource + 1,				// Info ID
	0,										// Icon Type
	0,										// Icon ID
	kTextSubCodecVersion,					// Version
	componentHasMultiplePlatforms +			// Registration Flags 
	componentDoAutoVersion,
	0,										// Resource ID of Icon Family
	{
#if TARGET_OS_MAC
	#if TARGET_REZ_CARBON_MACHO
        #if !(TARGET_REZ_MAC_PPC || TARGET_REZ_MAC_X86)
        	#error "Platform architecture not defined, TARGET_REZ_MAC_PPC and/or TARGET_REZ_MAC_X86 must be defined!"
        #endif
		#if TARGET_REZ_MAC_PPC
            kTextSubDecoFlags, 
            'dlle',								// Code Resource type - Entry point found by symbol name 'dlle' resource
            kTextSubCodecResource,				// ID of 'dlle' resource
            platformPowerPCNativeEntryPoint,	// PPC
        #endif
        #if TARGET_REZ_MAC_X86
			kTextSubDecoFlags, 
			'dlle',
			kTextSubCodecResource,
			platformIA32NativeEntryPoint,		// INTEL
        #endif
	#else
		#error "At least one non-obsolete TARGET_REZ_XXX_XXX platform must be defined."
	#endif
#endif
#if TARGET_OS_WIN32
	kTextSubDecoFlags, 
	'dlle',
	kTextSubCodecResource,
	platformWin32,
#endif
	};
};

// Component Name
resource 'STR ' (kTextSubCodecResource) {
	"Text Subtitle"
};

// Component Information
resource 'STR ' (kTextSubCodecResource + 1) {
	"Renders subtitles stored as text."
};

#if	TARGET_REZ_CARBON_MACHO || TARGET_REZ_WIN32
// Code Entry Point for Mach-O and Windows
	resource 'dlle' (kTextSubCodecResource) {
		"TextSubCodecComponentDispatch"
	};
#endif
