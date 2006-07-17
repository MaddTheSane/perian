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
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#define kXVIDName		"XVID"

// Codec names Resource ID

#define kDivX1NameResID		256
#define kDivX2NameResID		257
#define	kDivX3NameResID		258
#define	kDivX4NameResID		259
#define	kDivX5NameResID		260
#define	k3ivxNameResID		261
#define	kXVIDNameResID		262
#define kH264NameResID		270

// Codec infos Resource ID

#define kDivX1InfoResID		263
#define kDivX2InfoResID		264
#define	kDivX3InfoResID		265
#define	kDivX4InfoResID		266
#define	kDivX5InfoResID		267
#define	k3ivxInfoResID		268
#define	kXVIDInfoResID		269
#define kH264InfoResID		271

// These flags specify information about the capabilities of the component
// Works with 1-bit, 8-bit, 16-bit and 32-bit Pixel Maps

#define kFFusionDecompressionFlags ( codecInfoDoes32 | codecInfoDoes16 | codecInfoDoes8 | codecInfoDoes1 )

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

// H264
resource 'thng' (299) {
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

//---------------------------------------------------------------------------
// Component Name Resources
//---------------------------------------------------------------------------

resource 'STR ' (kDivX1NameResID) {
	"MS-MPEG4 v1 Decoder"
};

resource 'STR ' (kH264NameResID) {
	"H264 Decoder"
};

resource 'STR ' (kDivX2NameResID) {
	"MS-MPEG4 v2 Decoder"
};

resource 'STR ' (kDivX3NameResID) {
	"DivX 3 Decoder"
};

resource 'STR ' (kDivX4NameResID) {
	"DivX 4 Decoder"
};

resource 'STR ' (kDivX5NameResID) {
	"DivX 5 Decoder"
};

resource 'STR ' (k3ivxNameResID) {
	"3ivx Decoder"
};

resource 'STR ' (kXVIDNameResID) {
	"XVID Decoder"
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
	"Decompresses video stored in XVID format."
};

resource 'STR ' (kH264InfoResID) {
	"Decompresses video stored in H264 format."
};

//---------------------------------------------------------------------------
// Code Entry Point for Mach-O
//---------------------------------------------------------------------------

resource 'dlle' (256) {
        "FFusionCodecComponentDispatch"
};

