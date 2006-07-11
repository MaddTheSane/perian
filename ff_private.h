/*****************************************************************************
 *
 *  Avi Import Component Private Header
 *
 *  Copyright(C) 2006 Christoph Naegeli <chn1@mac.com>
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 ****************************************************************************/

#ifndef __FF_PRIVATE__
#define __FF_PRIVATE__

#include "avformat.h"
#include <QuickTime/Movies.h>

/* Data structres needed for import */
struct _NCStream {
	int index;
	AVStream *str;
	SampleDescriptionHandle sampleHdl;
	AudioStreamBasicDescription asbd;
	Media media;
	UInt32 vbr;
	AVRational base;
};
typedef struct _NCStream NCStream;

/* Utilities */
ComponentResult check_system();

/* Library initialization */
void register_parsers();

/* Public interface of the DataRef interface */
OSStatus url_open_dataref(ByteIOContext *pb, Handle dataRef, OSType dataRefType);

/* Import routines */
int prepare_track(AVFormatContext *ic, NCStream **out_map, Track targetTrack, Handle dataRef, OSType dataRefType);
int prepare_movie(AVFormatContext *ic, NCStream **out_map, Movie theMovie, Handle dataRef, OSType dataRefType);
void initialize_video_map(NCStream *map, Track targetTrack, Handle dataRef, OSType dataRefType);
void initialize_audio_map(NCStream *map, Track targetTrack, Handle dataRef, OSType dataRefType);

void import_avi(AVFormatContext *ic, NCStream *map, int64_t aviheader_offset);

void map_avi_to_mov_tag(enum CodecID codec_id, AudioStreamBasicDescription *asbd);
uint8_t *create_cookie(AVCodecContext *codec, int *cookieSize, UInt32 formatID);

uint8_t *write_int32(uint8_t *target, int32_t data);
uint8_t *write_int16(uint8_t *target, int16_t data);
uint8_t *write_data(uint8_t *target, uint8_t* data, int32_t data_size);


#endif
