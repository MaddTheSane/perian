/*
 *  PerianResources.r
 *  Perian
 *
 *  Created by Graham Booker on 7/10/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef kCurrentTHNGResID
#define kCurrentTHNGResID kStartTHNGResID
#endif

#ifdef kCodecName

resource 'cdci' (kCodecInfoResID) {
	kCodecName,						// Type
	1,								// Version
	1,								// Revision level
	kCodecManufacturer,				// Manufacturer
	kDecompressionFlags,			// Decompression Flags
	0,								// Compression Flags
	kFormatFlags,					// Format Flags
	128,							// Compression Accuracy
	128,							// Decomression Accuracy
	200,							// Compression Speed
	200,							// Decompression Speed
	128,							// Compression Level
	0,								// Reserved
	1,								// Minimum Height
	1,								// Minimum Width
	0,								// Decompression Pipeline Latency
	0,								// Compression Pipeline Latency
	0								// Private Data
};

#define kCodecNameResID kCodecCount + kCodecInfoResID

resource 'STR ' (kCodecNameResID) {
	kCodecName " (Perian)"
};

resource 'STR ' (kCodecInfoResID) {
	kCodecDescription
};

#endif

resource 'thng' (kCurrentTHNGResID) {
	decompressorComponentType,		// Type			
	kCodecSubType,					// SubType
	kCodecManufacturer,				// Manufacturer
	0,								// - use componentHasMultiplePlatforms
	0,
	0,
	0,
	'STR ',							// Name Type
	kCodecNameResID,				// Name ID
	'STR ',							// Info Type
	kCodecInfoResID,				// Info ID
	0,								// Icon Type
	0,								// Icon ID
	kCodecVersion,					// Version
	componentHasMultiplePlatforms +	// Registration Flags 
	componentDoAutoVersion,			// Registration Flags
	0,								// Resource ID of Icon Family
	{
		kDecompressionFlags, 
		'dlle',						// Entry point found by symbol name 'dlle' resource
		256,						// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
		kDecompressionFlags,
		'dlle',
		256,
		platformIA32NativeEntryPoint,
	};
};

#define IncrementAmount 1
#if (kCurrentTHNGResID  - kStartTHNGResID) & 128
#define CurrentIncrementedAmount128 128
#else
#define CurrentIncrementedAmount128 0
#endif

#if (kCurrentTHNGResID  - kStartTHNGResID) & 64
#define CurrentIncrementedAmount64 64
#else
#define CurrentIncrementedAmount64 0
#endif

#if (kCurrentTHNGResID  - kStartTHNGResID) & 32
#define CurrentIncrementedAmount32 32
#else
#define CurrentIncrementedAmount32 0
#endif

#if (kCurrentTHNGResID  - kStartTHNGResID) & 16
#define CurrentIncrementedAmount16 16
#else
#define CurrentIncrementedAmount16 0
#endif

#if (kCurrentTHNGResID  - kStartTHNGResID) & 8
#define CurrentIncrementedAmount8 8
#else
#define CurrentIncrementedAmount8 0
#endif

#if (kCurrentTHNGResID  - kStartTHNGResID) & 4
#define CurrentIncrementedAmount4 4
#else
#define CurrentIncrementedAmount4 0
#endif

#if (kCurrentTHNGResID  - kStartTHNGResID) & 2
#define CurrentIncrementedAmount2 2
#else
#define CurrentIncrementedAmount2 0
#endif

#if (kCurrentTHNGResID  - kStartTHNGResID) & 1
#define CurrentIncrementedAmount1 1
#else
#define CurrentIncrementedAmount1 0
#endif

#if kCurrentTHNGResID == kStartTHNGResID
#define kCurrentTHNGResID kStartTHNGResID + IncrementAmount + CurrentIncrementedAmount128 + CurrentIncrementedAmount64 + CurrentIncrementedAmount32 + CurrentIncrementedAmount16 + CurrentIncrementedAmount8 + CurrentIncrementedAmount4 + CurrentIncrementedAmount2 + CurrentIncrementedAmount1
#endif

#undef kCodecSubType
#undef kCodecName
#undef kCodecDescription
