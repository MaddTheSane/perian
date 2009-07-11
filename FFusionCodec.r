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

// These flags specify information about the capabilities of the component
// Works with 1-bit, 8-bit, 16-bit and 32-bit Pixel Maps

#define kDecompressionFlags ( codecInfoDoes32 | codecInfoDoes16 | codecInfoDoes8 | codecInfoDoes1 | codecInfoDoesTemporal | cmpThreadSafe )

// These flags specify the possible format of compressed data produced by the component
// and the format of compressed files that the component can handle during decompression
// The component can decompress from files at 1-bit, 8-bit, 16-bit and 32-bit depths

#define kFormatFlags ( codecInfoDepth32 | codecInfoDepth24 | codecInfoDepth16 | codecInfoDepth8 | codecInfoDepth1 )

#include "FFusionCodec.h"

#define kStartTHNGResID 256
#define kCodecManufacturer kFFusionCodecManufacturer
#define kCodecCount kFFusionCodecCount
#define kCodecVersion kFFusionCodecVersion

#define kCodecInfoResID kDivX1CodecInfoResID
#define kCodecName "MS-MPEG4 v1"
#define kCodecDescription "Decompresses video stored in MS-MPEG4 version 1 format."
#define kCodecSubType 'MPG4'
#include "PerianResources.r"
#define kCodecSubType 'mpg4'
#include "PerianResources.r"
#define kCodecSubType 'DIV1'
#include "PerianResources.r"
#define kCodecSubType 'div1'
#include "PerianResources.r"

#define kCodecInfoResID kDivX2CodecInfoResID
#define kCodecName "MS-MPEG4 v2"
#define kCodecDescription "Decompresses video stored in MS-MPEG4 version 2 format."
#define kCodecSubType 'MP42'
#include "PerianResources.r"
#define kCodecSubType 'mp42'
#include "PerianResources.r"
#define kCodecSubType 'DIV2'
#include "PerianResources.r"
#define kCodecSubType 'div2'
#include "PerianResources.r"

#define kCodecInfoResID kDivX3CodecInfoResID
#define kCodecName "DivX 3"
#define kCodecDescription "Decompresses video stored in DivX 3.11 alpha format."
#define kCodecSubType 'MPG3'
#include "PerianResources.r"
#define kCodecSubType 'mpg3'
#include "PerianResources.r"
#define kCodecSubType 'MP43'
#include "PerianResources.r"
#define kCodecSubType 'mp43'
#include "PerianResources.r"
#define kCodecSubType 'DIV3'
#include "PerianResources.r"
#define kCodecSubType 'div3'
#include "PerianResources.r"
#define kCodecSubType 'DIV4'
#include "PerianResources.r"
#define kCodecSubType 'div4'
#include "PerianResources.r"
#define kCodecSubType 'DIV5'
#include "PerianResources.r"
#define kCodecSubType 'div5'
#include "PerianResources.r"
#define kCodecSubType 'DIV6'
#include "PerianResources.r"
#define kCodecSubType 'div6'
#include "PerianResources.r"
#define kCodecSubType 'AP41'
#include "PerianResources.r"
#define kCodecSubType 'COL0'
#include "PerianResources.r"
#define kCodecSubType 'col0'
#include "PerianResources.r"
#define kCodecSubType 'COL1'
#include "PerianResources.r"
#define kCodecSubType 'col1'
#include "PerianResources.r"

#define kCodecInfoResID kDivX4CodecInfoResID
#define kCodecName "DivX 4"
#define kCodecDescription "Decompresses video stored in OpenDivX format."
#define kCodecSubType 'DIVX'
#include "PerianResources.r"
#define kCodecSubType 'divx'
#include "PerianResources.r"
#define kCodecSubType 'mp4s'
#include "PerianResources.r"
#define kCodecSubType 'MP4S'
#include "PerianResources.r"
#define kCodecSubType 'M4S2'
#include "PerianResources.r"
#define kCodecSubType 'm4s2'
#include "PerianResources.r"
#define kCodecSubType 0x04000000
#include "PerianResources.r"
#define kCodecSubType 'UMP4'
#include "PerianResources.r"

#define kCodecInfoResID kDivX5CodecInfoResID
#define kCodecName "DivX 5"
#define kCodecDescription "Decompresses video stored in DivX 5 format."
#define kCodecSubType 'DX50'
#include "PerianResources.r"

#define kCodecInfoResID k3ivxCodecInfoResID
#define kCodecName "3ivx"
#define kCodecDescription "Decompresses video stored in 3ivx format."
#define kCodecSubType '3IVD'
#include "PerianResources.r"
#define kCodecSubType '3ivd'
#include "PerianResources.r"
#define kCodecSubType '3IV2'
#include "PerianResources.r"
#define kCodecSubType '3iv2'
#include "PerianResources.r"

#define kCodecInfoResID kXVIDCodecInfoResID
#define kCodecName "Xvid"
#define kCodecDescription "Decompresses video stored in Xvid format."
#define kCodecSubType 'XVID'
#include "PerianResources.r"
#define kCodecSubType 'xvid'
#include "PerianResources.r"
#define kCodecSubType 'XviD'
#include "PerianResources.r"
#define kCodecSubType 'XVIX'
#include "PerianResources.r"
#define kCodecSubType 'BLZ0'
#include "PerianResources.r"

#define kCodecInfoResID kMPEG4CodecInfoResID
#define kCodecName "MPEG-4"
#define kCodecDescription "Decompresses video stored in MPEG-4 format."
#define kCodecSubType 'RMP4'
#include "PerianResources.r"
#define kCodecSubType 'SEDG'
#include "PerianResources.r"
#define kCodecSubType 'WV1F'
#include "PerianResources.r"
#define kCodecSubType 'FMP4'
#include "PerianResources.r"
#define kCodecSubType 'SMP4'
#include "PerianResources.r"
// XXX: can we do this without claiming Apple's manufacturer (and thus unregistering their decoder)?
#define kCodecManufacturer 'appl'
#define kCodecSubType 'mp4v'
#include "PerianResources.r"
#define kCodecManufacturer kFFusionCodecManufacturer

#define kCodecInfoResID kH264CodecInfoResID
#define kCodecName "H.264"
#define kCodecDescription "Decompresses video stored in H.264 format."
#define kCodecSubType 'H264'
#include "PerianResources.r"
#define kCodecSubType 'h264'
#include "PerianResources.r"
#define kCodecSubType 'X264'
#include "PerianResources.r"
#define kCodecSubType 'x264'
#include "PerianResources.r"
#define kCodecSubType 'DAVC'
#include "PerianResources.r"
#define kCodecSubType 'VSSH'
#include "PerianResources.r"
#define kCodecSubType 'AVC1'
#include "PerianResources.r"
#define kCodecSubType 'avc1'
#include "PerianResources.r"

#define kCodecInfoResID kFLV1CodecInfoResID
#define kCodecName "Sorenson H.263"
#define kCodecDescription "Decompresses video stored in Sorenson H.263 format."
#define kCodecSubType 'FLV1'
#include "PerianResources.r"

#define kCodecInfoResID kFlashSVCodecInfoResID
#define kCodecName "Flash Screen Video"
#define kCodecDescription "Decompresses video stored in Flash Screen Video format."
#define kCodecSubType 'FSV1'
#include "PerianResources.r"

#define kCodecInfoResID kVP6CodecInfoResID
#define kCodecName "TrueMotion VP6"
#define kCodecDescription "Decompresses video stored in On2 VP6 format."
#define kCodecSubType 'VP60'
#include "PerianResources.r"
#define kCodecSubType 'VP61'
#include "PerianResources.r"
#define kCodecSubType 'VP62'
#include "PerianResources.r"
#define kCodecSubType 'VP6F'
#include "PerianResources.r"
#define kCodecSubType 'FLV4'
#include "PerianResources.r"

#define kCodecInfoResID kI263CodecInfoResID
#define kCodecName "Intel H.263"
#define kCodecDescription "Decompresses video stored in Intel H.263 format."
#define kCodecSubType 'I263'
#include "PerianResources.r"
#define kCodecSubType 'i263'
#include "PerianResources.r"

#define kCodecInfoResID kVP3CodecInfoResID
#define kCodecName "On2 VP3"
#define kCodecDescription "Decompresses video stored in On2 VP3 format."
#define kCodecSubType 'VP30'
#include "PerianResources.r"
#define kCodecSubType 'VP31'
#include "PerianResources.r"

#define kCodecInfoResID kHuffYUVCodecInfoResID
#define kCodecName "HuffYUV"
#define kCodecDescription "Decompresses video stored in HuffYUV format."
#define kCodecSubType 'HFYU'
#include "PerianResources.r"
#define kCodecSubType 'FFVH'
#include "PerianResources.r"

#define kCodecInfoResID kMPEG1CodecInfoResID
#define kCodecName "MPEG-1"
#define kCodecDescription "Decompresses video stored in MPEG-1 format."
#define kCodecSubType 'MPEG'
#include "PerianResources.r"
#define kCodecSubType 'mpg1'
#include "PerianResources.r"
#define kCodecSubType 'mp1v'
#include "PerianResources.r"

#define kCodecInfoResID kMPEG2CodecInfoResID
#define kCodecName "MPEG-2"
#define kCodecDescription "Decompresses video stored in MPEG-2 format."
#define kCodecSubType 'MPG2'
#include "PerianResources.r"
#define kCodecSubType 'mpg2'
#include "PerianResources.r"
#define kCodecSubType 'mp2v'
#include "PerianResources.r"

#define kCodecInfoResID kFRAPSCodecInfoResID
#define kCodecName "Fraps"
#define kCodecDescription "Decompresses video stored in Fraps format."
#define kCodecSubType 'FPS1'
#include "PerianResources.r"

#define kCodecInfoResID kSnowCodecInfoResID
#define kCodecName "Snow"
#define kCodecDescription "Decompresses video stored in Snow format."
#define kCodecSubType 'SNOW'

#define kCodecInfoResID kNuvCodecInfoResID
#define kCodecName "NuppelVideo"
#define kCodecDescription "Decompresses video stored in NuppelVideo format."
#define kCodecSubType 'RJPG'
#include "PerianResources.r"
#define kCodecSubType 'NUV1'
#include "PerianResources.r"

#define kCodecInfoResID kIndeo2CodecInfoResID
#define kCodecName "Indeo 2"
#define kCodecDescription "Decompresses video stored in Intel's Indeo 2 format."
#define kCodecSubType 'RT21'
#include "PerianResources.r"

#define kCodecInfoResID kIndeo3CodecInfoResID
#define kCodecName "Indeo 3"
#define kCodecDescription "Decompresses video stored in Intel's Indeo 3 format."
#define kCodecSubType 'IV32'
#include "PerianResources.r"

#define kCodecInfoResID kTSCCCodecInfoResID
#define kCodecName "Techsmith Screen Capture"
#define kCodecDescription "Decompresses video stored in Techsmith Screen Capture format."
#define kCodecSubType 'tscc'
#include "PerianResources.r"

#define kCodecInfoResID kZMBVCodecInfoResID
#define kCodecName "DosBox Capture"
#define kCodecDescription "Decompresses video stored in DosBox Capture format."
#define kCodecSubType 'ZMBV'
#include "PerianResources.r"

#define kCodecInfoResID kVP6ACodecInfoResID
#define kCodecName "On2 VP6A"
#define kCodecDescription "Decompresses video stored in On2 VP6A format."
#define kCodecSubType 'VP6A'
#include "PerianResources.r"

//---------------------------------------------------------------------------
// Code Entry Point for Mach-O
//---------------------------------------------------------------------------

resource 'dlle' (256) {
        "FFusionCodecComponentDispatch"
};

