/*
 * TextSubCodec.r
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

// These flags specify information about the capabilities of the component
// Works with 1-bit, 8-bit, 16-bit and 32-bit Pixel Maps
#define kDecompressionFlags ( codecInfoDoes32 | cmpThreadSafe | codecInfoDoesStretch | codecInfoDoesShrink )

// These flags specify the possible format of compressed data produced by the component
// and the format of compressed files that the component can handle during decompression
// The component can decompress from files at 1-bit, 8-bit, 16-bit, 24-bit and 32-bit depths
#define kFormatFlags	( codecInfoDepth32 )

#include "PerianResourceIDs.h"

#define kStartTHNGResID kSSASubCodecResourceID
#define kCodecManufacturer kTextCodecManufacturer
#define kCodecVersion kTextSubCodecVersion
#define kEntryPointID kTextSubCodecResourceID

#define kCodecInfoResID kSSASubCodecResourceID
#define kCodecName "SSA Text Subtitle"
#define kCodecDescription "Renders subtitles stored in SubStation Alpha format."
#define kCodecSubType kSubFormatSSA
#include "PerianResources.r"

#define kCodecInfoResID kTextSubCodecResourceID
#define kCodecName "Text Subtitle"
#define kCodecDescription "Renders subtitles stored as text."
#define kCodecSubType kSubFormatUTF8
#include "PerianResources.r"

// Code Entry Point for Mach-O and Windows
resource 'dlle' (kTextSubCodecResourceID) {
	"TextSubCodecComponentDispatch"
};
