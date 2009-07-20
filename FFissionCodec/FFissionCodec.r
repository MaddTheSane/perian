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

#define TARGET_REZ_CARBON_MACHO 1
#define thng_RezTemplateVersion 1	// multiplatform 'thng' resource
#define cfrg_RezTemplateVersion 1	// extended version of 'cfrg' resource

#include <Carbon/Carbon.r>
#include <QuickTime/QuickTime.r>

#include "PerianResourceIDs.h"
#include "CodecIDs.h"

#define AudioComponentType
#define decompressorComponentType 'adec'
#ifndef cmpThreadSafeOnMac
	#define cmpThreadSafeOnMac 0x10000000
#endif
#define kDecompressionFlags cmpThreadSafeOnMac

#define kStartTHNGResID kWMA1MSCodecResourceID
#define kCodecManufacturer kFFissionCodecManufacturer
#define kCodecVersion kFFissionCodecVersion
#define kEntryPointID kWMA1MSCodecResourceID

#define kCodecInfoResID kWMA1MSCodecResourceID
#define kCodecName "Windows Media Audio 1"
#define kCodecDescription "An AudioCodec that decodes WMA v1 into linear PCM"
#define kCodecSubType kAudioFormatWMA2MS
#include "PerianResources.r"

#define kCodecInfoResID kFlashADPCMCodecResourceID
#define kCodecName "Flash ADPCM"
#define kCodecDescription "An AudioCodec that decodes Flash ADPCM into linear PCM"
#define kCodecSubType kAudioFormatFlashADPCM
#include "PerianResources.r"

#define kEntryPointID kXiphVorbisCodecResourceID

#define kCodecInfoResID kXiphVorbisCodecResourceID
#define kCodecName "Vorbis"
#define kCodecDescription "An AudioCodec that decodes Vorbis into linear PCM"
#define kCodecSubType kAudioFormatXiphVorbis
#include "PerianResources.r"

#define kCodecInfoResID kMPEG1L1CodecResourceID
#define kCodecName "MPEG-1 Layer 1"
#define kCodecDescription "An AudioCodec that decodes MPEG-1 layer 1 audio into linear PCM"
#define kCodecSubType '.mp1'
#include "PerianResources.r"

#define kCodecInfoResID kMPEG1L12CodecResourceID
#define kCodecName "MPEG-1 Layer 1/2"
#define kCodecDescription "An AudioCodec that decodes MPEG-1 layer 1 or 2 audio into linear PCM"
#define kCodecSubType 0x6d730050
#include "PerianResources.r"

#define kCodecInfoResID kMPEG1L2CodecResourceID
#define kCodecName "MPEG-1 Layer 2"
#define kCodecDescription "An AudioCodec that decodes MPEG-1 layer 2 audio into linear PCM"
#define kCodecSubType '.mp2'
#include "PerianResources.r"

#define kCodecInfoResID kTrueAudioCodecResourceID
#define kCodecName "True Audio"
#define kCodecDescription "An AudioCodec that decodes True Audio into linear PCM"
#define kCodecSubType kAudioFormatTTA
#include "PerianResources.r"

#define kCodecInfoResID kDTSCodecResourceID
#define kCodecName "DTS Coherent Acoustics"
#define kCodecDescription "An AudioCodec that decodes DCA Audio into linear PCM"
#define kCodecSubType kAudioFormatDTS
#include "PerianResources.r"

#define kCodecInfoResID kNellymoserCodecResourceID
#define kCodecName "Nellymoser ASAO"
#define kCodecDescription "An AudioCodec that decodes Nellymoser ASAO into linear PCM"
#define kCodecSubType kAudioFormatNellymoser
#include "PerianResources.r"

resource 'dlle' (kWMA1MSCodecResourceID) {
	"FFissionDecoderEntry"
};

resource 'dlle' (kXiphVorbisCodecResourceID) {
	"FFissionVBRDecoderEntry"
};