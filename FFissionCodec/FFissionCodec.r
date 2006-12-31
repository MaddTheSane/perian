//=============================================================================
//	The thng and related resources
//
//	The definitions below use the following macros, all of which must be
//	defined. Note that kPrimaryResourceID is used to define two 'STR '
//	resources with consecutive IDs so be sure to space them at least two'
//	apart. Here's a sample of how to do the defines:
//	
//	#define kPrimaryResourceID				128
//	#define kComponentType					'aenc'
//	#define kComponentSubtype				'ima4'
//	#define kComponentManufacturer			'appl'
//	#define	kComponentFlags					0
//	#define kComponentVersion				0x00010000
//	#define kComponentName					"Apple IMA4 Encoder"
//	#define kComponentInfo					"An AudioCodec that encodes linear PCM data into IMA4"
//	#define kComponentEntryPoint			"ACAppleIMA4EncoderEntry"
//	#define	kComponentPublicResourceMapType	0
//	#define kComponentIsThreadSafe			1
//=============================================================================

#include "FFusionCodec.h"
#include "CodecIDs.h"

#define BUILD_UNIVERSAL

#define kComponentManufacturer			'Peri'
#define kComponentFlags					0
#define kComponentVersion				kFFusionCodecVersion
#define kComponentEntryPoint			"FFissionDecoderEntry"
#define kComponentPublicResourceMapType	0
#define kComponentIsThreadSafe			1


#define kPrimaryResourceID				128
#define kComponentType					'adec'
#define kComponentSubtype				kAudioFormatWMA1MS
#define kComponentName					"Windows Media Audio 1"
#define kComponentInfo					"An AudioCodec that decodes WMA v1 into linear PCM"
#include "XCAResources.r"

#define kPrimaryResourceID				130
#define kComponentType					'adec'
#define kComponentSubtype				kAudioFormatWMA2MS
#define kComponentName					"Windows Media Audio 2"
#define kComponentInfo					"An AudioCodec that decodes WMA v2 into linear PCM"
#include "XCAResources.r"
