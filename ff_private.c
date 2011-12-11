/*****************************************************************************
*
*  Avi Import Component Private Functions
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

#include <libavcodec/avcodec.h>
#include <CoreServices/CoreServices.h>
#include <AudioToolbox/AudioToolbox.h>
#include <QuickTime/QuickTime.h>

#include "ff_private.h"
#include "Codecprintf.h"
#include "CommonUtils.h"
#include "CodecIDs.h"
#include "FFmpegUtils.h"
#include "bitstream_info.h"
#include "MatroskaCodecIDs.h"

#undef malloc
#undef free

/* This function prepares the target Track to receivve the movie data,
 * it is called if QT has asked an import operation which should just
 * load this track. After success, *outmap points to a valid stream maping
 * Return values:
 *	  0: ok
 *	< 0: couldn't find a matching track
 */
int prepare_track(ff_global_ptr storage, Track targetTrack, Handle dataRef, OSType dataRefType)
{
	int j;
	AVStream *st = NULL;
	AVStream *outstr = NULL;
	Media media;
	NCStream *map = NULL;
	AVFormatContext *ic = storage->format_context;
	
	/* If the track has already a media, return an err */
	media = GetTrackMedia(targetTrack);
	if(media) goto err;
	
	/* Search the AVFormatContext for a video stream */
	for(j = 0; j < ic->nb_streams && !outstr; j++) {
		st = ic->streams[j];
		if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			outstr = st;
	}
	/* Search the AVFormatContext for an audio stream (no video stream exists) */
	for(j = 0; j < ic->nb_streams && !outstr; j++) {
		st = ic->streams[j];
		if(st->codec->codec_type == AVMEDIA_TYPE_AUDIO)
			outstr = st;
	}
	/* Still no stream, then err */
	if(!outstr) goto err;
	
	/* prepare the stream map & initialize*/
	map = av_mallocz(sizeof(NCStream));
	map->index = st->index;
	map->str = outstr;
	
	if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		initialize_video_map(map, targetTrack, dataRef, dataRefType, storage->firstFrames + st->index);
	else if(st->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		initialize_audio_map(map, targetTrack, dataRef, dataRefType, storage->firstFrames + st->index);
	
	map->valid = map->media && map->sampleHdl;
	
	/* return the map */
	storage->stream_map = map;
	
	return 0;
err:
		if(map)
			av_free(map);
	return -1;
} /* prepare_track() */

/* A very large percent of movies have NTSC timebases (30/1.001) with misrounded fractions, so let's recover them. */
static void rescue_ntsc_timebase(AVRational *base)
{
	av_reduce(&base->num, &base->den, base->num, base->den, INT_MAX);
	
	if (base->num == 1) return; // FIXME: is this good enough?
	
	double fTimebase = av_q2d(*base), nearest_ntsc = floor(fTimebase * 1001. + .5) / 1001.;
	const double small_interval = 1./120.;
	
	if (fabs(fTimebase - nearest_ntsc) < small_interval)
	{
		base->num = 1001;
		base->den = (1001. / fTimebase) + .5;
	}
}

/* Initializes the map & targetTrack to receive video data */
void initialize_video_map(NCStream *map, Track targetTrack, Handle dataRef, OSType dataRefType, AVPacket *firstFrame)
{
	Media media;
	ImageDescriptionHandle imgHdl;
	Handle imgDescExt = NULL;
	AVCodecContext *codec;
	double num,den;
	
	codec = map->str->codec;
	
	map->base = map->str->time_base;
	
	rescue_ntsc_timebase(&map->base);
	if(map->base.num != 1001 && map->base.den > 100000) {
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
	
	if (!((*imgHdl)->cType = map_video_codec_to_mov_tag(codec->codec_id)))
		if(!((*imgHdl)->cType = BSWAP(codec->codec_tag)))
			(*imgHdl)->cType = forced_map_video_codec_to_mov_tag(codec->codec_id);
//	FourCCprintf("fourcc: ", (*imgHdl)->cType);
	
	(*imgHdl)->temporalQuality = codecMaxQuality;
	(*imgHdl)->spatialQuality = codecMaxQuality;
	(*imgHdl)->width = codec->width;
	(*imgHdl)->height = codec->height;
	(*imgHdl)->hRes = 72 << 16;
	(*imgHdl)->vRes = 72 << 16;
	(*imgHdl)->depth = codec->bits_per_coded_sample;
	(*imgHdl)->clutID = -1; // no color lookup table...
	
	// 12 is invalid in mov
	// FIXME: it might be better to set this based entirely on pix_fmt
	if ((*imgHdl)->depth == 12 || (*imgHdl)->depth == 0) (*imgHdl)->depth = codec->pix_fmt == PIX_FMT_YUVA420P ? 32 : 24;
	
	/* Create the strf image description extension (see AVI's BITMAPINFOHEADER) */
	imgDescExt = create_strf_ext(codec);
	if (imgDescExt) {
		AddImageDescriptionExtension(imgHdl, imgDescExt, 'strf');
		DisposeHandle(imgDescExt);
	}
	
	map->sampleHdl = (SampleDescriptionHandle)imgHdl;
	
} /* initialize_video_map() */

/* Initializes the map & targetTrack to receive audio data */
OSStatus initialize_audio_map(NCStream *map, Track targetTrack, Handle dataRef, OSType dataRefType, AVPacket *firstFrame)
{
	Media media;
	SoundDescriptionHandle sndHdl = NULL;
	AudioStreamBasicDescription asbd;
	AVCodecContext *codec;
	UInt32 ioSize;
	OSStatus err = noErr;
	
	uint8_t *cookie = NULL;
	size_t cookieSize = 0;
	
	codec = map->str->codec;
	map->base = map->str->time_base;
	
	media = NewTrackMedia(targetTrack, SoundMediaType, codec->sample_rate, dataRef, dataRefType);
	
	map->media = media;
	
	memset(&asbd,0,sizeof(asbd));
	map_avi_to_mov_tag(codec->codec_id, &asbd, map, codec->channels);
	if(asbd.mFormatID == 0) /* no known codec, use the ms tag */
		asbd.mFormatID = 'ms\0\0' + codec->codec_tag; /* the number is stored in the last byte => big endian */
	
	/* Ask the AudioToolbox about vbr of the codec */
	ioSize = sizeof(UInt32);
	AudioFormatGetProperty(kAudioFormatProperty_FormatIsVBR, sizeof(AudioStreamBasicDescription), &asbd, &ioSize, &map->vbr);
	
	cookie = create_cookie(codec, &cookieSize, asbd.mFormatID, map->vbr);
	/* Set as much of the AudioStreamBasicDescription as possible.
	 * Then ask the codec to correct it by calling FormatInfo before creating the SoundDescriptionHandle.
	 * FormatInfo is poorly documented and doesn't set much of an example for 3rd party codecs but we can hope
	 * they'll overwrite bad values here.
	 */
	asbd.mSampleRate       = codec->sample_rate;
	asbd.mBytesPerPacket   = codec->block_align;
	asbd.mFramesPerPacket  = codec->frame_size;
	asbd.mChannelsPerFrame = codec->channels;
	asbd.mBitsPerChannel   = codec->bits_per_coded_sample;
	
	/* ask the toolbox about more information */
	ioSize = sizeof(AudioStreamBasicDescription);
	err = AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, cookieSize, cookie, &ioSize, &asbd);
	
	// We can't recover from this (FormatInfo resets mFormatID for bad MPEG-4 AOTs)
	if (!asbd.mFormatID || !asbd.mChannelsPerFrame) {
		Codecprintf(NULL, "Audio channels or format not set\n");
		goto bail;
	}
	
	// We might be able to recover from this (at least try to import the packets)
	if (err) {
		Codecprintf(NULL, "AudioFormatGetProperty failed (error %ld / format %lx)\n", err, asbd.mFormatID);
		err = noErr;
	}
	
	// This needs to be set for playback to work, but 10.4 (+ AppleTV) didn't set it in FormatInfo.
	// FIXME anything non-zero (like 1) might work here
	if (!asbd.mFramesPerPacket && asbd.mFormatID == kAudioFormatMPEGLayer3)
		asbd.mFramesPerPacket = asbd.mSampleRate > 24000 ? 1152 : 576;
	
	// if we don't have mBytesPerPacket, we can't import as CBR. Probably should be VBR, and the codec
	// either lied about kAudioFormatProperty_FormatIsVBR or isn't present
	if (asbd.mBytesPerPacket == 0)
		map->vbr = 1;
	
	/* If we have vbr audio, the media scale most likely has to be set to the time_base denumerator */
	if(map->vbr) {
		/* if we have mFramesPerPacket, set mBytesPerPacket to 0 as this can cause
		 * errors if set incorrectly. But in vbr, we just need the mFramesPerPacket
		 * value */
		if(asbd.mFramesPerPacket)
			asbd.mBytesPerPacket = 0;
		
		SetMediaTimeScale(media, map->str->time_base.den);
	}
	
	if (asbd.mFormatID == kAudioFormatLinearPCM)
		asbd.mFramesPerPacket = 1;
	else if (asbd.mBytesPerPacket) {
		/* FIXME: in the MSADPCM codec, we get a wrong mFramesPerPacket entry because
		 * of the difference in the sample_rate and the time_base denumerator. So we
		 * recalculate here the mFramesPerPacket entry */

		/* For calculation, lets assume a packet duration of 1, use ioSize as tmp storage */
		ioSize = map->str->time_base.num * codec->sample_rate / map->str->time_base.den;
		/* downscale to correct bytes_per_packet */
		asbd.mFramesPerPacket = ioSize * asbd.mBytesPerPacket / codec->block_align;
	}
	
	AudioChannelLayout acl;
	int aclSize = 0;  //Set this if you intend to use it
	memset(&acl, 0, sizeof(AudioChannelLayout));

	/* We have to parse the format */
	int useDefault = 1;
	if(asbd.mFormatID == kAudioFormatAC3 || asbd.mFormatID == 'ms \0')
	{
		QTMetaDataRef trackMetaData;
		OSErr error = QTCopyTrackMetaData(targetTrack, &trackMetaData);
		if(error == noErr)
		{
			const char *prop = "Surround";
			OSType key = 'name';
			error = QTMetaDataAddItem(trackMetaData, kQTMetaDataStorageFormatUserData, kQTMetaDataKeyFormatUserData, (UInt8 *)&key, sizeof(key), (UInt8 *)prop, strlen(prop), kQTMetaDataTypeUTF8, NULL);
			QTMetaDataRelease(trackMetaData);
		}
		if(parse_ac3_bitstream(&asbd, &acl, firstFrame->data, firstFrame->size))
		{
			useDefault = 0;
			aclSize = sizeof(AudioChannelLayout);
		}
	}
	if(useDefault && asbd.mChannelsPerFrame > 2)
	{
		acl = GetDefaultChannelLayout(&asbd);
		aclSize = sizeof(AudioChannelLayout);
	}
	
	if (asbd.mSampleRate > 0) {		
		err = QTSoundDescriptionCreate(&asbd, aclSize == 0 ? NULL : &acl, aclSize, cookie, cookieSize, kQTSoundDescriptionKind_Movie_LowestPossibleVersion, &sndHdl);
		
		if(err) {
			fprintf(stderr, "AVI IMPORTER: Error %ld creating the sound description\n", err);
			goto bail;
		}
	}	
	map->sampleHdl = (SampleDescriptionHandle)sndHdl;
	map->asbd = asbd;
	
bail:
	if(cookie)
		av_free(cookie);
	
	return err;
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
		case CODEC_ID_VP6A:
			return 'VP6A';
	}
	return 0;
}

OSType forced_map_video_codec_to_mov_tag(enum CodecID codec_id)
{
	switch (codec_id) {
		case CODEC_ID_H264:
			return 'H264';
		case CODEC_ID_MPEG4:
			return 'MP4S';
	}
	return 0;
}

/* maps the codec_id tag of libavformat to a constant the AudioToolbox can work with */
void map_avi_to_mov_tag(enum CodecID codec_id, AudioStreamBasicDescription *asbd, NCStream *map, int channels)
{
	OSType fourcc = FFCodecIDToFourCC(codec_id);
	
	if (fourcc)
		asbd->mFormatID = fourcc;
	
	switch(codec_id) {
		case CODEC_ID_AC3:
			map->vbr = 1;
			break;
		case CODEC_ID_PCM_S16LE:
			asbd->mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;
			asbd->mBytesPerPacket = 2 * channels;
			break;
		case CODEC_ID_PCM_U8:
			asbd->mFormatFlags = kLinearPCMFormatFlagIsBigEndian;
			asbd->mBytesPerPacket = channels;
			break;
		case CODEC_ID_VORBIS:
			asbd->mFormatID = 'OggV';
			break;
		case CODEC_ID_DTS:
			map->vbr = 1;
			break;
	}
} /* map_avi_to_mov_tag() */

/* This function creates a magic cookie basec on the codec parameter and formatID
 * Return value: a pointer to a magic cookie which has to be av_free()'d
 * in cookieSize, the size of the magic cookie is returned to the caller */
uint8_t *create_cookie(AVCodecContext *codec, size_t *cookieSize, UInt32 formatID, int vbr)
{
	uint8_t *result = NULL;
	uint8_t *ptr;
	AudioFormatAtom formatAtom;
	AudioTerminatorAtom termAtom;
	long waveSize;
	uint8_t *waveAtom = NULL;
	int size = 0;
	
	if (formatID == kAudioFormatMPEG4AAC) {
		return CreateEsdsFromSetupData(codec->extradata, codec->extradata_size, cookieSize, 1, true, false);
	}
	
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
	ptr = write_int32(ptr, EndianS32_NtoB(formatID));
	ptr = write_int16(ptr, EndianS16_NtoL(codec->codec_tag));
	ptr = write_int16(ptr, EndianS16_NtoL(codec->channels));
	ptr = write_int32(ptr, EndianS32_NtoL(codec->sample_rate));
	if(vbr)
		ptr = write_int32(ptr, 0);
	else
		ptr = write_int32(ptr, EndianS32_NtoL(codec->bit_rate / 8));
	ptr = write_int16(ptr, EndianS16_NtoL(codec->block_align));
	ptr = write_int16(ptr, EndianS16_NtoL(codec->bits_per_coded_sample));
	ptr = write_int16(ptr, EndianS16_NtoL(codec->extradata_size));
	/* now the remaining stuff */
	ptr = write_data(ptr, codec->extradata, codec->extradata_size);
	
	/* Calculate the size of the cookie */
	size  = sizeof(formatAtom) + sizeof(termAtom) + waveSize;
	
	/* Audio Format Atom */
	formatAtom.size = EndianS32_NtoB(sizeof(AudioFormatAtom));
	formatAtom.atomType = EndianS32_NtoB(kAudioFormatAtomType);
	formatAtom.format = EndianS32_NtoB(formatID);
	
	/* Terminator Atom */
	termAtom.atomType = EndianS32_NtoB(kAudioTerminatorAtomType);
	termAtom.size = EndianS32_NtoB(sizeof(AudioTerminatorAtom));
	
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
	
	ptr = write_int16(ptr, EndianS16_NtoL(codec->bits_per_coded_sample ? codec->bits_per_coded_sample : 24)); /* depth */
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

/* Add the meta data that lavf exposes to the movie */
static void add_metadata(AVFormatContext *ic, Movie theMovie)
{
    QTMetaDataRef movie_metadata;
    OSType err;
    
    err = QTCopyMovieMetaData(theMovie, &movie_metadata);
	if (err) return;

	void (^AddMetaDataItem)(const char *, OSType) = ^(const char *ff_name, OSType qt_name) {
		AVDictionaryEntry *e = av_dict_get(ic->metadata, ff_name, NULL, 0);
		if (!e) return;
		
		QTMetaDataAddItem(movie_metadata, kQTMetaDataStorageFormatQuickTime, kQTMetaDataKeyFormatCommon,
						  (UInt8*)&qt_name, sizeof(qt_name), (UInt8*)e->value, strlen(e->value), kQTMetaDataTypeUTF8,
						  NULL);
	};

	AddMetaDataItem("title",     kQTMetaDataCommonKeyDisplayName);
	AddMetaDataItem("author",    kQTMetaDataCommonKeyAuthor);
	AddMetaDataItem("artist",    kQTMetaDataCommonKeyArtist);
	AddMetaDataItem("copyright", kQTMetaDataCommonKeyCopyright);
	AddMetaDataItem("comment",   kQTMetaDataCommonKeyComment);
	AddMetaDataItem("album",     kQTMetaDataCommonKeyAlbum);
	AddMetaDataItem("genre",     kQTMetaDataCommonKeyGenre);
	AddMetaDataItem("composer",  kQTMetaDataCommonKeyComposer);
	AddMetaDataItem("encoder",   kQTMetaDataCommonKeySoftware);
	// TODO iTunes track number, disc number, ...

    QTMetaDataRelease(movie_metadata);
}

static void get_track_dimensions_for_codec(AVStream *st, Fixed *fixedWidth, Fixed *fixedHeight)
{	
	AVCodecContext *codec = st->codec;
	*fixedHeight = IntToFixed(codec->height);

	if (!st->sample_aspect_ratio.num) *fixedWidth = IntToFixed(codec->width);
	else *fixedWidth = FloatToFixed(codec->width * av_q2d(st->sample_aspect_ratio));
}

// Create the 'pasp' atom for video tracks. No guesswork required.
// References http://www.uwasa.fi/~f76998/video/conversion/
void set_track_clean_aperture_ext(ImageDescriptionHandle imgDesc, Fixed displayW, Fixed displayH, Fixed pixelW, Fixed pixelH)
{
	if (displayW == pixelW && displayH == pixelH)
		return;
		
	AVRational dar, invPixelSize, sar;
	
	dar			   = (AVRational){displayW, displayH};
	invPixelSize   = (AVRational){pixelH, pixelW};
	sar = av_mul_q(dar, invPixelSize);
	
	av_reduce(&sar.num, &sar.den, sar.num, sar.den, fixed1);
	
	if (sar.num == sar.den)
		return;
		
	PixelAspectRatioImageDescriptionExtension **pasp = (PixelAspectRatioImageDescriptionExtension**)NewHandle(sizeof(PixelAspectRatioImageDescriptionExtension));
	
	**pasp = (PixelAspectRatioImageDescriptionExtension){EndianU32_NtoB(sar.num), EndianU32_NtoB(sar.den)};
	
	AddImageDescriptionExtension(imgDesc, (Handle)pasp, kPixelAspectRatioImageDescriptionExtension);
	
	DisposeHandle((Handle)pasp);
}

// Create the 'nclc' atom for video tracks. Guessed entirely from image size following ffdshow.
// FIXME: read H.264 VUI/MPEG2 etc and especially read chroma positioning information.
// this needs the parsers working
// References: http://developer.apple.com/quicktime/icefloe/dispatch019.html
// http://www.mir.com/DMG/chroma.html
void set_track_colorspace_ext(ImageDescriptionHandle imgDescHandle, Fixed displayW, Fixed displayH)
{
	ImageDescription *imgDesc = *imgDescHandle;
	Boolean isHd, isPAL; // otherwise NTSC
	AVRational palRatio = (AVRational){5, 4}, displayRatio = (AVRational){displayW, displayH};
	int colorPrimaries, transferFunction, yuvMatrix;
	
	isHd  = imgDesc->height >  576;
	isPAL = imgDesc->height == 576 || av_cmp_q(palRatio, displayRatio) == 0;
	
	NCLCColorInfoImageDescriptionExtension **nclc = (NCLCColorInfoImageDescriptionExtension**)NewHandle(sizeof(NCLCColorInfoImageDescriptionExtension));
		
	if (isHd) {
		colorPrimaries = kQTPrimaries_ITU_R709_2;
		transferFunction = kQTTransferFunction_ITU_R709_2;
		yuvMatrix = kQTMatrix_ITU_R_709_2;
	} else if (isPAL) {
		colorPrimaries = kQTPrimaries_EBU_3213;
		transferFunction = kQTTransferFunction_ITU_R709_2;
		yuvMatrix = kQTMatrix_ITU_R_601_4;
	} else {
		colorPrimaries = kQTPrimaries_SMPTE_C;
		transferFunction = kQTTransferFunction_ITU_R709_2;
		yuvMatrix = kQTMatrix_ITU_R_601_4;
	}
	
	**nclc = (NCLCColorInfoImageDescriptionExtension){EndianU32_NtoB(kVideoColorInfoImageDescriptionExtensionType),
													  EndianU16_NtoB(colorPrimaries),
													  EndianU16_NtoB(transferFunction),
													  EndianU16_NtoB(yuvMatrix)};
	
	AddImageDescriptionExtension(imgDescHandle, (Handle)nclc, kColorInfoImageDescriptionExtension);
	
	DisposeHandle((Handle)nclc);
}

/* This function prepares the movie to receivve the movie data,
 * After success, *out_map points to a valid stream maping
 * Return values:
 *	  0: ok
 */
OSStatus prepare_movie(ff_global_ptr storage, Movie theMovie, Handle dataRef, OSType dataRefType)
{
	int j;
	AVStream *st;
	NCStream *map;
	Track track = NULL;
	Track first_audio_track = NULL;
	AVFormatContext *ic = storage->format_context;
	OSStatus err = noErr;
	
	/* make the stream map structure */
	map = av_mallocz(ic->nb_streams * sizeof(NCStream));
	
	for(j = 0; j < ic->nb_streams; j++) {
		
		st = ic->streams[j];
		map[j].index = st->index;
		map[j].str = st;
		map[j].duration = -1;
		
		if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			Fixed width, height;
			
			get_track_dimensions_for_codec(st, &width, &height);
			track = NewMovieTrack(theMovie, width, height, kNoVolume);

            // Support for 'old' NUV files, that didn't put the codec_tag in the file. 
            if( st->codec->codec_id == CODEC_ID_NUV && st->codec->codec_tag == 0 ) {
                st->codec->codec_tag = MKTAG( 'N', 'U', 'V', '1' );
            }
			
			initialize_video_map(&map[j], track, dataRef, dataRefType, storage->firstFrames + j);
			set_track_clean_aperture_ext((ImageDescriptionHandle)map[j].sampleHdl, width, height, IntToFixed(st->codec->width), IntToFixed(st->codec->height));
			set_track_colorspace_ext((ImageDescriptionHandle)map[j].sampleHdl, width, height);
		} else if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			if (st->codec->sample_rate > 0) {
				track = NewMovieTrack(theMovie, 0, 0, kFullVolume);
				err = initialize_audio_map(&map[j], track, dataRef, dataRefType, storage->firstFrames + j);
				
				if (first_audio_track == NULL)
					first_audio_track = track;
				else
					SetTrackAlternate(track, first_audio_track);
			}
		} else
			continue;
		
		// can't import samples if neither of these were created.
		map[j].valid = map[j].media && map[j].sampleHdl;
		
		if (err) {
			if (track)
				DisposeMovieTrack(track);
			return err;
		}			
	}
	
    add_metadata(ic, theMovie);
    
	storage->stream_map = map;
	
	return 0;
} /* prepare_movie() */

int determine_header_offset(ff_global_ptr storage) {
	AVFormatContext *formatContext;
	AVPacket packet;
	int result, i;
	
	formatContext = storage->format_context;
	result = noErr;
	storage->header_offset = 0;
	
	/* Seek backwards to get a manually read packet for file offset */
	if(formatContext->streams[0]->index_entries == NULL || storage->componentType == 'FLV ')
	{
		storage->header_offset = 0;
	}
	else
	{
		int streamsRead = 0;
		AVStream *st;
		
		result = av_seek_frame(formatContext, -1, 0, AVSEEK_FLAG_ANY);
		if(result < 0) goto bail;
		
		result = av_read_frame(formatContext, &packet);
		if(result < 0) goto bail;
		st = formatContext->streams[packet.stream_index];
		
		/* read_packet will give the first decodable packet. However, that isn't necessarily
			the first entry in the index, so look for an entry with a matching size. */
		for (i = 0; i < st->nb_index_entries; i++) {
			if (packet.dts == st->index_entries[i].timestamp) {
				storage->header_offset = packet.pos - st->index_entries[i].pos;
				break;
			}
		}
		while(streamsRead < formatContext->nb_streams)
		{
			int streamIndex = packet.stream_index;
			if(storage->firstFrames[streamIndex].size == 0)
			{
				memcpy(storage->firstFrames + streamIndex, &packet, sizeof(AVPacket));
				streamsRead++;
				if(streamsRead == formatContext->nb_streams)
					break;
			}
			else
				av_free_packet(&packet);
			int status = formatContext->iformat->read_packet(formatContext, &packet);
			if(status < 0)
				break;
		}
		
		// seek back to the beginning, otherwise av_read_frame-based decoding will skip a few packets.
		av_seek_frame(formatContext, -1, 0, AVSEEK_FLAG_ANY | AVSEEK_FLAG_BACKWARD);
	}
		
bail:
	return result;
}

/* This function imports the avi represented by the AVFormatContext to the movie media represented
 * in the map function. The aviheader_offset is used to calculate the packet offset from the
 * beginning of the file. It returns whether it was successful or not (i.e. whether the file had an index) */
int import_using_index(ff_global_ptr storage, int *hadIndex, TimeValue *addedDuration) {
	int j, k, l;
	NCStream *map;
	NCStream *ncstr;
	AVFormatContext *ic;
	AVStream *stream;
	AVCodecContext *codec;
	SampleReference64Ptr sampleRec;
	int64_t header_offset, offset, duration;
	short flags;
	int sampleNum;
	ComponentResult result = noErr;
	
	map = storage->stream_map;
	ic = storage->format_context;
	header_offset = storage->header_offset;

	if(*hadIndex == 0)
		goto bail;
		
	//FLVs have unusable indexes, so don't even bother.
	if(storage->componentType == 'FLV ')
		goto bail;
	
	/* process each stream in ic */
	for(j = 0; j < ic->nb_streams; j++) {
		ncstr = &map[j];
		stream = ncstr->str;
		codec = stream->codec;
		
		/* no stream we can read */
		if(!ncstr->valid)
			continue;
		
		/* no index, we might as well skip */
		if(stream->nb_index_entries == 0)
			continue;
		
		sampleNum = 0;
		ncstr->sampleTable = calloc(stream->nb_index_entries, sizeof(SampleReference64Record));
			
		/* now parse the index entries */
		for(k = 0; k < stream->nb_index_entries; k++) {
			
			/* file offset */
			offset = header_offset + stream->index_entries[k].pos;
			
			/* flags */
			flags = 0;
			if((stream->index_entries[k].flags & AVINDEX_KEYFRAME) == 0)
				flags |= mediaSampleNotSync;
			
			sampleRec = &ncstr->sampleTable[sampleNum++];
			
			/* set as many fields in sampleRec as possible */
			sampleRec->dataOffset.hi = offset >> 32;
			sampleRec->dataOffset.lo = (uint32_t)offset;
			sampleRec->dataSize = stream->index_entries[k].size;
			sampleRec->sampleFlags = flags;
			
			/* some samples have a data_size of zero. if that's the case, ignore them
				* they seem to be used to stretch the frame duration & are already handled
				* by the previous pkt */
			if(sampleRec->dataSize <= 0) {
				sampleNum--;
				continue;
			}
			
			/* switch for the remaining fields */
			if(codec->codec_type == AVMEDIA_TYPE_VIDEO) {
				
				/* Calculate the frame duration */
				duration = 1;
				for(l = k+1; l < stream->nb_index_entries; l++) {
					if(stream->index_entries[l].size > 0)
						break;
					duration++;
				}
				
				sampleRec->durationPerSample = map->base.num * duration;
				sampleRec->numberOfSamples = 1;
			}
			else if(codec->codec_type == AVMEDIA_TYPE_AUDIO) {
				
				/* FIXME: check if that's really the right thing to do here */
				if(ncstr->vbr) {
					sampleRec->numberOfSamples = 1;
					
					if (k + 1 < stream->nb_index_entries)
						sampleRec->durationPerSample = (stream->index_entries[k+1].timestamp - stream->index_entries[k].timestamp) * ncstr->base.num;
					else if (sampleNum - 2 >= 0)
						// if we're at the last index entry, use the duration of the previous sample
						// FIXME: this probably could be better
						sampleRec->durationPerSample = ncstr->sampleTable[sampleNum-2].durationPerSample;
					
				} else {
					sampleRec->durationPerSample = 1;
					sampleRec->numberOfSamples = (stream->index_entries[k].size * ncstr->asbd.mFramesPerPacket) / ncstr->asbd.mBytesPerPacket;
				}
			}
		}
		if(sampleNum != 0)
		{
			/* Add all of the samples to the media */
			AddMediaSampleReferences64(ncstr->media, ncstr->sampleHdl, sampleNum, ncstr->sampleTable, NULL);

			/* The index is both present and not empty */
			*hadIndex = 1;
		}
		free(ncstr->sampleTable);			
	}
	
	if(*hadIndex == 0)
		//No index, the remainder of this function will fail.
		goto bail;
	
	// insert media and set addedDuration;
	for(j = 0; j < storage->map_count && result == noErr; j++) {
		ncstr = &map[j];
		if(ncstr->valid) {
			Media media = ncstr->media;
			Track track;
			TimeRecord time;
			TimeValue mediaDuration;
			TimeScale mediaTimeScale;
			TimeScale movieTimeScale;
			int startTime = map[j].str->index_entries[0].timestamp;

			mediaDuration = GetMediaDuration(media);
			mediaTimeScale = GetMediaTimeScale(media);
			movieTimeScale = GetMovieTimeScale(storage->movie);
			
			/* we could handle this stream.
			* convert the atTime parameter to track scale.
			* FIXME: check if that's correct */
			time.value.hi = 0;
			time.value.lo = storage->atTime;
			time.scale = movieTimeScale;
			time.base = NULL;
			ConvertTimeScale(&time, mediaTimeScale);
			
			track = GetMediaTrack(media);
			result = InsertMediaIntoTrack(track, time.value.lo, 0, mediaDuration, fixed1);

			// set audio/video start delay
			// note str.start_time exists but is always 0 for AVI
			if (startTime) {
				TimeRecord startTimeRec;
				startTimeRec.value.hi = 0;
				startTimeRec.value.lo = startTime * map[j].str->time_base.num;
				startTimeRec.scale = map[j].str->time_base.den;
				startTimeRec.base = NULL;
				ConvertTimeScale(&startTimeRec, movieTimeScale);
				SetTrackOffset(track, startTimeRec.value.lo);
			}
			
			if(result != noErr)
				goto bail;
			
			time.value.hi = 0;
			time.value.lo = mediaDuration;
			time.scale = mediaTimeScale;
			time.base = NULL;
			ConvertTimeScale(&time, movieTimeScale);
			
			if(time.value.lo > *addedDuration)
				*addedDuration = time.value.lo;
		}
	}
	
	storage->loadedTime = *addedDuration;
	
bail:
	return result;
} /* import_using_index() */

/* Import function for movies that lack an index.
 * Supports progressive importing, but will not idle if maxFrames == 0.
 */
ComponentResult import_with_idle(ff_global_ptr storage, long inFlags, long *outFlags, int minFrames, int maxFrames, bool addSamples) {
	SampleReference64Record sampleRec;
	DataHandler dataHandler;
	AVFormatContext *formatContext;
	AVCodecContext *codecContext;
	AVStream *stream;
	AVPacket packet;
	NCStream *ncstream;
	ComponentResult dataResult; //used for data handler operations that can fail.
	ComponentResult result;
	TimeValue minLoadedTime;
	TimeValue movieTimeScale = GetMovieTimeScale(storage->movie);
	int64_t availableSize, margin;
	long idling;
	int readResult, framesProcessed, i;
	int firstPts[storage->map_count];
	short flags;
	
	dataHandler = storage->dataHandler;
	formatContext = storage->format_context;
	dataResult = noErr;
	result = noErr;
	minLoadedTime = -1;
	availableSize = 0;
	margin = 0;
	idling = (inFlags & movieImportWithIdle);
	framesProcessed = 0;
		
	if(idling) {
		//get the size of immediately available data
		if(storage->dataHandlerSupportsWideOffsets) {
			wide wideSize;
			
			dataResult = DataHGetAvailableFileSize64(storage->dataHandler, &wideSize);
			if(dataResult == noErr) availableSize = ((int64_t)wideSize.hi << 32) + wideSize.lo;
		} else {
			long longSize;
			
			dataResult = DataHGetAvailableFileSize(storage->dataHandler, &longSize);
			if(dataResult == noErr) availableSize = longSize;
		}
	}
	
	for(i = 0; i < storage->map_count; i++)
		firstPts[i] = -1;
	
	// record stream durations before we add any samples so that we know what to tell InsertMediaIntoTrack later
	// also, tell ffmpeg that we don't want them parsing because their parsers screw up the position/size data!!!
	for(i = 0; i < storage->map_count; i++) {
		ncstream = &storage->stream_map[i];
		Media media = ncstream->media;
		
		if(media && ncstream->duration == -1)
			ncstream->duration = GetMediaDuration(media);
		ncstream->str->need_parsing = AVSTREAM_PARSE_NONE;
	}
	
	while((readResult = av_read_frame(formatContext, &packet)) == 0) {		
		bool trustPacketDuration = true;
		ncstream = &storage->stream_map[packet.stream_index];
		stream = ncstream->str;
		codecContext = stream->codec;
		flags = 0;
		
		if (!ncstream->valid)
			continue;
		
		if((packet.flags & AV_PKT_FLAG_KEY) == 0)
			flags |= mediaSampleNotSync;
		
		if(IS_NUV(storage->componentType) && codecContext->codec_id == CODEC_ID_MP3) trustPacketDuration = false;
		if(IS_FLV(storage->componentType) && codecContext->codec_id == CODEC_ID_H264) trustPacketDuration = false;
		if(IS_FLV(storage->componentType) && codecContext->codec_type == AVMEDIA_TYPE_AUDIO) trustPacketDuration = false;
		
		memset(&sampleRec, 0, sizeof(sampleRec));
		sampleRec.dataOffset.hi = packet.pos >> 32;
		sampleRec.dataOffset.lo = (uint32_t)packet.pos;
		sampleRec.dataSize = packet.size;
		sampleRec.sampleFlags = flags;
				
		if(firstPts[packet.stream_index] < 0)
			firstPts[packet.stream_index] = packet.pts;
		
		if(packet.size > storage->largestPacketSize)
			storage->largestPacketSize = packet.size;
		
		if(sampleRec.dataSize <= 0)
			continue;
		
		if(codecContext->codec_type == AVMEDIA_TYPE_AUDIO && !ncstream->vbr)
			sampleRec.numberOfSamples = (packet.size * ncstream->asbd.mFramesPerPacket) / ncstream->asbd.mBytesPerPacket;
		else
			sampleRec.numberOfSamples = 1; //packet.duration;
		
		//add any samples waiting to be added
		if(ncstream->lastSample.numberOfSamples > 0) {
			//calculate the duration of the sample before adding it
			ncstream->lastSample.durationPerSample = (packet.dts - ncstream->lastdts) * ncstream->base.num;
			
			AddMediaSampleReferences64(ncstream->media, ncstream->sampleHdl, 1, &ncstream->lastSample, NULL);
		}
		

        // If this is a nuv file, then we want to set the duration to zero.
        // This is because the nuv container doesn't have the framesize info 
        // for audio. 

		if(packet.duration == 0 || !trustPacketDuration) {
			//no duration, we'll have to wait for the next packet to calculate it
			// keep the duration of the last sample, so we can use it if it's the last frame
			sampleRec.durationPerSample = ncstream->lastSample.durationPerSample;
			ncstream->lastSample = sampleRec;
			ncstream->lastdts = packet.dts;
		} else {
			ncstream->lastSample.numberOfSamples = 0;
			
			if(codecContext->codec_type == AVMEDIA_TYPE_AUDIO && !ncstream->vbr)
				sampleRec.durationPerSample = 1;
			else
				sampleRec.durationPerSample = ncstream->base.num * packet.duration;

			AddMediaSampleReferences64(ncstream->media, ncstream->sampleHdl, 1, &sampleRec, NULL);
		}
		
		framesProcessed++;
		
		//if we're idling, try really not to read past the end of available data
		//otherwise we will cause blocking i/o.
		if(idling && framesProcessed >= minFrames && availableSize > 0 && availableSize < storage->dataSize) {
			margin = availableSize - (packet.pos + packet.size);
			if(margin < (storage->largestPacketSize * 8)) { // 8x fudge factor for comfortable margin, could be tweaked.
				av_free_packet(&packet);
				break;
			}
		}
		
		av_free_packet(&packet);
		
		//stop processing if we've hit the max frame limit
		if(maxFrames > 0 && framesProcessed >= maxFrames)
			break;
	}
		
	if(readResult != 0) {
		//if readResult != 0, we've hit the end of the stream.
		//add any pending last frames.
		for(i = 0; i < formatContext->nb_streams; i++) {
			ncstream = &storage->stream_map[i];
			if(ncstream->lastSample.numberOfSamples > 0)
				AddMediaSampleReferences64(ncstream->media, ncstream->sampleHdl, 1, &ncstream->lastSample, NULL);
		}
	}
	
	for(i = 0; i < storage->map_count && result == noErr; i++) {
		ncstream = &storage->stream_map[i];
		Media media = ncstream->media;
		
		if(ncstream->valid && (addSamples || readResult != 0)) {
			Track track = GetMediaTrack(media);
			TimeScale mediaTimeScale = GetMediaTimeScale(media);
			TimeValue prevDuration = ncstream->duration;
			TimeValue mediaDuration = GetMediaDuration(media);
			TimeValue addedDuration = mediaDuration - prevDuration;
			TimeValue mediaLoadedTime = movieTimeScale * mediaDuration / mediaTimeScale;
			
			if(minLoadedTime == -1 || mediaLoadedTime < minLoadedTime)
				minLoadedTime = mediaLoadedTime;
			
			if(addedDuration > 0) {
				result = InsertMediaIntoTrack(track, -1, prevDuration, addedDuration, fixed1);
			}
			
			if (!prevDuration && firstPts[i] > 0) {
				TimeRecord startTimeRec;
				startTimeRec.value.hi = 0;
				startTimeRec.value.lo = firstPts[i] * formatContext->streams[i]->time_base.num;
				startTimeRec.scale = formatContext->streams[i]->time_base.den;
				startTimeRec.base = NULL;
				ConvertTimeScale(&startTimeRec, movieTimeScale);
				SetTrackOffset(track, startTimeRec.value.lo);
			}
			ncstream->duration = -1;
		}
	}
	
	//set the loaded time to the length of the shortest track.
	if(minLoadedTime > 0)
		storage->loadedTime = minLoadedTime;
	
	if(readResult != 0) {
		//remove the placeholder track
		if(storage->placeholderTrack != NULL) {
			DisposeMovieTrack(storage->placeholderTrack);
			storage->placeholderTrack = NULL;
		}
		
		//set the movie load state to complete, as well as mark the import output flag.
		storage->movieLoadState = kMovieLoadStateComplete;
		*outFlags |= movieImportResultComplete;		
	} else {
		//if we're not yet done with the import, calculate the movie load state.
		int64_t timeToCompleteFile; //time until the file should be completely available, in terms of AV_TIME_BASE
		long dataRate = 0;
		
		dataResult = DataHGetDataRate(storage->dataHandler, 0, &dataRate);
		if(dataResult == noErr && dataRate > 0) {
			timeToCompleteFile = (AV_TIME_BASE * (storage->dataSize - availableSize)) / dataRate;
			
			if(storage->loadedTime > (10 * GetMovieTimeScale(storage->movie)) && timeToCompleteFile < (storage->format_context->duration * .85))
				storage->movieLoadState = kMovieLoadStatePlaythroughOK;
			else
				storage->movieLoadState = kMovieLoadStatePlayable;
			
		} else {
			storage->movieLoadState = kMovieLoadStatePlayable;
		}
		
		*outFlags |= movieImportResultNeedIdles;
	}
	
	send_movie_changed_notification(storage->movie);
	
	//tell the idle manager to idle us again in 500ms.
	if(idling && storage->idleManager && storage->isStreamed)
		QTIdleManagerSetNextIdleTimeDelta(storage->idleManager, 1, 2);
	
	return(result);
} /* import_with_idle() */

ComponentResult create_placeholder_track(Movie movie, Track *placeholderTrack, TimeValue duration, Handle dataRef, OSType dataRefType) {
	SampleDescriptionHandle sdH = NULL;
	Media placeholderMedia;
	TimeScale movieTimeScale;
	ComponentResult result = noErr;
	
	movieTimeScale = GetMovieTimeScale(movie);
	
	sdH = (SampleDescriptionHandle)NewHandleClear(sizeof(SampleDescription));
	(*sdH)->descSize = sizeof(SampleDescription);
	
	*placeholderTrack = NewMovieTrack(movie, 0, 0, kNoVolume);
	placeholderMedia = NewTrackMedia(*placeholderTrack, BaseMediaType, movieTimeScale, dataRef, dataRefType);
	
	result = AddMediaSampleReference(placeholderMedia, 0, 1, duration, sdH, 1, 0, NULL);
	if(result != noErr)
		goto bail;
	
	result = InsertMediaIntoTrack(*placeholderTrack, -1, 0, duration, fixed1);
	
bail:
	if (sdH)
		DisposeHandle((Handle) sdH);
	return(result);
}

void send_movie_changed_notification(Movie movie) {
	QTAtomContainer container;
	
	if(QTNewAtomContainer(&container) == noErr) {
		QTAtom anAction;
		OSType whichAction = EndianU32_NtoB(kActionMovieChanged);
		
		OSErr err = QTInsertChild(container, kParentAtomIsContainer, kAction, 1, 0, 0, NULL, &anAction);
		
		if(err == noErr)
			err = QTInsertChild(container, anAction, kWhichAction, 1, 0, sizeof(whichAction), &whichAction, NULL);
		
		if(err == noErr)
			MovieExecuteWiredActions(movie, 0, container);
		
		err = QTDisposeAtomContainer(container);
	}	
}