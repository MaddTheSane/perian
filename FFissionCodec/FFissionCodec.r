/*
 * FFissionCodec.r
 *
 * This file is part of Perian.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
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

#define kPrimaryResourceID				132
#define kComponentType					'adec'
#define kComponentSubtype				kAudioFormatFlashADPCM
#define kComponentName					"Flash ADPCM"
#define kComponentInfo					"An AudioCodec that decodes Flash ADPCM into linear PCM"
#include "XCAResources.r"


#define kComponentEntryPoint			"FFissionVBRDecoderEntry"

#define kPrimaryResourceID				134
#define kComponentType					'adec'
#define kComponentSubtype				kAudioFormatXiphVorbis
#define kComponentName					"Vorbis"
#define kComponentInfo					"An AudioCodec that decodes Vorbis audio into linear PCM"
#include "XCAResources.r"

#define kPrimaryResourceID				136
#define kComponentType					'adec'
#define kComponentSubtype				'.mp1'
#define kComponentName					"MPEG-1 Layer 1"
#define kComponentInfo					"An AudioCodec that decodes MPEG-1 layer 1 audio into linear PCM"
#include "XCAResources.r"

#define kPrimaryResourceID				138
#define kComponentType					'adec'
#define kComponentSubtype				0x6d730050
#define kComponentName					"MPEG-1 Layer 1/2"
#define kComponentInfo					"An AudioCodec that decodes MPEG-1 layer 1 or 2 audio into linear PCM"
#include "XCAResources.r"

#define kPrimaryResourceID				140
#define kComponentType					'adec'
#define kComponentSubtype				'.mp2'
#define kComponentName					"MPEG-1 Layer 2"
#define kComponentInfo					"An AudioCodec that decodes MPEG-1 layer 2 audio into linear PCM"
#include "XCAResources.r"

#define kPrimaryResourceID				142
#define kComponentType					'adec'
#define kComponentSubtype				kAudioFormatTTA
#define kComponentName					"True Audio"
#define kComponentInfo					"An AudioCodec that decodes True Audio into linear PCM"
#include "XCAResources.r"

#define kPrimaryResourceID				144
#define kComponentType					'adec'
#define kComponentSubtype				kAudioFormatDTS
#define kComponentName					"DTS Coherent Acoustics"
#define kComponentInfo					"An AudioCodec that decodes DCA Audio into linear PCM"
#include "XCAResources.r"

#define kPrimaryResourceID				146
#define kComponentType					'adec'
#define kComponentSubtype				kAudioFormatNellymoser
#define kComponentName					"Nellymoser ASAO"
#define kComponentInfo					"An AudioCodec that decodes Nellymoser ASAO into linear PCM"
#include "XCAResources.r"