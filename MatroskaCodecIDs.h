/*
 *  MatroskaCodecIDs.h
 *
 *    MatroskaCodecIDs.h - Codec and language IDs for conversion between Matroska and QuickTime
 *
 *
 *  Copyright (c) 2006  David Conrad
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; 
 *  version 2.1 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __MATROSKACODECIDS_H__
#define __MATROSKACODECIDS_H__

#include <QuickTime/QuickTime.h>
#include <AudioToolbox/AudioToolbox.h>

#ifdef __cplusplus
#include <matroska/KaxTracks.h>
#include <matroska/KaxTrackEntryData.h>
#include "CodecIDs.h"

using namespace libmatroska;


// these CodecIDs need special handling since they correspond to many fourccs
#define MKV_V_MS "V_MS/VFW/FOURCC"
#define MKV_A_MS "A_MS/ACM"
#define MKV_V_QT "V_QUICKTIME"

// these codecs have their profile as a part of the CodecID
#define MKV_A_PCM_BIG "A_PCM/INT/BIG"
#define MKV_A_PCM_LIT "A_PCM/INT/LIT"
#define MKV_A_PCM_FLOAT "A_PCM/FLOAT/IEEE"


typedef enum {
	kToKaxTrackEntry,
	kToSampleDescription
} DescExtDirection;

ComponentResult MkvFinishSampleDescription(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc, DescExtDirection dir);
ComponentResult MkvFinishASBD(KaxTrackEntry *tr_entry, AudioStreamBasicDescription *asbd);
FourCharCode GetFourCC(KaxTrackEntry *tr_entry);

extern "C"{
#endif

AudioChannelLayout GetDefaultChannelLayout(AudioStreamBasicDescription *asbd);

#ifdef __cplusplus
}
#endif
#endif
