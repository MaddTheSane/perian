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
//Global Header
//---------------------------------------------------------------------------

#define kFFusionCodecVersion		(0x00030005)
#define kFFusionCodecManufacturer	'Peri'

enum {
	kDivX1CodecInfoResID		= 256,
	kDivX2CodecInfoResID,
	kDivX3CodecInfoResID,
	kDivX4CodecInfoResID,
	kDivX5CodecInfoResID,
	k3ivxCodecInfoResID,
	kXVIDCodecInfoResID,
	kMPEG4CodecInfoResID,
	kH264CodecInfoResID,
	kFLV1CodecInfoResID,
	kFlashSVCodecInfoResID,
	kVP6CodecInfoResID,
	kI263CodecInfoResID,
	kVP3CodecInfoResID,
	kHuffYUVCodecInfoResID,
	kMPEG1CodecInfoResID,
	kMPEG2CodecInfoResID,
	kFRAPSCodecInfoResID,
	kSnowCodecInfoResID,
	kNuvCodecInfoResID,
	kIndeo2CodecInfoResID,
	kIndeo3CodecInfoResID,
	kTSCCCodecInfoResID,
	kZMBVCodecInfoResID,
	kVP6ACodecInfoResID,
	kPERIANCODECLASTID,
};

#define kFFusionCodecCount kPERIANCODECLASTID - kDivX1CodecInfoResID

#define kOptionKeyModifier		0x04