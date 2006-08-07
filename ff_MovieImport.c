/*****************************************************************************
*
*  Avi Import Component QuickTime Component Interface
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

#include "ff_MovieImportVersion.h"
#include "avformat.h"
#include "ff_private.h"
#include "allformats.h"
#include "Codecprintf.h"

/* This one is a little big in ffmpeg and private anyway */
#define PROBE_BUF_SIZE 64

#include <CoreServices/CoreServices.h>
#include <QuickTime/QuickTime.h>

#define MOVIEIMPORT_BASENAME()		FFAvi_MovieImport
#define MOVIEIMPORT_GLOBALS()		ff_global_ptr storage

#define CALLCOMPONENT_BASENAME()	MOVIEIMPORT_BASENAME()
#define CALLCOMPONENT_GLOBALS()		MOVIEIMPORT_GLOBALS()

#define COMPONENT_DISPATCH_FILE		"ff_MovieImportDispatch.h"
#define COMPONENT_UPP_SELECT_ROOT()	MovieImport

struct _ff_global_context {
	ComponentInstance ci;
	
	/* For feedback during import */
	MovieProgressUPP prog;
	long refcon;
	
	/* for overwriting the default sample descriptions */
	ImageDescriptionHandle imgHdl;
	SoundDescriptionHandle sndHdl;
	AVInputFormat		*format;
};
typedef struct _ff_global_context ff_global_context;
typedef ff_global_context *ff_global_ptr;

#include <CoreServices/Components.k.h>
#include <QuickTime/QuickTimeComponents.k.h>
#include <QuickTime/ComponentDispatchHelper.c>

#pragma mark -

void initLib()
{
	/* This one is used because Global variables are initialized ONE time
	* until the application quits. Thus, we have to make sure we're initialize
	* the libavformat only once or we get an endlos loop when registering the same
	* element twice!! */
	static Boolean inited = FALSE;
	
	/* Register the Parser of ffmpeg, needed because we do no proper setup of the libraries */
	if(!inited) {
		inited = TRUE;
		av_register_input_format(&avi_demuxer);
		register_parsers();
		av_log_set_callback(FFMpegCodecprintf);
	}
}

/************************************
** Base Component Manager Routines **
*************************************/

ComponentResult FFAvi_MovieImportOpen(ff_global_ptr storage, ComponentInstance self)
{
	ComponentResult result;
	
	/* Check for Mac OS 10.4 & QT 7 */
	result = check_system();
	require_noerr(result,bail);
	
	storage = malloc(sizeof(ff_global_context));
	if(!storage) goto bail;
	
	memset(storage, 0, sizeof(ff_global_context));
	storage->ci = self;
	
	SetComponentInstanceStorage(storage->ci, (Handle)storage);
	
	/* Set this component as default one */
	SetDefaultComponent((Component)self, defaultComponentAnyFlagsAnyManufacturer);
	
bail:
		return result;
} /* FFAvi_MovieImportOpen() */

ComponentResult FFAvi_MovieImportClose(ff_global_ptr storage, ComponentInstance self)
{
	/* Free all global storage */
	if(storage->imgHdl)
		DisposeHandle((Handle)storage->imgHdl);
	if(storage->sndHdl)
		DisposeHandle((Handle)storage->sndHdl);
	
	if(storage)
		free(storage);
	
	return noErr;
} /* FFAvi_MovieImportClose() */

ComponentResult FFAvi_MovieImportVersion(ff_global_ptr storage)
{
	return kFFAviComponentVersion;
} /* FFAvi_MovieImportVersion() */

ComponentResult FFAvi_MovieImportRegister(ff_global_ptr storage)
{
	SetDefaultComponent((Component)storage->ci, defaultComponentAnyFlagsAnyManufacturer);
	
	return noErr;
} /* FFAvi_MovieImportRegister() */

#pragma mark -

/********************************
** Configuration Stuff is here **
*********************************/

ComponentResult FFAvi_MovieImportGetMIMETypeList(ff_global_ptr storage, QTAtomContainer *mimeInfo)
{
	return GetComponentResource((Component)storage->ci, 'mime', 512, (Handle*)mimeInfo);
} /* FFAvi_MovieImportGetMIMETypeList() */

ComponentResult FFAvi_MovieImportSetProgressProc(ff_global_ptr storage, MovieProgressUPP prog, long refcon)
{
	storage->prog = prog;
	storage->refcon = refcon;
	
	if(!storage->prog)
		storage->refcon = 0;
	
	return noErr;
} /* FFAvi_MovieImportSetProgressProc() */

ComponentResult FFAvi_MovieImportSetSampleDescription(ff_global_ptr storage, SampleDescriptionHandle desc, OSType media)
{
	ComponentResult result = noErr;
	
	switch(media) {
		case VideoMediaType:
			if(storage->imgHdl) /* already one stored */
				DisposeHandle((Handle)storage->imgHdl);
			
			storage->imgHdl = (ImageDescriptionHandle)desc;
			if(storage->imgHdl)
				result = HandToHand((Handle*)&storage->imgHdl);
				break;
		case SoundMediaType:
			if(storage->sndHdl)
				DisposeHandle((Handle)storage->sndHdl);
			
			storage->sndHdl = (SoundDescriptionHandle)desc;
			if(storage->sndHdl)
				result = HandToHand((Handle*)&storage->sndHdl);
				break;
		default:
			break;
	}
	
	return result;
} /* FFAvi_MovieImportSetSampleDescription() */

ComponentResult FFAvi_MovieImportGetDestinationMediaType(ff_global_ptr storage, OSType *mediaType)
{
	*mediaType = VideoMediaType;
	return noErr;
} /* FFAvi_MovieImportGetDestinationMediaType() */

#pragma mark -

/****************************
** Import Stuff comes here **
*****************************/

ComponentResult FFAvi_MovieImportValidate(ff_global_ptr storage, const FSSpec *theFile, Handle theData, Boolean *valid)
{
	ComponentResult result = noErr;
	Handle dataRef = NULL;
	OSType dataRefType;
	
	*valid = FALSE;
	
	result = QTNewDataReferenceFromFSSpec(theFile, 0, &dataRef, &dataRefType);
	require_noerr(result,bail);
	
	result = MovieImportValidateDataRef(storage->ci, dataRef, dataRefType, (UInt8*)valid);
	
bail:
		if(dataRef)
			DisposeHandle(dataRef);
	
	return result;
} /* FFAvi_MovieImportValidate() */

ComponentResult FFAvi_MovieImportValidateDataRef(ff_global_ptr storage, Handle dataRef, OSType dataRefType, UInt8 *valid)
{
	ComponentResult result = noErr;
	DataHandler dataHandler = NULL;
	uint8_t buf[PROBE_BUF_SIZE];
	AVProbeData *pd = (AVProbeData *)malloc(sizeof(AVProbeData));
	int success;
	
	/* default */
	*valid = 0;
	
	/* Get a data handle and read a probe of data */
	result = OpenADataHandler(dataRef, dataRefType, NULL, 0, NULL, kDataHCanRead, &dataHandler);
	if(result || !dataHandler) goto bail;
	
	pd->buf = buf;
	pd->buf_size = PROBE_BUF_SIZE;
	
	result = DataHScheduleData(dataHandler, (Ptr)(pd->buf), 0, PROBE_BUF_SIZE, 0, NULL, NULL);
	require_noerr(result,bail);
	
	initLib();
	storage->format = av_probe_input_format(pd, 1);
	if(storage->format != NULL)
		*valid = 255; /* This means we can read the data */
bail:
		if(dataHandler)
			CloseComponent(dataHandler);
	free(pd);
	
	return result;
} /* FFAvi_MovieImportValidateDataRef() */


ComponentResult FFAvi_MovieImportFile(ff_global_ptr storage, const FSSpec *theFile, Movie theMovie, Track targetTrack,
									  Track *usedTrack, TimeValue atTime, TimeValue *addedDuration, long inFlags, long *outFlags)
{
	ComponentResult result;
	Handle dataRef = NULL;
	OSType dataRefType;
	
	*outFlags = 0;
	
	result = QTNewDataReferenceFromFSSpec(theFile, 0, &dataRef, &dataRefType);
	require_noerr(result,bail);
	
	result = MovieImportDataRef(storage->ci, dataRef, dataRefType, theMovie, targetTrack, usedTrack, atTime, addedDuration,
								inFlags, outFlags);
bail:
		if(dataRef)
			DisposeHandle(dataRef);
	
	return result;
} /* FFAvi_MovieImportFile() */

ComponentResult FFAvi_MovieImportDataRef(ff_global_ptr storage, Handle dataRef, OSType dataRefType, Movie theMovie, Track targetTrack,
										 Track *usedTrack, TimeValue atTime, TimeValue *addedDuration, long inFlags, long *outFlags)
{
	ComponentResult result;
	ByteIOContext byteContext;
	AVFormatContext *ic = NULL;
	AVFormatParameters params;
	int64_t dataOffset;
	AVPacket pkt;
	NCStream *map = NULL;
	int map_count,j,count;
	OSType mediaType;
	Track track;
	Media media;
	TimeRecord time;
	int i;
	
	/* make sure that in case of error, the flag movieImportResultComplete is not set */
	*outFlags = 0;
	
	/* probe the format first */
	UInt8 valid = 0;
	FFAvi_MovieImportValidateDataRef(storage, dataRef, dataRefType, &valid);
	if(valid != 255)
		goto bail;
	
	/* Prepare the iocontext structure */
	memset(&byteContext, 0, sizeof(byteContext));
	result = url_open_dataref(&byteContext, dataRef, dataRefType);
	require_noerr(result, bail);
	
	/* Open the Format Context */
	memset(&params, 0, sizeof(params));
	result = av_open_input_stream(&ic, &byteContext, "", storage->format, &params);
	require_noerr(result,bail);
	
	/* Get the Stream Infos if not already read */
	result = av_find_stream_info(ic);
	if(result < 0)
		goto bail;
	
	/* Seek backwards to get a manually read packet for file offset */
	if(ic->streams[0]->index_entries == NULL)
	{
		//Try to seek to the first frame; don't care if it fails
		av_seek_frame(ic, -1, 0, 0);
		dataOffset = 0;
	}
	else
	{
		result = av_seek_frame(ic, -1, 0, 0);
		if(result < 0) goto bail;
		
		ic->iformat->read_packet(ic, &pkt);
		/* read_packet will give the first decodable packet. However, that isn't necessarily
			the first entry in the index, so look for an entry with a matching size. */
		for (i = 0; i < ic->streams[pkt.stream_index]->nb_index_entries; i++) {
			if (pkt.size == ic->streams[pkt.stream_index]->index_entries[i].size) {
				dataOffset = pkt.pos - ic->streams[pkt.stream_index]->index_entries[i].pos;
				break;
			}
		}
		av_free_packet(&pkt);
	}
	
	/* Initialize the Movie */
	if(inFlags & movieImportMustUseTrack) {
		map_count = 1;
		prepare_track(ic, &map, targetTrack, dataRef, dataRefType);
	} else {
		map_count = ic->nb_streams;
		prepare_movie(ic, &map, theMovie, dataRef, dataRefType);
	}
	
	/* replace the SampleDescription if user called MovieImportSetSampleDescription() */
	if(storage->imgHdl) {
		for(j = 0; j < map_count; j++) {
			GetMediaHandlerDescription(map[j].media, &mediaType, NULL, NULL);
			if(mediaType == VideoMediaType && map[j].sampleHdl) {
				DisposeHandle((Handle)map[j].sampleHdl);
				map[j].sampleHdl = (SampleDescriptionHandle)storage->imgHdl;
			}
		}
	}
	if(storage->sndHdl) {
		for(j = 0; j < map_count; j++) {
			GetMediaHandlerDescription(map[j].media, &mediaType, NULL, NULL);
			if(mediaType == SoundMediaType && map[j].sampleHdl) {
				DisposeHandle((Handle)map[j].sampleHdl);
				map[j].sampleHdl = (SampleDescriptionHandle)storage->sndHdl;
			}
		}
	}
	
	/* Import the Data*/
	/* FIXME: Implement the progress upp */
	import_avi(ic, map, dataOffset);
	
	/* Insert the Medias into the Tracks */
	result = noErr;
	for(j = 0; j < map_count && result == noErr; j++) {
		media = map[j].media;
		if(media) {
			/* we could handle this stream.
			* convert the atTime parameter to track scale.
			* FIXME: check if that's correct */			
			time.value.hi = 0;
			time.value.lo = atTime;
			time.scale = GetMovieTimeScale(theMovie);
			time.base = NULL;
			ConvertTimeScale(&time, GetMediaTimeScale(media));
			
			track = GetMediaTrack(media);
			result = InsertMediaIntoTrack(track, time.value.lo, 0, GetMediaDuration(media), fixed1);
		}
	}
	require_noerr(result,bail);
	
	/* Set return values of the function */
	
	count = 0; media = NULL;
	for(j = 0; j < map_count; j++) {
		media = map[j].media;
		if(media)
			count++;
	}
	
	/* The usedTrack parameter. Count the number of Tracks and set usedTrack if we operated
		* on a single track. Note that this requires the media to be set by track counting above*/
	if(usedTrack && count == 1 && media)
		*usedTrack = GetMediaTrack(media);
	
	/* the addedDuration parameter */
	if(addedDuration) {
		*addedDuration = 0;
		for(j = 0; j < map_count; j++) {
			media = map[j].media;
			if(media) {
				time.value.hi = 0;
				time.value.lo = GetMediaDuration(media);
				time.scale = GetMediaTimeScale(media);
				time.base = NULL;
				ConvertTimeScale(&time, GetMovieTimeScale(theMovie));
				
				/* if that's longer than before, replace */
				if(time.value.lo > *addedDuration)
					*addedDuration = time.value.lo;
			}
		}
	}
	
	/* now set the outflags, set to zero at the beginning of the function */
	if(outFlags) {
		if(count > 1)
			*outFlags |= movieImportResultUsedMultipleTracks;
		
		/* set the finished flag */
		*outFlags |= movieImportResultComplete;
	}
	
bail:
		/* Free all the data structures used */
		if(ic)
			av_close_input_file(ic);
	if(map)
		av_free(map);
	
	return result;
} /* FFAvi_MovieImportDataRef */
