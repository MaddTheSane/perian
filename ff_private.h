/*****************************************************************************
*
*  Avi Import Component Private Header
*
*  Copyright(C) 2006 Christoph Naegeli <chn1@mac.com>
*
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*  
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*  
*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
	int64_t lastpts;
	SampleReference64Ptr sampleTable;
	SampleReference64Record lastSample;
	TimeValue duration;
};
typedef struct _NCStream NCStream;

// this used to live in ff_MovieImport.c, but it had to move here for idling support
struct _ff_global_context {
	Movie movie;
	ComponentInstance ci;
	OSType componentType;
	
	/* For feedback during import */
	MovieProgressUPP prog;
	long refcon;
	
	/* for overwriting the default sample descriptions */
	ImageDescriptionHandle imgHdl;
	SoundDescriptionHandle sndHdl;
	
	// things needed for idling support
	Track placeholderTrack;
	DataHandler dataHandler;
	Boolean dataHandlerSupportsWideOffsets;
	int64_t dataSize;
	int largestPacketSize;
	
	IdleManager idleManager;
	long movieLoadState;
	TimeValue loadedTime;
	
	//the "atTime" parameter to the initial import.
	TimeValue atTime;
	
	// libavcodec fun.
	AVInputFormat *format;
	AVFormatContext *format_context;
	NCStream *stream_map;
	int map_count;
	int64_t header_offset;
	
	AVPacket firstFrames[MAX_STREAMS];
};
typedef struct _ff_global_context ff_global_context;
typedef ff_global_context *ff_global_ptr;

/* Utilities */
ComponentResult check_system();

/* Library initialization */
void register_parsers();

/* Public interface of the DataRef interface */
OSStatus url_open_dataref(ByteIOContext *pb, Handle dataRef, OSType dataRefType, DataHandler *dataHandler, Boolean *wideSupport, int64_t *dataSize);

/* Import routines */
int prepare_track(ff_global_ptr storage, Track targetTrack, Handle dataRef, OSType dataRefType);
int prepare_movie(ff_global_ptr storage, Movie theMovie, Handle dataRef, OSType dataRefType);
void initialize_video_map(NCStream *map, Track targetTrack, Handle dataRef, OSType dataRefType, AVPacket *firstFrame);
void initialize_audio_map(NCStream *map, Track targetTrack, Handle dataRef, OSType dataRefType, AVPacket *firstFrame);

int determine_header_offset(ff_global_ptr storage);
int import_using_index(ff_global_ptr storage, int *hadIndex, TimeValue *addedDuration);
ComponentResult import_with_idle(ff_global_ptr storage, long inFlags, long *outFlags, int minFrames, int maxFrames);
ComponentResult create_placeholder_track(Movie movie, Track *placeholderTrack, TimeValue duration, Handle dataRef, OSType dataRefType);
void send_movie_changed_notification(Movie movie);

OSType map_video_codec_to_mov_tag(enum CodecID codec_id);
void map_avi_to_mov_tag(enum CodecID codec_id, AudioStreamBasicDescription *asbd);
uint8_t *create_cookie(AVCodecContext *codec, int *cookieSize, UInt32 formatID);
Handle create_strf_ext(AVCodecContext *codec);

uint8_t *write_int32(uint8_t *target, int32_t data);
uint8_t *write_int16(uint8_t *target, int16_t data);
uint8_t *write_data(uint8_t *target, uint8_t* data, int32_t data_size);

#define BSWAP(a) ( (((a)&0xff) << 24) | (((a)&0xff00) << 8) | (((a)&0xff0000) >> 8) | (((a) >> 24) & 0xff) )

#define IS_AVI(x) (x == 'AVI ' || x == 'VfW ' || x == 'VFW ')

#endif
