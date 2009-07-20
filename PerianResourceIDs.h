/*
 * PerianResourceIDs.h
 * Created by Graham Booker on 7/17/09.
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

#import "CodecIDs.h"

#define kPerianManufacturer		'Peri'

#define kFFusionCodecManufacturer	kPerianManufacturer
#define kTextCodecManufacturer		kPerianManufacturer
#define kVobSubCodecManufacturer	kPerianManufacturer
#define kFFissionCodecManufacturer	kPerianManufacturer

#define kFFusionCodecVersion		(0x00030005)
#define kTextSubCodecVersion		(0x00020003)
#define kVobSubCodecVersion			(0x00020003)
#define kFFissionCodecVersion		kFFusionCodecVersion

enum {
	kWMA1MSCodecResourceID		= 128,
	kWMA2MSCodecResourceID,
	kFlashADPCMCodecResourceID,
	kXiphVorbisCodecResourceID,
	kMPEG1L1CodecResourceID,
	kMPEG1L12CodecResourceID,
	kMPEG1L2CodecResourceID,
	kTrueAudioCodecResourceID,
	kDTSCodecResourceID,
	kNellymoserCodecResourceID,
	kSSASubCodecResourceID		= 256,
	kTextSubCodecResourceID,
	kVobSubCodecResourceID,
	kDivX1CodecInfoResID,
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
};

