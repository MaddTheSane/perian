/*
 * CompressCodecUtils.c
 * Created by Graham Booker on 8/14/10.
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

#include "CompressCodecUtils.h"
#include <QuickTime/QuickTime.h>
#include "CodecIDs.h"

OSType compressStreamFourCC(OSType original)
{
	switch (original) {
		case kH264CodecType:
			return kCompressedAVC1;
		case kMPEG4VisualCodecType:
			return kCompressedMP4V;
		case kAudioFormatMPEGLayer3:
			return kCompressedMP3;
		case kAudioFormatAC3:
			return kCompressedAC3;
		case kAudioFormatDTS:
			return kCompressedDTS;
		default:
			break;
	}
	
	return 0;
}

OSType originalStreamFourCC(OSType compress)
{
	switch (compress) {
		case kCompressedAVC1:
			return kH264CodecType;
		case kCompressedMP4V:
			return kMPEG4VisualCodecType;
		case kCompressedMP3:
			return kAudioFormatMPEGLayer3;
		case kCompressedAC3:
			return kAudioFormatAC3;
		case kCompressedDTS:
			return kAudioFormatDTS;
		default:
			break;
	}
	
	return 0;
}