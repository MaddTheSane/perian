/*****************************************************************************
*
*  Avi Import Component Private Functions
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

#include "ff_private.h"
#include "avcodec.h"
#include "Codecprintf.h"

#include <CoreServices/CoreServices.h>
#include <AudioToolbox/AudioToolbox.h>

/* This routine checks if the system requirements are fullfilled */
ComponentResult check_system()
{
	ComponentResult result;
	long systemVersion;
	
	result = Gestalt(gestaltSystemVersion, &systemVersion);
	require_noerr(result,bail);
	
	/* Make sure we have at least 10.4 installed...*/
	if(systemVersion < 0x00001040)
		result = -1;
	
bail:
		return result;
} /* check_system() */

/* This routine does register the ffmpeg parsers which would normally
 * be registred through the normal initialization process */
void register_parsers()
{
	/* Do we need more parsers here? */
//    av_register_codec_parser(&mpegaudio_parser);
//	av_register_codec_parser(&ac3_parser);
} /* register_parsers() */


/* This function prepares the target Track to receivve the movie data,
 * it is called if QT has asked an import operation which should just
 * load this track. After success, *outmap points to a valid stream maping
 * Return values:
 *	  0: ok
 *	< 0: couldn't find a matching track
 */
int prepare_track(AVFormatContext *ic, NCStream **out_map, Track targetTrack, Handle dataRef, OSType dataRefType)
{
	int j;
	AVStream *st;
	AVStream *outstr = NULL;
	Media media;
	NCStream *map = NULL;
	
	/* If the track has already a media, return an err */
	media = GetTrackMedia(targetTrack);
	if(media) goto err;
	
	/* Search the AVFormatContext for a video stream */
	for(j = 0; j < ic->nb_streams && !outstr; j++) {
		st = ic->streams[j];
		if(st->codec->codec_type == CODEC_TYPE_VIDEO)
			outstr = st;
	}
	/* Search the AVFormatContext for an audio stream (no video stream exists) */
	for(j = 0; j < ic->nb_streams && !outstr; j++) {
		st = ic->streams[j];
		if(st->codec->codec_type == CODEC_TYPE_AUDIO)
			outstr = st;
	}
	/* Still no stream, then err */
	if(!outstr) goto err;
	
	/* prepare the stream map & initialize*/
	map = av_mallocz(sizeof(NCStream));
	map->index = st->index;
	map->str = outstr;
	
	if(st->codec->codec_type == CODEC_TYPE_VIDEO)
		initialize_video_map(map, targetTrack, dataRef, dataRefType);
	else if(st->codec->codec_type == CODEC_TYPE_AUDIO)
		initialize_audio_map(map, targetTrack, dataRef, dataRefType);
	
	/* return the map */
	*out_map = map;
	
	return 0;
err:
		if(map)
			av_free(map);
	return -1;
} /* prepare_track() */

/* Initializes the map & targetTrack to receive video data */
void initialize_video_map(NCStream *map, Track targetTrack, Handle dataRef, OSType dataRefType)
{
	Media media;
	ImageDescriptionHandle imgHdl;
	Handle imgDescExt = NULL;
	AVCodecContext *codec;
	double num,den;
	
	codec = map->str->codec;
	
	map->base = map->str->time_base;
	if(map->base.den > 100000) {
		/* if that's the case, then we probably ran out of timevalues!
		* a timescale of 100000 allows a movie duration of 5-6 hours
		* so I think this will be enough */
		den = map->base.den;
		num = map->base.num;
		
		/* we use the following approach ##.### frames per second.
			* afterwards rounding */
		den = den / num * 1000.0;
		map->base.num = 1000;
		map->base.den = round(den);
	}
	
	media = NewTrackMedia(targetTrack, VideoMediaType, map->base.den, dataRef, dataRefType);
	map->media = media;
	
	/* Create the Image Description Handle */
	imgHdl = (ImageDescriptionHandle)NewHandleClear(sizeof(ImageDescription));
	(*imgHdl)->idSize = sizeof(ImageDescription);
	
	if (codec->codec_tag)
		(*imgHdl)->cType = BSWAP(codec->codec_tag);
	else
		// need to lookup the fourcc from the codec_id
		(*imgHdl)->cType = map_video_codec_to_mov_tag(codec->codec_id);
	Codecprintf(NULL, "fourcc: %c%c%c%c\n",0xff & (*imgHdl)->cType,0xff & (*imgHdl)->cType>>8,0xff & (*imgHdl)->cType>>16,0xff & (*imgHdl)->cType>>24);
	
	(*imgHdl)->temporalQuality = codecMaxQuality;
	(*imgHdl)->spatialQuality = codecMaxQuality;
	(*imgHdl)->width = codec->width;
	(*imgHdl)->height = codec->height;
	(*imgHdl)->hRes = 72 << 16;
	(*imgHdl)->vRes = 72 << 16;
	(*imgHdl)->depth = codec->bits_per_sample;
	(*imgHdl)->clutID = -1; // no color lookup table...
	
	/* Create the strf image description extension (see AVI's BITMAPINFOHEADER) */
	imgDescExt = create_strf_ext(codec);
	if (imgDescExt) {
		AddImageDescriptionExtension(imgHdl, imgDescExt, 'strf');
		DisposeHandle(imgDescExt);
	}
	
	map->sampleHdl = (SampleDescriptionHandle)imgHdl;
	
} /* initialize_video_map() */

/* Initializes the map & targetTrack to receive audio data */
void initialize_audio_map(NCStream *map, Track targetTrack, Handle dataRef, OSType dataRefType)
{
	Media media;
	SoundDescriptionHandle sndHdl;
	AudioStreamBasicDescription asbd;
	AVCodecContext *codec;
	UInt32 ioSize;
	OSStatus err;
	
	uint8_t *cookie = NULL;
	int cookieSize = 0;
	
	codec = map->str->codec;
	map->base = map->str->time_base;
	
	media = NewTrackMedia(targetTrack, SoundMediaType, codec->sample_rate, dataRef, dataRefType);
	
	map->media = media;
	
	memset(&asbd,0,sizeof(asbd));
	map_avi_to_mov_tag(codec->codec_id, &asbd);
	if(asbd.mFormatID == 0) /* no know codec, use the ms tag */
		asbd.mFormatID = 'ms\0\0' + codec->codec_tag; /* the number is stored in the last byte => big endian */
	
	/* Ask the AudioToolbox about vbr of the codec */
	ioSize = sizeof(UInt32);
	AudioFormatGetProperty(kAudioFormatProperty_FormatIsVBR, sizeof(AudioStreamBasicDescription), &asbd, &ioSize, &map->vbr);
	
	/* ask the toolbox about more information */
	ioSize = sizeof(AudioStreamBasicDescription);
	AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, NULL, &ioSize, &asbd);
	
	/* Set some fields of the AudioStreamBasicDescription. Then ask the AudioToolbox
		* to fill as much as possible before creating the SoundDescriptionHandle */
	asbd.mSampleRate = codec->sample_rate;
	asbd.mChannelsPerFrame = codec->channels;
	if(!map->vbr) /* This works for all the tested codecs. but is there any better way? */
		asbd.mBytesPerPacket = codec->block_align; /* this is tested for alaw/mulaw/msadpcm */
	asbd.mFramesPerPacket = codec->frame_size; /* works for mp3, all other codecs this is 0 anyway */
	asbd.mBitsPerChannel = codec->bits_per_sample;
	
	// this probably isn't quite right; FLV doesn't set frame_size or block_align, 
	// but we need > 0 frames per packet or Apple's mp3 decoder won't work
	if (asbd.mBytesPerPacket == 0 && asbd.mFramesPerPacket == 0)
		asbd.mFramesPerPacket = 1;
	
	/* If we have vbr audio, the media scale most likely has to be set to the time_base denumerator */
	if(map->vbr) {
		/* if we have mFramesPerPacket, set mBytesPerPacket to 0 as this can cause
		* errors if set incorrectly. But in vbr, we just need the mFramesPerPacket
		* value */
		if(asbd.mFramesPerPacket)
			asbd.mBytesPerPacket = 0;
		SetMediaTimeScale(media, map->str->time_base.den);
	}
	
	/* FIXME. in the MSADPCM codec, we get a wrong mFramesPerPacket entry because
		* of the difference in the sample_rate and the time_base denumerator. So we
		* recalculate here the mFramesPerPacket entry */
	if(asbd.mBytesPerPacket) {
		/* For calculation, lets assume a packet duration of 1, use ioSize as tmp storage */
		ioSize = map->str->time_base.num * codec->sample_rate / map->str->time_base.den;
		/* downscale to correct bytes_per_packet */
		asbd.mFramesPerPacket = ioSize * asbd.mBytesPerPacket / codec->block_align;
	}
	
	/* here use a version1, because version2 will fail! (no idea why)
		* and as we are using version1, we may not have more than 2 channels.
		* perhaps we should go to version2 some day. */
	if(asbd.mChannelsPerFrame > 2)
		asbd.mChannelsPerFrame = 2;
	err = QTSoundDescriptionCreate(&asbd, NULL, 0, NULL, 0, kQTSoundDescriptionKind_Movie_LowestPossibleVersion, &sndHdl);
	if(err) fprintf(stderr, "AVI IMPORTER: Error creating the sound description\n");
	
	/* Create the magic cookie */
	cookie = create_cookie(codec, &cookieSize, asbd.mFormatID);
	if(cookie) {
		err = QTSoundDescriptionSetProperty(sndHdl, kQTPropertyClass_SoundDescription, kQTSoundDescriptionPropertyID_MagicCookie,
											cookieSize, cookie);
		if(err) fprintf(stderr, "AVI IMPORTER: Error appending the magic cookie to the sound description\n");
		av_free(cookie);
	}
	
	map->sampleHdl = (SampleDescriptionHandle)sndHdl;
	map->asbd = asbd;
} /* initialize_audio_map() */

OSType map_video_codec_to_mov_tag(enum CodecID codec_id)
{
	switch(codec_id) {
		case CODEC_ID_FLV1:
			return 'FLV1';
		case CODEC_ID_VP6F:
			return 'VP6F';
		case CODEC_ID_FLASHSV:
			return 'FSV1';
	}
	return 0;
}

/* maps the codec_id tag of libavformat to a constant the AudioToolbox can work with */
void map_avi_to_mov_tag(enum CodecID codec_id, AudioStreamBasicDescription *asbd)
{
	switch(codec_id) {
		case CODEC_ID_MP2:
			asbd->mFormatID = kAudioFormatMPEGLayer2;
			break;
		case CODEC_ID_MP3:
			asbd->mFormatID = kAudioFormatMPEGLayer3;
			break;
			/* currently we use ms four_char_code for ac3 */
			/* case CODEC_ID_AC3:
			asbd->mFormatID = kAudioFormatAC3;
			break; */
		case CODEC_ID_PCM_S16LE:
			asbd->mFormatID = kAudioFormatLinearPCM;
			asbd->mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;
			break;
		case CODEC_ID_PCM_U8:
			asbd->mFormatID = kAudioFormatLinearPCM;
			asbd->mFormatFlags = kLinearPCMFormatFlagIsBigEndian;
			break;
		case CODEC_ID_PCM_ALAW:
			asbd->mFormatID = kAudioFormatALaw;
			break;
		case CODEC_ID_PCM_MULAW:
			asbd->mFormatID = kAudioFormatULaw;
			break;
		case CODEC_ID_ADPCM_MS:
			asbd->mFormatID = kMicrosoftADPCMFormat;
			break;
		case CODEC_ID_AAC:
		case CODEC_ID_MPEG4AAC:
			asbd->mFormatID = kAudioFormatMPEG4AAC;
			break;
		case CODEC_ID_VORBIS:
			asbd->mFormatID = 'OggV';
			break;
		default:
			break;
	}
} /* map_avi_to_mov_tag() */

/* This function creates a magic cookie basec on the codec parameter and formatID
 * Return value: a pointer to a magic cookie which has to be av_free()'d
 * in cookieSize, the size of the magic cookie is returned to the caller */
uint8_t *create_cookie(AVCodecContext *codec, int *cookieSize, UInt32 formatID)
{
	uint8_t *result = NULL;
	uint8_t *ptr;
	AudioFormatAtom formatAtom;
	AudioTerminatorAtom termAtom;
	long waveSize;
	uint8_t *waveAtom = NULL;
	int size = 0;
	
	/* Do we need an endia atom, too? */
	
	/* initialize the user Atom
		* 8 bytes			for the atom size & atom type
		* 18 bytes			for the already extracted part, see wav.c in the ffmpeg project
		* extradata_size	for the data still stored in the AVCodecContext structure */
	waveSize = 18 + codec->extradata_size + 8;
	waveAtom = av_malloc(waveSize);
	
	/* now construct the wave atom */
	/* QT Atoms are big endian, I think but the WAVE data should be little endian */
	ptr = write_int32(waveAtom, EndianS32_NtoB(waveSize));
	ptr = write_int32(ptr, formatID);
	ptr = write_int16(ptr, EndianS16_NtoL(codec->codec_tag));
	ptr = write_int16(ptr, EndianS16_NtoL(codec->channels));
	ptr = write_int32(ptr, EndianS32_NtoL(codec->sample_rate));
	ptr = write_int32(ptr, EndianS32_NtoL(codec->bit_rate / 8));
	ptr = write_int16(ptr, EndianS16_NtoL(codec->block_align));
	ptr = write_int16(ptr, EndianS16_NtoL(codec->bits_per_sample));
	ptr = write_int16(ptr, EndianS16_NtoL(codec->extradata_size));
	/* now the remaining stuff */
	ptr = write_data(ptr, codec->extradata, codec->extradata_size);
	
	/* Calculate the size of the cookie */
	size  = sizeof(formatAtom) + sizeof(termAtom) + waveSize;
	
	/* Audio Format Atom */
	formatAtom.size = sizeof(AudioFormatAtom);
	formatAtom.atomType = kAudioFormatAtomType;
	formatAtom.format = formatID;
	
	/* Terminator Atom */
	termAtom.atomType = kAudioTerminatorAtomType;
	termAtom.size = sizeof(AudioTerminatorAtom);
	
	result = av_malloc(size);
	
	/* use ptr to write to the result */
	ptr = result;
	
	/* format atom */
	ptr = write_data(ptr, (uint8_t*)&formatAtom, sizeof(formatAtom));
	ptr = write_data(ptr, waveAtom, waveSize);
	ptr = write_data(ptr, (uint8_t*)&termAtom, sizeof(termAtom));
	
bail:
		*cookieSize = size;
	if(waveAtom)
		av_free(waveAtom);
	return result;
} /* create_cookie() */

/* This function creates an image description extension that some codecs need to be able 
 * to decode properly, a copy of the strf (BITMAPINFOHEADER) chunk in the avi.
 * Return value: a handle to an image description extension which has to be DisposeHandle()'d
 * in cookieSize, the size of the image description extension is returned to the caller */
Handle create_strf_ext(AVCodecContext *codec)
{
	Handle result = NULL;
	uint8_t *ptr;
	long size;
	
	/* initialize the extension
		* 40 bytes			for the BITMAPINFOHEADER stucture, see avienc.c in the ffmpeg project
		* extradata_size	for the data still stored in the AVCodecContext structure */
	size = 40 + codec->extradata_size;
	result = NewHandle(size);
	if (result == NULL)
		goto bail;
	
	/* construct the BITMAPINFOHEADER structure */
	/* QT Atoms are big endian, but the STRF atom should be little endian */
	ptr = write_int32((uint8_t *)*result, EndianS32_NtoL(size)); /* size */
	ptr = write_int32(ptr, EndianS32_NtoL(codec->width));
	ptr = write_int32(ptr, EndianS32_NtoL(codec->height));
	ptr = write_int16(ptr, EndianS16_NtoL(1)); /* planes */
	
	ptr = write_int16(ptr, EndianS16_NtoL(codec->bits_per_sample ? codec->bits_per_sample : 24)); /* depth */
	/* compression type */
	ptr = write_int32(ptr, EndianS32_NtoL(codec->codec_tag));
	ptr = write_int32(ptr, EndianS32_NtoL(codec->width * codec->height * 3));
	ptr = write_int32(ptr, EndianS32_NtoL(0));
	ptr = write_int32(ptr, EndianS32_NtoL(0));
	ptr = write_int32(ptr, EndianS32_NtoL(0));
	ptr = write_int32(ptr, EndianS32_NtoL(0));
	
	/* now the remaining stuff */
	ptr = write_data(ptr, codec->extradata, codec->extradata_size);
	
	if (codec->extradata_size & 1) {
		uint8_t zero = 0;
		ptr = write_data(ptr, &zero, 1);
	}
	
bail:
	return result;
} /* create_extension() */

/* write the int32_t data to target & then return a pointer which points after that data */
uint8_t *write_int32(uint8_t *target, int32_t data)
{
	return write_data(target, (uint8_t*)&data, sizeof(data));
} /* write_int32() */

/* write the int16_t data to target & then return a pointer which points after that data */
uint8_t *write_int16(uint8_t *target, int16_t data)
{
	return write_data(target, (uint8_t*)&data, sizeof(data));
} /* write_int16() */

/* write the data to the target adress & then return a pointer which points after the written data */
uint8_t *write_data(uint8_t *target, uint8_t* data, int32_t data_size)
{
	if(data_size > 0)
		memcpy(target, data, data_size);
	return (target + data_size);
} /* write_data() */


/* Add the meta data that lavf exposes to the movie */
void add_metadata(AVFormatContext *ic, Movie theMovie)
{
    QTMetaDataRef movie_metadata;
    OSType key, err;
    
    err = QTCopyMovieMetaData(theMovie, &movie_metadata);
    if (err) return;
    
    key = kQTMetaDataCommonKeyDisplayName;
    QTMetaDataAddItem(movie_metadata, kQTMetaDataStorageFormatQuickTime, kQTMetaDataKeyFormatCommon, 
                      (UInt8 *)&key, sizeof(key), (UInt8 *)ic->title, strlen(ic->title), kQTMetaDataTypeUTF8, NULL);

    key = kQTMetaDataCommonKeyAuthor;
    QTMetaDataAddItem(movie_metadata, kQTMetaDataStorageFormatQuickTime, kQTMetaDataKeyFormatCommon, 
                      (UInt8 *)&key, sizeof(key), (UInt8 *)ic->author, strlen(ic->author), kQTMetaDataTypeUTF8, NULL);

    key = kQTMetaDataCommonKeyCopyright;
    QTMetaDataAddItem(movie_metadata, kQTMetaDataStorageFormatQuickTime, kQTMetaDataKeyFormatCommon, 
                      (UInt8 *)&key, sizeof(key), (UInt8 *)ic->copyright, strlen(ic->copyright), kQTMetaDataTypeUTF8, NULL);

    key = kQTMetaDataCommonKeyComment;
    QTMetaDataAddItem(movie_metadata, kQTMetaDataStorageFormatQuickTime, kQTMetaDataKeyFormatCommon, 
                      (UInt8 *)&key, sizeof(key), (UInt8 *)ic->comment, strlen(ic->comment), kQTMetaDataTypeUTF8, NULL);

    key = kQTMetaDataCommonKeyComment;
    QTMetaDataAddItem(movie_metadata, kQTMetaDataStorageFormatQuickTime, kQTMetaDataKeyFormatCommon, 
                      (UInt8 *)&key, sizeof(key), (UInt8 *)ic->album, strlen(ic->album), kQTMetaDataTypeUTF8, NULL);

    key = kQTMetaDataCommonKeyGenre;
    QTMetaDataAddItem(movie_metadata, kQTMetaDataStorageFormatQuickTime, kQTMetaDataKeyFormatCommon, 
                      (UInt8 *)&key, sizeof(key), (UInt8 *)ic->genre, strlen(ic->genre), kQTMetaDataTypeUTF8, NULL);
    QTMetaDataRelease(movie_metadata);
}

/* This function prepares the movie to receivve the movie data,
 * After success, *out_map points to a valid stream maping
 * Return values:
 *	  0: ok
 */
int prepare_movie(AVFormatContext *ic, NCStream **out_map, Movie theMovie, Handle dataRef, OSType dataRefType)
{
	int j;
	AVStream *st;
	NCStream *map;
	Track track;
	Track first_audio_track = NULL;
	
	/* make the stream map structure */
	map = av_mallocz(ic->nb_streams * sizeof(NCStream));
	
	for(j = 0; j < ic->nb_streams; j++) {
		
		st = ic->streams[j];
		map[j].index = st->index;
		map[j].str = st;
		
		if(st->codec->codec_type == CODEC_TYPE_VIDEO) {
			track = NewMovieTrack(theMovie, st->codec->width << 16, st->codec->height << 16, kNoVolume);
			initialize_video_map(&map[j], track, dataRef, dataRefType);
		} else if (st->codec->codec_type == CODEC_TYPE_AUDIO) {
			track = NewMovieTrack(theMovie, 0, 0, kFullVolume);
			initialize_audio_map(&map[j], track, dataRef, dataRefType);
			
			if (first_audio_track == NULL)
				first_audio_track = track;
			else
				SetTrackAlternate(track, first_audio_track);
		}
	}
	
    add_metadata(ic, theMovie);
    
	*out_map = map;
	
	return 0;
} /* prepare_movie() */

/* This function imports the avi represented by the AVFormatContext to the movie media represented
 * in the map function. The aviheader_offset is used to calculate the packet offset from the
 * beginning of the file. It returns whether it was successful or not (i.e. whether the file had an index) */
short import_avi(AVFormatContext *ic, NCStream *map, int64_t aviheader_offset)
{
	int j, k, l;
	NCStream *ncstr;
	AVStream *stream;
	AVCodecContext *codec;
	SampleReference64Record sampleRec;
	int64_t offset,duration;
	OSStatus err;
	short flags;
	short hadIndex = 0;
	
	/* process each stream in ic */
	for(j = 0; j < ic->nb_streams; j++) {
		
		ncstr = &map[j];
		stream = ncstr->str;
		codec = stream->codec;
		
		/* no stream we can read */
		if(!ncstr->media)
			continue;
		
		/* now parse the index entries */
		for(k = 0; k < stream->nb_index_entries; k++) {
			
			hadIndex = 1;
			
			/* file offset */
			offset = aviheader_offset + stream->index_entries[k].pos;
			
			/* flags */
			flags = 0;
			if((stream->index_entries[k].flags & AVINDEX_KEYFRAME) == 0)
				flags |= mediaSampleNotSync;
			
			/* set as many fields in sampleRec as possible */
			memset(&sampleRec, 0, sizeof(sampleRec));
			sampleRec.dataOffset.hi = offset >> 32;
			sampleRec.dataOffset.lo = (uint32_t)offset;
			sampleRec.dataSize = stream->index_entries[k].size;
			sampleRec.sampleFlags = flags;
			
			/* some samples have a data_size of zero. if that's the case, ignore them
				* they seem to be used to stretch the frame duration & are already handled
				* by the previous pkt */
			if(sampleRec.dataSize <= 0)
				continue;
			
			/* switch for the remaining fields */
			if(codec->codec_type == CODEC_TYPE_VIDEO) {
				
				/* Calculate the frame duration */
				duration = 1;
				for(l = k+1; l < stream->nb_index_entries; l++) {
					if(stream->index_entries[l].size > 0)
						break;
					duration++;
				}
				
				sampleRec.durationPerSample = map->base.num * duration;
				sampleRec.numberOfSamples = 1;
			}
			else if(codec->codec_type == CODEC_TYPE_AUDIO) {
				
				/* FIXME: check if that's really the right thing to do here */
				if(ncstr->vbr) {
					if(codec->frame_size == ncstr->base.num || (codec->frame_size == 0 && ncstr->base.num > 1)) {
						/* frame_size is set to zero for AAC and some MP3 tracks and it works this way */
						sampleRec.durationPerSample = ncstr->base.num;
						sampleRec.numberOfSamples = 1;
					} else {
						/* this logic seems to be needed even iff ncstr->base.num == 1, but I'm not entirely sure */
						/* This seems to work. Although I have no idea why.
						* Perhaps the stream's timebase is adjusted to
						* let that work. as the timebase has strange values...*/
						sampleRec.durationPerSample = sampleRec.dataSize;
						sampleRec.numberOfSamples = 1;
					}
				} else {
					sampleRec.durationPerSample = 1;
					sampleRec.numberOfSamples = (stream->index_entries[k].size * ncstr->asbd.mFramesPerPacket) / ncstr->asbd.mBytesPerPacket;
				}
			}
			
			/* Add the sample to the media */
			err = AddMediaSampleReferences64(ncstr->media, ncstr->sampleHdl, 1, &sampleRec, NULL);
		}
	}
	return hadIndex;
} /* import_avi() */

/* This function imports the video file using standard av_read_frame() calls, 
 * which works on files that don't have an index */
void import_without_index(AVFormatContext *ic, NCStream *map, int64_t aviheader_offset)
{
	int i;
	NCStream *ncstr;
	AVStream *stream;
	AVCodecContext *codec;
	SampleReference64Record sampleRec;
	short flags;
	AVPacket pkt;
	
	while(av_read_frame(ic, &pkt) == noErr)
	{
		ncstr = &map[pkt.stream_index];
		stream = ncstr->str;
		codec = stream->codec;
		
		flags = 0;
		if((pkt.flags & PKT_FLAG_KEY) == 0)
			flags |= mediaSampleNotSync;
		
		memset(&sampleRec, 0, sizeof(sampleRec));
		sampleRec.dataOffset.hi = pkt.pos >> 32;
		sampleRec.dataOffset.lo = (uint32_t) pkt.pos;
		sampleRec.dataSize = pkt.size;
		sampleRec.sampleFlags = flags;
		
		if(sampleRec.dataSize <= 0)
			continue;
		
		if (codec->codec_type == CODEC_TYPE_AUDIO && !ncstr->vbr)
			sampleRec.numberOfSamples = (pkt.size * ncstr->asbd.mFramesPerPacket) / ncstr->asbd.mBytesPerPacket;
		else
			sampleRec.numberOfSamples = 1;
		
		// we have a sample waiting to be added; calculate the duration and add it
		if (ncstr->lastSample.numberOfSamples > 0) {
			ncstr->lastSample.durationPerSample = (pkt.pts - ncstr->lastpts) * ncstr->base.num;
			AddMediaSampleReferences64(ncstr->media, ncstr->sampleHdl, 1, &ncstr->lastSample, NULL);
		}
		
		if (pkt.duration == 0) {
			// no duration, we'll have to wait for the next packet to calculate it
			// keep the duration of the last sample, so we can use it if it's the last frame
			sampleRec.durationPerSample = ncstr->lastSample.durationPerSample;
			ncstr->lastSample = sampleRec;
			ncstr->lastpts = pkt.pts;
			
		} else {
			ncstr->lastSample.numberOfSamples = 0;
			if (codec->codec_type == CODEC_TYPE_AUDIO && !ncstr->vbr)
				sampleRec.durationPerSample = 1;
			else
				sampleRec.durationPerSample = pkt.duration * ncstr->base.num;
			AddMediaSampleReferences64(ncstr->media, ncstr->sampleHdl, 1, &sampleRec, NULL);
		}
#if 0
		if(codec->codec_type == CODEC_TYPE_VIDEO)
		{
			if(pkt.duration == 0)
				sampleRec.durationPerSample = map->base.num;
			else
				sampleRec.durationPerSample = map->base.num * pkt.duration;
			sampleRec.numberOfSamples = 1;
		}
		else if (codec->codec_type == CODEC_TYPE_AUDIO)
		{
			if(ncstr->vbr) {
				if(codec->frame_size == ncstr->base.num) {
					sampleRec.durationPerSample = codec->frame_size;
					sampleRec.numberOfSamples = 1;
				} else if (ncstr->asbd.mFormatID == kAudioFormatMPEG4AAC) {
					/* AVI-mux GUI, the author of which created this hack in the first place,
					* seems to special-case getting an AAC audio sample's duration this way */
					sampleRec.durationPerSample = ic->streams[pkt.stream_index]->time_base.num;
					sampleRec.numberOfSamples = 1;
				} else {
					/* This seems to work. Although I have no idea why.
					* Perhaps the stream's timebase is adjusted to
					* let that work. as the timebase has strange values...*/
					sampleRec.durationPerSample = sampleRec.dataSize;
					sampleRec.numberOfSamples = 1;
				}
			} else {
				sampleRec.durationPerSample = 1;
				sampleRec.numberOfSamples = (pkt.size * ncstr->asbd.mFramesPerPacket) / ncstr->asbd.mBytesPerPacket;
			}
		}
		err = AddMediaSampleReferences64(ncstr->media, ncstr->sampleHdl, 1, &sampleRec, NULL);
#endif
		//Need to do something like this when the libavformat doesn't give us a position
		/*			Handle dataIn = NewHandle(pkt.size);
		HLock(dataIn);
		memcpy(*dataIn, pkt.data, pkt.size);
		HUnlock(dataIn);
		err = AddMediaSample(ncstr->media, dataIn, 0, pkt.size, 1, ncstr->sampleHdl, pkt.duration, sampleRec.sampleFlags, NULL);*/
		av_free_packet(&pkt);
	}
	// import the last frames
	for (i = 0; i < ic->nb_streams; i++) {
		ncstr = &map[i];
		if (ncstr->lastSample.numberOfSamples > 0)
			AddMediaSampleReferences64(ncstr->media, ncstr->sampleHdl, 1, &ncstr->lastSample, NULL);
	}
}
