/*****************************************************************************
*
*  Avi Import Component QuickTime Component Interface
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

#include <libavformat/riff.h>

#include "ff_MovieImportVersion.h"
#include "ff_private.h"
#include "Codecprintf.h"
#include "SubImport.h"
#include "CommonUtils.h"
#include "FFmpegUtils.h"

/* This one is a little big in ffmpeg and private anyway */
#define PROBE_BUF_SIZE 64

#include <CoreServices/CoreServices.h>
#include <QuickTime/QuickTime.h>
#include <pthread.h>

#define MOVIEIMPORT_BASENAME()		FFAvi_MovieImport
#define MOVIEIMPORT_GLOBALS()		ff_global_ptr storage

#define CALLCOMPONENT_BASENAME()	MOVIEIMPORT_BASENAME()
#define CALLCOMPONENT_GLOBALS()		MOVIEIMPORT_GLOBALS()

#define COMPONENT_DISPATCH_FILE		"ff_MovieImportDispatch.h"
#define COMPONENT_UPP_SELECT_ROOT()	MovieImport

#include <CoreServices/Components.k.h>
#include <QuickTime/QuickTimeComponents.k.h>
#include <QuickTime/ComponentDispatchHelper.c>

#pragma mark -

/************************************
** Base Component Manager Routines **
*************************************/

ComponentResult FFAvi_MovieImportOpen(ff_global_ptr storage, ComponentInstance self)
{
	ComponentResult result = noErr;
    ComponentDescription descout;
	
    GetComponentInfo((Component)self, &descout, 0, 0, 0);
	
	storage = malloc(sizeof(ff_global_context));
	if(!storage) goto bail;
	
	memset(storage, 0, sizeof(ff_global_context));
	storage->ci = self;
	
	SetComponentInstanceStorage(storage->ci, (Handle)storage);
	
	storage->componentType = descout.componentSubType;
	storage->movieLoadState = kMovieLoadStateLoading;
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
	
	if(storage->stream_map)
		av_free(storage->stream_map);
	
	if(storage->format_context)
		av_close_input_file(storage->format_context);
	
	int i;
	for(i=0; i<16; i++)
	{
		if(storage->firstFrames[i].data != NULL)
			av_free_packet(storage->firstFrames + i);
	}
	
	if(storage)
		free(storage);
	
	return noErr;
} /* FFAvi_MovieImportClose() */

ComponentResult FFAvi_MovieImportVersion(ff_global_ptr storage)
{
	return kFFAviComponentVersion;
} /* FFAvi_MovieImportVersion() */

#pragma mark -

/********************************
** Configuration Stuff is here **
*********************************/

ComponentResult FFAvi_MovieImportGetMIMETypeList(ff_global_ptr storage, QTAtomContainer *mimeInfo)
{
	ComponentResult err = noErr;
	switch (storage->componentType) {
		case 'VfW ':
		case 'VFW ':
		case 'AVI ':
			err = GetComponentResource((Component)storage->ci, 'mime', kAVIthngResID, (Handle*)mimeInfo);
			break;
		case 'FLV ':
			err = GetComponentResource((Component)storage->ci, 'mime', kFLVthngResID, (Handle*)mimeInfo);
			break;
		case 'TTA ':
			err = GetComponentResource((Component)storage->ci, 'mime', kTTAthngResID, (Handle*)mimeInfo);
			break;
		case 'NUV ':
			err = GetComponentResource((Component)storage->ci, 'mime', kNUVthngResID, (Handle*)mimeInfo);
			break;
		default:
			err = GetComponentResource((Component)storage->ci, 'mime', kAVIthngResID, (Handle*)mimeInfo);
			break;
	}
	return err;
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

// this function is a small avi parser to get the video track's fourcc as
// fast as possible, so we can decide whether we can handle the necessary 
// image description extensions for the format in ValidateDataRef() quickly
OSType get_avi_strf_fourcc(ByteIOContext *pb)
{
	OSType tag, subtag;
	unsigned int size;
	
	if (get_be32(pb) != 'RIFF')
		return 0;
	
	// file size
	get_le32(pb);
	
	if (get_be32(pb) != 'AVI ')
		return 0;
	
	while (!url_feof(pb)) {
		tag = get_be32(pb);
		size = get_le32(pb);
		
		if (tag == 'LIST') {
			subtag = get_be32(pb);
			
			// only lists we care about: hdrl & strl, so skip the rest
			if (subtag != 'hdrl' && subtag != 'strl')
				url_fskip(pb, size - 4 + (size & 1));
			
		} else if (tag == 'strf') {
			// 16-byte offset to the fourcc
			url_fskip(pb, 16);
			return get_be32(pb);
		} else if (tag == 'strh'){
			// 4-byte offset to the fourcc
			OSType tag1 = get_be32(pb);
			if(tag1 == 'iavs' || tag1 == 'ivas')
				return get_be32(pb);
			else
				url_fskip(pb, size + (size & 1) - 4);
		} else
			url_fskip(pb, size + (size & 1));
	}
	return 0;
}

ComponentResult FFAvi_MovieImportValidateDataRef(ff_global_ptr storage, Handle dataRef, OSType dataRefType, UInt8 *valid)
{
	ComponentResult result = noErr;
	DataHandler dataHandler = NULL;
	uint8_t buf[PROBE_BUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE] = {0};
	AVProbeData pd;
	ByteIOContext *byteContext;

	/* default */
	*valid = 0;
	
	/* Get a data handle and read a probe of data */
	result = OpenADataHandler(dataRef, dataRefType, NULL, 0, NULL, kDataHCanRead, &dataHandler);
	if(result || !dataHandler) goto bail;
	
	pd.buf = buf;
	pd.buf_size = PROBE_BUF_SIZE;
	
	result = DataHScheduleData(dataHandler, (Ptr)pd.buf, 0, PROBE_BUF_SIZE, 0, NULL, NULL);
	require_noerr(result,bail);
	
	FFInitFFmpeg();
	storage->format = av_probe_input_format(&pd, 1);
	if(storage->format != NULL) {
		*valid = 255; /* This means we can read the data */
		
		/* we don't do MJPEG's Huffman tables right yet, and DV video seems to have 
		an aspect ratio coded in the bitstream that ffmpeg doesn't read, so let Apple's 
		AVI importer handle AVIs with these two video types */
		if (IS_AVI(storage->componentType)) {
			/* Prepare the iocontext structure */
			result = url_open_dataref(&byteContext, dataRef, dataRefType, NULL, NULL, NULL);
			require_noerr(result, bail);
			
			OSType fourcc = get_avi_strf_fourcc(byteContext);
			enum CodecID id = ff_codec_get_id(ff_codec_bmp_tags, BSWAP(fourcc));
			
			if (id == CODEC_ID_MJPEG || id == CODEC_ID_DVVIDEO || id == CODEC_ID_RAWVIDEO || id == CODEC_ID_MSVIDEO1 || id == CODEC_ID_MSRLE)
				*valid = 0;
			
			url_fclose(byteContext);
		}
	}
		
bail:
		if(dataHandler)
			CloseComponent(dataHandler);
	
	return result;
} /* FFAvi_MovieImportValidateDataRef() */


ComponentResult FFAvi_MovieImportFile(ff_global_ptr storage, const FSSpec *theFile, Movie theMovie, Track targetTrack,
									  Track *usedTrack, TimeValue atTime, TimeValue *addedDuration, long inFlags, long *outFlags)
{
	ComponentResult result;
	Handle dataRef = NULL;
	OSType dataRefType;
	FSRef theFileFSRef;
	
	*outFlags = 0;
	
	result = QTNewDataReferenceFromFSSpec(theFile, 0, &dataRef, &dataRefType);
	require_noerr(result,bail);
	
	result = MovieImportDataRef(storage->ci, dataRef, dataRefType, theMovie, targetTrack, usedTrack, atTime, addedDuration,
								inFlags, outFlags);
	require_noerr(result, bail);
	
	result = FSpMakeFSRef(theFile, &theFileFSRef);
	require_noerr(result, bail);
		
bail:
		if(dataRef)
			DisposeHandle(dataRef);
	
	return result;
} /* FFAvi_MovieImportFile() */

ComponentResult FFAvi_MovieImportDataRef(ff_global_ptr storage, Handle dataRef, OSType dataRefType, Movie theMovie, Track targetTrack,
										 Track *usedTrack, TimeValue atTime, TimeValue *addedDuration, long inFlags, long *outFlags)
{
	ComponentResult result = noErr;
	ByteIOContext *byteContext;
	AVFormatContext *ic = NULL;
	AVFormatParameters params;
	OSType mediaType;
	Media media;
	int count, hadIndex, j;
		
	/* make sure that in case of error, the flag movieImportResultComplete is not set */
	*outFlags = 0;
	
	/* probe the format first */
	UInt8 valid = 0;
	FFAvi_MovieImportValidateDataRef(storage, dataRef, dataRefType, &valid);
	if(valid != 255)
		goto bail;
			
	/* Prepare the iocontext structure */
	result = url_open_dataref(&byteContext, dataRef, dataRefType, &storage->dataHandler, &storage->dataHandlerSupportsWideOffsets, &storage->dataSize);
	storage->isStreamed = dataRefType == URLDataHandlerSubType;
	require_noerr(result, bail);
	
	/* Open the Format Context */
	memset(&params, 0, sizeof(params));
	result = av_open_input_stream(&ic, byteContext, "", storage->format, &params);
	require_noerr(result,bail);
	storage->format_context = ic;
	
	if (ic->nb_streams >= 16)
		goto bail;

	// AVIs without an index currently add a few entries to the index so it can
	// determine codec parameters.  Check for index existence here before it
	// reads any packets.
	hadIndex = 1;
	for (j = 0; j < ic->nb_streams; j++) {
		if (ic->streams[j]->nb_index_entries <= 1)
		{
			hadIndex = 0;
			break;
		}
	}
	
	/* Get the Stream Infos if not already read */
	result = av_find_stream_info(ic);
	
	// -1 means it couldn't understand at least one stream
	// which might just mean we don't have its video decoder enabled
	if(result < 0 && result != -1)
		goto bail;
	
	// we couldn't find any streams, bail with an error.
	if(ic->nb_streams == 0) {
		result = -1; //is there a more appropriate error code?
		goto bail;
	}
	
	//determine a header offset (needed by index-based import).
	result = determine_header_offset(storage);
	if(result < 0)
		goto bail;
	
	/* Initialize the Movie */
	storage->movie = theMovie;
	if(inFlags & movieImportMustUseTrack) {
		storage->map_count = 1;
		prepare_track(storage, targetTrack, dataRef, dataRefType);
	} else {
		storage->map_count = ic->nb_streams;
		result = prepare_movie(storage, theMovie, dataRef, dataRefType);
		if (result != 0)
			goto bail;
	}
	
	/* replace the SampleDescription if user called MovieImportSetSampleDescription() */
	if(storage->imgHdl) {
		for(j = 0; j < storage->map_count; j++) {
			NCStream ncstream = storage->stream_map[j];
			GetMediaHandlerDescription(ncstream.media, &mediaType, NULL, NULL);
			if(mediaType == VideoMediaType && ncstream.sampleHdl) {
				DisposeHandle((Handle)ncstream.sampleHdl);
				ncstream.sampleHdl = (SampleDescriptionHandle)storage->imgHdl;
			}
		}
	}
	if(storage->sndHdl) {
		for(j = 0; j < storage->map_count; j++) {
			NCStream ncstream = storage->stream_map[j];
			GetMediaHandlerDescription(ncstream.media, &mediaType, NULL, NULL);
			if(mediaType == SoundMediaType && ncstream.sampleHdl) {
				DisposeHandle((Handle)ncstream.sampleHdl);
				ncstream.sampleHdl = (SampleDescriptionHandle)storage->sndHdl;
			}
		}
	}
	
	count = 0; media = NULL;
	for(j = 0; j < storage->map_count; j++) {
		media = storage->stream_map[j].media;
		if(media)
			count++;
	}
	
	if(count > 1)
		*outFlags |= movieImportResultUsedMultipleTracks;
	
	/* The usedTrack parameter. Count the number of Tracks and set usedTrack if we operated
		* on a single track. Note that this requires the media to be set by track counting above*/
	if(usedTrack && count == 1 && media)
		*usedTrack = GetMediaTrack(media);
	
	*addedDuration = 0;
	
	//attempt to import using indexes.
	result = import_using_index(storage, &hadIndex, addedDuration);
	require_noerr(result, bail);
	
	if(hadIndex) {
		//file had an index and was imported; we are done.
		*outFlags |= movieImportResultComplete;
		
	} else if(inFlags & movieImportWithIdle) {
		if(addedDuration && ic->duration > 0) {
			TimeScale movieTimeScale = GetMovieTimeScale(theMovie);
			*addedDuration = movieTimeScale * ic->duration / AV_TIME_BASE;
			
			//create a placeholder track so that progress displays correctly.
			create_placeholder_track(storage->movie, &storage->placeholderTrack, *addedDuration, dataRef, dataRefType);
			
			//give the data handler a hint as to how fast we need the data.
			//suggest a speed that's faster than the bare minimum.
			//if there's an error, the data handler probably doesn't support
			//this, so we can just ignore.
			DataHPlaybackHints(storage->dataHandler, 0, 0, -1, (storage->dataSize * 1.15) / ((double)ic->duration / AV_TIME_BASE));
		}
			
		//import with idle. Decode a little bit of data now.
		import_with_idle(storage, inFlags, outFlags, 10, 300, true);
	} else {
		//QuickTime didn't request import with idle, so do it all now.
		import_with_idle(storage, inFlags, outFlags, 0, 0, true);			
	}
	
	LoadExternalSubtitlesFromFileDataRef(dataRef, dataRefType, theMovie);

bail:
	if(result == noErr)
		storage->movieLoadState = kMovieLoadStateLoaded;
	else
		storage->movieLoadState = kMovieLoadStateError;
		
	if (result == -1)
		result = invalidMovie; // a bit better error message
	
	return result;
} /* FFAvi_MovieImportDataRef */

ComponentResult FFAvi_MovieImportSetIdleManager(ff_global_ptr storage, IdleManager im) {
	storage->idleManager = im;
	return noErr;
}

ComponentResult FFAvi_MovieImportIdle(ff_global_ptr storage, long inFlags, long *outFlags) {
	ComponentResult err = noErr;
	TimeValue currentIdleTime = GetMovieTime(storage->movie, NULL);
	TimeScale movieTimeScale = GetMovieTimeScale(storage->movie);
	int addSamples = false;
	
	storage->idlesSinceLastAdd++;
	
	if ((currentIdleTime == storage->lastIdleTime && storage->idlesSinceLastAdd > 5) || 
		storage->loadedTime < currentIdleTime + 5*movieTimeScale)
	{
		storage->idlesSinceLastAdd = 0;
		addSamples = true;
	}
	
	err = import_with_idle(storage, inFlags | movieImportWithIdle, outFlags, 0, 1000, addSamples);
	
	storage->lastIdleTime = currentIdleTime;
	return err;
}

ComponentResult FFAvi_MovieImportGetLoadState(ff_global_ptr storage, long *importerLoadState) {
	*importerLoadState = storage->movieLoadState;
	return(noErr);
}

ComponentResult FFAvi_MovieImportGetMaxLoadedTime(ff_global_ptr storage, TimeValue *time) {
	*time = storage->loadedTime;
	return(noErr);
}