/*
 * FFmpegUtils.h
 * Created by Alexander Strange on 11/7/10.
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

/*
 * FFmpeg API interfaces and data structure conversion
 */

#ifndef __FFMPEGUTILS__
#define __FFMPEGUTILS__

#include <QuickTime/QuickTime.h>
#include <CoreAudio/CoreAudio.h>

__BEGIN_DECLS

#include "avcodec.h"

void FFAVCodecContextToASBD(AVCodecContext *avctx, AudioStreamBasicDescription *asbd);

void FFASBDToAVCodecContext(AudioStreamBasicDescription *asbd, AVCodecContext *avctx);

void FFInitFFmpeg();

enum CodecID FFFourCCToCodecID(OSType formatID);

OSType FFCodecIDToFourCC(enum CodecID codecID);

__END_DECLS

#endif