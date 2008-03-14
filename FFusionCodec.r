//---------------------------------------------------------------------------
//FFusion
//Alternative DivX Codec for MacOS X
//version 2.2 (build 72)
//by Jerome Cornet
//Copyright 2002-2003 Jerome Cornet
//parts by Dan Christiansen
//Copyright 2003 Dan Christiansen
//from DivOSX by Jamby
//Copyright 2001-2002 Jamby
//uses libavcodec from ffmpeg 0.4.6
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
//---------------------------------------------------------------------------
//Source Code
//---------------------------------------------------------------------------

#define TARGET_REZ_CARBON_MACHO 1
#define thng_RezTemplateVersion 1	// multiplatform 'thng' resource
#define cfrg_RezTemplateVersion 1	// extended version of 'cfrg' resource

#include <Carbon/Carbon.r>
#include <QuickTime/QuickTime.r>

#include "FFusionCodec.h"

//---------------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------------

// Codec names displayed in QuickTime Player

#define kDivX1Name		"MS-MPEG4 v1"
#define kDivX2Name		"MS-MPEG4 v2"
#define	kDivX3Name		"DivX 3.11 alpha"
#define	kDivX4Name		"DivX 4"
#define	kDivX5Name		"DivX 5"
#define k3ivxName		"3ivx"
#define kXVIDName		"Xvid"
#define kMPEG4Name		"MPEG-4 Video"
#define kH264Name		"H.264"
#define kFLV1Name		"Sorenson H.263"
#define kFlashSVName	"Flash Screen Video"
#define kVP6Name		"TrueMotion VP6"
#define kI263Name		"Intel H.263"
#define kVP3Name		"On2 VP3"
#define kHuffYUVName		"HuffYUV"
#define kMPEG1Name		"MPEG-1 Video"
#define kMPEG2Name		"MPEG-2 Video"
#define kFRAPSName		"Fraps"
#define kSnowName		"Snow"
#define kNuvName		"NuppelVideo"
#define kTSCCName		"Techsmith Screen Capture"

// Codec names Resource ID

#define kDivX1NameResID		256
#define kDivX2NameResID		257
#define	kDivX3NameResID		258
#define	kDivX4NameResID		259
#define	kDivX5NameResID		260
#define	k3ivxNameResID		261
#define	kXVIDNameResID		262
#define	kMPEG4NameResID		263
#define kH264NameResID		264
#define kFLV1NameResID		265
#define kFlashSVNameResID	266
#define kVP6NameResID		267
#define kI263NameResID		268
#define kVP3NameResID		269
#define kHuffYUVNameResID	270
#define kMPEG1NameResID		271
#define kMPEG2NameResID		272
#define kFRAPSNameResID		273
#define kSnowNameResID		274
#define kNuvNameResID		275

// Codec infos Resource ID

#define kDivX1InfoResID		285
#define kDivX2InfoResID		286
#define	kDivX3InfoResID		287
#define	kDivX4InfoResID		288
#define	kDivX5InfoResID		289
#define	k3ivxInfoResID		290
#define	kXVIDInfoResID		291
#define	kMPEG4InfoResID		292
#define kH264InfoResID		293
#define kFLV1InfoResID		294
#define kFlashSVInfoResID	295
#define kVP6InfoResID		296
#define kI263InfoResID		297
#define kVP3InfoResID		298
#define kHuffYUVInfoResID		299
#define kMPEG1InfoResID		300
#define kMPEG2InfoResID		301
#define kFRAPSInfoResID		302
#define kSnowInfoResID		303
#define kNuvInfoResID		304
#define kTSCCNameResID		305
#define kTSCCInfoResID		306

// These flags specify information about the capabilities of the component
// Works with 1-bit, 8-bit, 16-bit and 32-bit Pixel Maps

#define kFFusionDecompressionFlags ( codecInfoDoes32 | codecInfoDoes16 | codecInfoDoes8 | codecInfoDoes1 | codecInfoDoesTemporal | cmpThreadSafe )

// These flags specify the possible format of compressed data produced by the component
// and the format of compressed files that the component can handle during decompression
// The component can decompress from files at 1-bit, 8-bit, 16-bit and 32-bit depths

#define kFFusionFormatFlags ( codecInfoDepth32 | codecInfoDepth24 | codecInfoDepth16 | codecInfoDepth8 | codecInfoDepth1 )

//---------------------------------------------------------------------------
// DivX 1 (MS-MPEG4 v1) Description Resource
//---------------------------------------------------------------------------

resource 'cdci' (kDivX1CodecInfoResID) {
	kDivX1Name,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// DivX 2 (MS-MPEG4 v2) Description Resource
//---------------------------------------------------------------------------

resource 'cdci' (kDivX2CodecInfoResID) {
	kDivX2Name,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// DivX 3 (MS-MPEG4 v3) Description Resource
//---------------------------------------------------------------------------

resource 'cdci' (kDivX3CodecInfoResID) {
	kDivX3Name,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// DivX 4 (OpenDivX) Description Resource
//---------------------------------------------------------------------------

resource 'cdci' (kDivX4CodecInfoResID) {
	kDivX4Name,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// DivX 5 Description Resource
//---------------------------------------------------------------------------

resource 'cdci' (kDivX5CodecInfoResID) {
	kDivX5Name,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// 3ivx Description Resource
//---------------------------------------------------------------------------

resource 'cdci' (k3ivxCodecInfoResID) {
	k3ivxName,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// XVID Description Resource
//---------------------------------------------------------------------------

resource 'cdci' (kXVIDCodecInfoResID) {
	kXVIDName,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// MPEG-4 Description Resource
//---------------------------------------------------------------------------

resource 'cdci' (kMPEG4CodecInfoResID) {
	kMPEG4Name,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// H.264 Description Resource
//---------------------------------------------------------------------------

resource 'cdci' (kH264CodecInfoResID) {
	kH264Name,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	190,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// Flash Video Description Resources
//---------------------------------------------------------------------------

resource 'cdci' (kFLV1CodecInfoResID) {
	kFLV1Name,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// Flash Screen Video Description Resources
//---------------------------------------------------------------------------

resource 'cdci' (kFlashSVCodecInfoResID) {
	kFlashSVName,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// VP6 Description Resources
//---------------------------------------------------------------------------

resource 'cdci' (kVP6CodecInfoResID) {
	kVP6Name,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// VP3 Description Resources
//---------------------------------------------------------------------------

resource 'cdci' (kVP3CodecInfoResID) {
	kVP3Name,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// I.263 Description Resources
//---------------------------------------------------------------------------

resource 'cdci' (kI263CodecInfoResID) {
	kI263Name,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// Huffyuv Description Resources
//---------------------------------------------------------------------------

resource 'cdci' (kHuffYUVCodecInfoResID) {
	kHuffYUVName,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// MPEG-1 Description Resources
//---------------------------------------------------------------------------

resource 'cdci' (kMPEG1CodecInfoResID) {
	kMPEG1Name,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// MPEG-2 Description Resources
//---------------------------------------------------------------------------

resource 'cdci' (kMPEG2CodecInfoResID) {
	kMPEG2Name,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// Fraps Description Resources
//---------------------------------------------------------------------------

resource 'cdci' (kFRAPSCodecInfoResID) {
	kFRAPSName,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// Snow Description Resources
//---------------------------------------------------------------------------

resource 'cdci' (kSnowCodecInfoResID) {
	kSnowName,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};

//---------------------------------------------------------------------------
// Nuv Description Resources
//---------------------------------------------------------------------------

resource 'cdci' (kNuvCodecInfoResID) {
	kNuvName,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};


//---------------------------------------------------------------------------
// TSCC Description Resources
//---------------------------------------------------------------------------

resource 'cdci' (kTSCCCodecInfoResID) {
	kTSCCName,				// Type
	1,					// Version
	1,					// Revision level
	kFFusionCodecManufacturer,			// Manufacturer
	kFFusionDecompressionFlags,		// Decompression Flags
	0,					// Compression Flags
	kFFusionFormatFlags,			// Format Flags
	128,					// Compression Accuracy
	128,					// Decomression Accuracy
	200,					// Compression Speed
	200,					// Decompression Speed
	128,					// Compression Level
	0,					// Reserved
	1,					// Minimum Height
	1,					// Minimum Width
	0,					// Decompression Pipeline Latency
	0,					// Compression Pipeline Latency
	0					// Private Data
};


//---------------------------------------------------------------------------
// MS-MPEG4 v1 Component
//---------------------------------------------------------------------------

resource 'thng' (256) {
	decompressorComponentType,		// Type			
	'MPG4',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX1NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX1InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (257) {
	decompressorComponentType,		// Type			
	'mpg4',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX1NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX1InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (258) {
	decompressorComponentType,		// Type			
	'DIV1',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX1NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX1InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (259) {
	decompressorComponentType,		// Type			
	'div1',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX1NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX1InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

//---------------------------------------------------------------------------
// MS-MPEG4 v2 Component
//---------------------------------------------------------------------------

resource 'thng' (260) {
	decompressorComponentType,		// Type			
	'MP42',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX2NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX2InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (261) {
	decompressorComponentType,		// Type			
	'mp42',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX2NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX2InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (262) {
	decompressorComponentType,		// Type			
	'DIV2',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX2NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX2InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (263) {
	decompressorComponentType,		// Type			
	'div2',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX2NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX2InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

//---------------------------------------------------------------------------
// DivX 3 (MS-MPEG4 v3) Components
//---------------------------------------------------------------------------

resource 'thng' (264) {
	decompressorComponentType,		// Type			
	'MPG3',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (265) {
	decompressorComponentType,		// Type			
	'mpg3',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (266) {
	decompressorComponentType,		// Type			
	'MP43',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (267) {
	decompressorComponentType,		// Type			
	'mp43',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (268) {
	decompressorComponentType,		// Type			
	'DIV3',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (269) {
	decompressorComponentType,		// Type			
	'div3',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (270) {
	decompressorComponentType,		// Type			
	'DIV4',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (271) {
	decompressorComponentType,		// Type			
	'div4',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (272) {
	decompressorComponentType,		// Type			
	'DIV5',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (273) {
	decompressorComponentType,		// Type			
	'div5',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (274) {
	decompressorComponentType,		// Type			
	'DIV6',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (275) {
	decompressorComponentType,		// Type			
	'div6',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (276) {
	decompressorComponentType,		// Type			
	'AP41',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (277) {
	decompressorComponentType,		// Type			
	'COL0',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (278) {
	decompressorComponentType,		// Type			
	'col0',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (279) {
	decompressorComponentType,		// Type			
	'COL1',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (280) {
	decompressorComponentType,		// Type			
	'col1',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX3NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

//---------------------------------------------------------------------------
// DivX 4 (OpenDivX) Components
//---------------------------------------------------------------------------

resource 'thng' (281) {
	decompressorComponentType,		// Type			
	'DIVX',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX4NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX4InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (282) {
	decompressorComponentType,		// Type			
	'divx',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX4NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX4InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (283) {
	decompressorComponentType,		// Type			
	'mp4s',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX4NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX4InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (284) {
	decompressorComponentType,		// Type			
	'MP4S',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX4NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX4InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (285) {
	decompressorComponentType,		// Type			
	'M4S2',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX4NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX4InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (286) {
	decompressorComponentType,		// Type			
	'm4s2',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX4NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX4InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (287) {
	decompressorComponentType,		// Type			
	0x04000000,				// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX4NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX4InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (288) {
	decompressorComponentType,		// Type			
	'UMP4',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX4NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX4InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

//---------------------------------------------------------------------------
// DivX 5 Component
//---------------------------------------------------------------------------

resource 'thng' (289) {
	decompressorComponentType,		// Type			
	'DX50',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kDivX5NameResID,			// Name ID
	'STR ',					// Info Type
	kDivX5InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

//---------------------------------------------------------------------------
// XVID Components
//---------------------------------------------------------------------------

resource 'thng' (290) {
	decompressorComponentType,		// Type			
	'XVID',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kXVIDNameResID,				// Name ID
	'STR ',					// Info Type
	kXVIDInfoResID,				// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (291) {
	decompressorComponentType,		// Type			
	'xvid',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kXVIDNameResID,				// Name ID
	'STR ',					// Info Type
	kXVIDInfoResID,				// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (292) {
	decompressorComponentType,		// Type			
	'XviD',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kXVIDNameResID,				// Name ID
	'STR ',					// Info Type
	kXVIDInfoResID,				// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (293) {
	decompressorComponentType,		// Type			
	'XVIX',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kXVIDNameResID,				// Name ID
	'STR ',					// Info Type
	kXVIDInfoResID,				// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (294) {
	decompressorComponentType,		// Type			
	'BLZ0',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kXVIDNameResID,				// Name ID
	'STR ',					// Info Type
	kXVIDInfoResID,				// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

//---------------------------------------------------------------------------
// 3ivx Components
//---------------------------------------------------------------------------

resource 'thng' (295) {
	decompressorComponentType,		// Type			
	'3IVD',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	k3ivxNameResID,				// Name ID
	'STR ',					// Info Type
	k3ivxInfoResID,				// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (296) {
	decompressorComponentType,		// Type			
	'3ivd',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	k3ivxNameResID,				// Name ID
	'STR ',					// Info Type
	k3ivxInfoResID,				// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (297) {
	decompressorComponentType,		// Type			
	'3IV2',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	k3ivxNameResID,				// Name ID
	'STR ',					// Info Type
	k3ivxInfoResID,				// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

resource 'thng' (298) {
	decompressorComponentType,		// Type			
	'3iv2',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	k3ivxNameResID,				// Name ID
	'STR ',					// Info Type
	k3ivxInfoResID,				// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
        };
};

//---------------------------------------------------------------------------
// Miscellaneous MPEG4 AVI FourCCs
//---------------------------------------------------------------------------
resource 'thng' (299) {
	decompressorComponentType,		// Type			
	'RMP4',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kMPEG4NameResID,				// Name ID
	'STR ',					// Info Type
	kMPEG4NameResID,				// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
	};
};

resource 'thng' (300) {
	decompressorComponentType,		// Type			
	'SEDG',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kMPEG4NameResID,				// Name ID
	'STR ',					// Info Type
	kMPEG4NameResID,				// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
	};
};

resource 'thng' (301) {
	decompressorComponentType,		// Type			
	'WV1F',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kMPEG4NameResID,				// Name ID
	'STR ',					// Info Type
	kMPEG4NameResID,				// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
	};
};

resource 'thng' (302) {
	decompressorComponentType,		// Type			
	'FMP4',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kMPEG4NameResID,				// Name ID
	'STR ',					// Info Type
	kMPEG4NameResID,				// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
	};
};

resource 'thng' (303) {
	decompressorComponentType,		// Type			
	'SMP4',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kMPEG4NameResID,				// Name ID
	'STR ',					// Info Type
	kMPEG4NameResID,				// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
	};
};

// XXX: can we do this without claiming Apple's manufacturer (and thus unregistering their decoder)?
resource 'thng' (304) {
	decompressorComponentType,		// Type			
	'mp4v',					// SubType
	'appl',			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kMPEG4NameResID,				// Name ID
	'STR ',					// Info Type
	kMPEG4NameResID,				// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion + 0x10000,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',				// Entry point found by symbol name 'dlle' resource
		256,				// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
	};
};


//---------------------------------------------------------------------------
// H264 Components
//---------------------------------------------------------------------------
resource 'thng' (305) {
	decompressorComponentType,		// Type			
	'H264',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kH264NameResID,			// Name ID
	'STR ',					// Info Type
	kH264InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (306) {
	decompressorComponentType,		// Type			
	'h264',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kH264NameResID,			// Name ID
	'STR ',					// Info Type
	kH264InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (307) {
	decompressorComponentType,		// Type			
	'X264',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kH264NameResID,			// Name ID
	'STR ',					// Info Type
	kH264InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (308) {
	decompressorComponentType,		// Type			
	'x264',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kH264NameResID,			// Name ID
	'STR ',					// Info Type
	kH264InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (309) {
	decompressorComponentType,		// Type			
	'DAVC',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kH264NameResID,			// Name ID
	'STR ',					// Info Type
	kH264InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (310) {
	decompressorComponentType,		// Type			
	'VSSH',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kH264NameResID,			// Name ID
	'STR ',					// Info Type
	kH264InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (311) {
	decompressorComponentType,		// Type			
	'AVC1',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kH264NameResID,			// Name ID
	'STR ',					// Info Type
	kH264InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (312) {
	decompressorComponentType,		// Type			
	'avc1',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kH264NameResID,			// Name ID
	'STR ',					// Info Type
	kH264InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};


//---------------------------------------------------------------------------
// Flash Video Codecs
//---------------------------------------------------------------------------
resource 'thng' (313) {
	decompressorComponentType,		// Type			
	'FLV1',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kFLV1NameResID,			// Name ID
	'STR ',					// Info Type
	kFLV1InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (314) {
	decompressorComponentType,		// Type			
	'FSV1',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kFlashSVNameResID,			// Name ID
	'STR ',					// Info Type
	kFlashSVInfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};


//---------------------------------------------------------------------------
// VP6 Components
//---------------------------------------------------------------------------
resource 'thng' (315) {
	decompressorComponentType,		// Type			
	'VP60',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kVP6NameResID,			// Name ID
	'STR ',					// Info Type
	kVP6InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (316) {
	decompressorComponentType,		// Type			
	'VP61',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kVP6NameResID,			// Name ID
	'STR ',					// Info Type
	kVP6InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (317) {
	decompressorComponentType,		// Type			
	'VP62',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kVP6NameResID,			// Name ID
	'STR ',					// Info Type
	kVP6InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (318) {
	decompressorComponentType,		// Type			
	'VP6F',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kVP6NameResID,			// Name ID
	'STR ',					// Info Type
	kVP6InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};


//---------------------------------------------------------------------------
// Intel H.263 Components
//---------------------------------------------------------------------------
resource 'thng' (319) {
	decompressorComponentType,		// Type			
	'I263',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kI263NameResID,			// Name ID
	'STR ',					// Info Type
	kI263InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (320) {
	decompressorComponentType,		// Type			
	'i263',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kI263NameResID,			// Name ID
	'STR ',					// Info Type
	kI263InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};


//---------------------------------------------------------------------------
// VP3 Components
//---------------------------------------------------------------------------
resource 'thng' (321) {
	decompressorComponentType,		// Type			
	'VP30',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kVP3NameResID,			// Name ID
	'STR ',					// Info Type
	kVP3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (322) {
	decompressorComponentType,		// Type			
	'VP31',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kVP3NameResID,			// Name ID
	'STR ',					// Info Type
	kVP3InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};


//---------------------------------------------------------------------------
// VP3 Components
//---------------------------------------------------------------------------
resource 'thng' (323) {
	decompressorComponentType,		// Type			
	'HFYU',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kHuffYUVNameResID,			// Name ID
	'STR ',					// Info Type
	kHuffYUVInfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (324) {
	decompressorComponentType,		// Type			
	'FFVH',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kHuffYUVNameResID,			// Name ID
	'STR ',					// Info Type
	kHuffYUVInfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

//---------------------------------------------------------------------------
// MPEG-1 Components
//---------------------------------------------------------------------------
resource 'thng' (325) {
	decompressorComponentType,		// Type			
	'MPEG',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kMPEG1NameResID,			// Name ID
	'STR ',					// Info Type
	kMPEG1InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (326) {
	decompressorComponentType,		// Type			
	'mpg1',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kMPEG1NameResID,			// Name ID
	'STR ',					// Info Type
	kMPEG1InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (327) {
	decompressorComponentType,		// Type			
	'mp1v',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kMPEG1NameResID,			// Name ID
	'STR ',					// Info Type
	kMPEG1InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

//---------------------------------------------------------------------------
// MPEG-2 Components
//---------------------------------------------------------------------------
resource 'thng' (328) {
	decompressorComponentType,		// Type			
	'MPG2',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kMPEG2NameResID,			// Name ID
	'STR ',					// Info Type
	kMPEG2InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (329) {
	decompressorComponentType,		// Type			
	'mpg2',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kMPEG2NameResID,			// Name ID
	'STR ',					// Info Type
	kMPEG2InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (330) {
	decompressorComponentType,		// Type			
	'mp2v',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kMPEG2NameResID,			// Name ID
	'STR ',					// Info Type
	kMPEG2InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

//---------------------------------------------------------------------------
// Fraps Components
//---------------------------------------------------------------------------
resource 'thng' (331) {
	decompressorComponentType,              // Type
	'FPS1',                                 // SubType
	kFFusionCodecManufacturer,                      // Manufacturer
	0,                                      // - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',                                 // Name Type
	kFRAPSNameResID,                        // Name ID
	'STR ',                                 // Info Type
	kFRAPSInfoResID,                        // Info ID
	0,                                      // Icon Type
	0,                                      // Icon ID
	kFFusionCodecVersion,                   // Version
	componentHasMultiplePlatforms +         // Registration Flags 
	componentDoAutoVersion,                 // Registration Flags
	0,                                      // Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',                         // Entry point found by symbol name 'dlle' resource
	256,                            // ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

resource 'thng' (332) {
	decompressorComponentType,		// Type			
	'FLV4',					// SubType
	kFFusionCodecManufacturer,			// Manufacturer
	0,					// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',					// Name Type
	kVP6NameResID,			// Name ID
	'STR ',					// Info Type
	kVP6InfoResID,			// Info ID
	0,					// Icon Type
	0,					// Icon ID
	kFFusionCodecVersion,			// Version
	componentHasMultiplePlatforms +		// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,					// Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',				// Entry point found by symbol name 'dlle' resource
	256,				// ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

//---------------------------------------------------------------------------
// Snow Components
//---------------------------------------------------------------------------
resource 'thng' (333) {
	decompressorComponentType,              // Type
	'SNOW',                                 // SubType
	kFFusionCodecManufacturer,                      // Manufacturer
	0,                                      // - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',                                 // Name Type
	kSnowNameResID,                        // Name ID
	'STR ',                                 // Info Type
	kSnowInfoResID,                        // Info ID
	0,                                      // Icon Type
	0,                                      // Icon ID
	kFFusionCodecVersion,                   // Version
	componentHasMultiplePlatforms +         // Registration Flags 
	componentDoAutoVersion,                 // Registration Flags
	0,                                      // Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',                         // Entry point found by symbol name 'dlle' resource
	256,                            // ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

//---------------------------------------------------------------------------
// New Nuv Components
//---------------------------------------------------------------------------
resource 'thng' (334) {
	decompressorComponentType,              // Type
	'RJPG',                                 // SubType
	kFFusionCodecManufacturer,                      // Manufacturer
	0,                                      // - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',                                 // Name Type
	kNuvNameResID,                        // Name ID
	'STR ',                                 // Info Type
	kNuvInfoResID,                        // Info ID
	0,                                      // Icon Type
	0,                                      // Icon ID
	kFFusionCodecVersion,                   // Version
	componentHasMultiplePlatforms +         // Registration Flags 
	componentDoAutoVersion,                 // Registration Flags
	0,                                      // Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',                         // Entry point found by symbol name 'dlle' resource
	256,                            // ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

//---------------------------------------------------------------------------
// Old Nuv Components
//---------------------------------------------------------------------------
resource 'thng' (335) {
	decompressorComponentType,              // Type
	'NUV1',                                 // SubType
	kFFusionCodecManufacturer,                      // Manufacturer
	0,                                      // - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',                                 // Name Type
	kNuvNameResID,                        // Name ID
	'STR ',                                 // Info Type
	kNuvInfoResID,                        // Info ID
	0,                                      // Icon Type
	0,                                      // Icon ID
	kFFusionCodecVersion,                   // Version
	componentHasMultiplePlatforms +         // Registration Flags 
	componentDoAutoVersion,                 // Registration Flags
	0,                                      // Resource ID of Icon Family
{
	kFFusionDecompressionFlags, 
	'dlle',                         // Entry point found by symbol name 'dlle' resource
	256,                            // ID of 'dlle' resource
	platformPowerPCNativeEntryPoint,
	kFFusionDecompressionFlags,
	'dlle',
	256,
	platformIA32NativeEntryPoint,
};
};

//---------------------------------------------------------------------------
// TSCC Components
//---------------------------------------------------------------------------
resource 'thng' (336) {
	decompressorComponentType,              // Type
	'tscc',                                 // SubType
	kFFusionCodecManufacturer,                      // Manufacturer
	0,                                      // - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',                                 // Name Type
	kTSCCNameResID,                        // Name ID
	'STR ',                                 // Info Type
	kTSCCInfoResID,                        // Info ID
	0,                                      // Icon Type
	0,                                      // Icon ID
	kFFusionCodecVersion,                   // Version
	componentHasMultiplePlatforms +         // Registration Flags 
	componentDoAutoVersion,                 // Registration Flags
	0,                                      // Resource ID of Icon Family
	{
		kFFusionDecompressionFlags, 
		'dlle',                         // Entry point found by symbol name 'dlle' resource
		256,                            // ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kFFusionDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
	};
};


//---------------------------------------------------------------------------
// Component Name Resources
//---------------------------------------------------------------------------

resource 'STR ' (kDivX1NameResID) {
	"MS-MPEG4 v1 (Perian)"
};

resource 'STR ' (kDivX2NameResID) {
	"MS-MPEG4 v2 (Perian)"
};

resource 'STR ' (kDivX3NameResID) {
	"DivX 3 (Perian)"
};

resource 'STR ' (kDivX4NameResID) {
	"DivX 4 (Perian)"
};

resource 'STR ' (kDivX5NameResID) {
	"DivX 5 (Perian)"
};

resource 'STR ' (k3ivxNameResID) {
	"3ivx (Perian)"
};

resource 'STR ' (kXVIDNameResID) {
	"Xvid (Perian)"
};

resource 'STR ' (kMPEG4NameResID) {
	"MPEG-4 (Perian)"
};

resource 'STR ' (kH264NameResID) {
	"H.264 (Perian)"
};

resource 'STR ' (kFLV1NameResID) {
	"Sorenson H.263 (Perian)"
};

resource 'STR ' (kFlashSVNameResID) {
	"Flash Screen Video (Perian)"
};

resource 'STR ' (kVP6NameResID) {
	"TrueMotion VP6 (Perian)"
};

resource 'STR ' (kI263NameResID) {
	"Intel H.263 (Perian)"
};

resource 'STR ' (kVP3NameResID) {
	"On2 VP3 (Perian)"
};

resource 'STR ' (kHuffYUVNameResID) {
	"HuffYUV (Perian)"
};

resource 'STR ' (kMPEG1NameResID) {
	"MPEG-1 (Perian)"
};

resource 'STR ' (kMPEG2NameResID) {
	"MPEG-2 (Perian)"
};

resource 'STR ' (kFRAPSNameResID) {
	"Fraps (Perian)"
};

resource 'STR ' (kSnowNameResID) {
	"Snow (Perian)"
};

resource 'STR ' (kNuvNameResID) {
	"NuppelVideo (Perian)"
};

resource 'STR ' (kTSCCNameResID) {
	"Techsmith Screen Capture (Perian)"
};

//---------------------------------------------------------------------------
// Component Name Resources
//---------------------------------------------------------------------------

resource 'STR ' (kDivX1InfoResID) {
	"Decompresses video stored in MS-MPEG4 version 1 format."
};

resource 'STR ' (kDivX2InfoResID) {
	"Decompresses video stored in MS-MPEG4 version 2 format."
};

resource 'STR ' (kDivX3InfoResID) {
	"Decompresses video stored in DivX 3.11 alpha format."
};

resource 'STR ' (kDivX4InfoResID) {
	"Decompresses video stored in OpenDivX format."
};

resource 'STR ' (kDivX5InfoResID) {
	"Decompresses video stored in DivX 5 format."
};

resource 'STR ' (k3ivxInfoResID) {
	"Decompresses video stored in 3ivx format."
};

resource 'STR ' (kXVIDInfoResID) {
	"Decompresses video stored in Xvid format."
};

resource 'STR ' (kMPEG4InfoResID) {
	"Decompresses video stored in MPEG-4 format."
};

resource 'STR ' (kH264InfoResID) {
	"Decompresses video stored in H.264 format."
};

resource 'STR ' (kFLV1InfoResID) {
	"Decompresses video stored in Sorenson H.263 format."
};

resource 'STR ' (kFlashSVInfoResID) {
	"Decompresses video stored in Flash Screen Video format."
};

resource 'STR ' (kVP6InfoResID) {
	"Decompresses video stored in TrueMotion VP6 format."
};

resource 'STR ' (kI263InfoResID) {
	"Decompresses video stored in Intel H.263 format."
};

resource 'STR ' (kVP3InfoResID) {
	"Decompresses video stored in On2 VP3 format."
};

resource 'STR ' (kHuffYUVInfoResID) {
	"Decompresses video stored in HuffYUV format."
};

resource 'STR ' (kMPEG1InfoResID) {
	"Decompresses video stored in MPEG-1 format."
};

resource 'STR ' (kMPEG2InfoResID) {
	"Decompresses video stored in MPEG-2 format."
};

resource 'STR ' (kFRAPSInfoResID) {
	"Decompresses video stored in Fraps format."
};

resource 'STR ' (kSnowInfoResID) {
	"Decompresses video stored in Snow format."
};

resource 'STR ' (kNuvInfoResID) {
	"Decompresses video stored in NuppelVideo format."
};

resource 'STR ' (kTSCCInfoResID) {
	"Decompresses video stored in Techsmith Screen Capture format."
};

//---------------------------------------------------------------------------
// Code Entry Point for Mach-O
//---------------------------------------------------------------------------

resource 'dlle' (256) {
        "FFusionCodecComponentDispatch"
};

