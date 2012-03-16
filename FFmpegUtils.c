/*
 * FFmpegUtils.c
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
 * FFmpeg API interfaces and such
 */

#include <pthread.h>
#include <libavformat/avformat.h>

#include "FFmpegUtils.h"
#include "CommonUtils.h"
#include "Codecprintf.h"
#include "CodecIDs.h"

void FFAVCodecContextToASBD(AVCodecContext *avctx, AudioStreamBasicDescription *asbd)
{
	asbd->mFormatID         = avctx->codec_tag;
	asbd->mSampleRate       = avctx->sample_rate;
	asbd->mChannelsPerFrame = avctx->channels;
	asbd->mBytesPerPacket   = avctx->block_align;
	asbd->mFramesPerPacket  = avctx->frame_size;
	asbd->mBitsPerChannel   = avctx->bits_per_coded_sample;
}

void FFASBDToAVCodecContext(AudioStreamBasicDescription *asbd, AVCodecContext *avctx)
{
	avctx->codec_tag   = asbd->mFormatID;
	avctx->sample_rate = asbd->mSampleRate;
	avctx->channels    = asbd->mChannelsPerFrame;
	avctx->block_align = asbd->mBytesPerPacket;
	avctx->frame_size  = asbd->mFramesPerPacket;
	avctx->bits_per_coded_sample = asbd->mBitsPerChannel;
}

static int PerianLockMgrCallback(void **mutex, enum AVLockOp op)
{
	pthread_mutex_t **m = (pthread_mutex_t **)mutex;
	int ret = 0;
	
	switch (op) {
		case AV_LOCK_CREATE:
			*m = malloc(sizeof(pthread_mutex_t));
			ret = pthread_mutex_init(*m, NULL);
			break;
		case AV_LOCK_OBTAIN:
			ret = pthread_mutex_lock(*m);
			break;
		case AV_LOCK_RELEASE:
			ret = pthread_mutex_unlock(*m);
			break;
		case AV_LOCK_DESTROY:
			ret = pthread_mutex_destroy(*m);
			free(*m);
	}
	
	return ret;
}

#define REGISTER_DEMUXER(x) { \
	extern AVInputFormat ff_##x##_demuxer; \
		av_register_input_format(&ff_##x##_demuxer); }

#define REGISTER_DECODER(x) { \
	extern AVCodec ff_##x##_decoder; \
		avcodec_register(&ff_##x##_decoder); }

#define REGISTER_PARSER(x) { \
	extern AVCodecParser ff_##x##_parser; \
		av_register_codec_parser(&ff_##x##_parser); }

void FFInitFFmpeg()
{
	static dispatch_once_t once;
	
	dispatch_once(&once, ^{
		av_lockmgr_register(PerianLockMgrCallback);

		REGISTER_DEMUXER(avi);
		REGISTER_DEMUXER(flv);
		REGISTER_DEMUXER(tta);
		REGISTER_DEMUXER(nuv);
		REGISTER_PARSER(ac3);
		REGISTER_PARSER(mpegaudio);
		
		REGISTER_DECODER(msmpeg4v1);
		REGISTER_DECODER(msmpeg4v2);
		REGISTER_DECODER(msmpeg4v3);
		REGISTER_DECODER(mpeg4);
		REGISTER_DECODER(h264);
		REGISTER_DECODER(flv);
		REGISTER_DECODER(flashsv);
		REGISTER_DECODER(vp3);
		REGISTER_DECODER(vp6);
		REGISTER_DECODER(vp6f);
		REGISTER_DECODER(vp8);
		REGISTER_DECODER(h263i);
		REGISTER_DECODER(huffyuv);
		REGISTER_DECODER(ffvhuff);
		REGISTER_DECODER(mpeg1video);
		REGISTER_DECODER(mpeg2video);
		REGISTER_DECODER(fraps);
		REGISTER_DECODER(snow);
		REGISTER_DECODER(nuv);
		REGISTER_DECODER(ffv1);
		REGISTER_DECODER(theora);
		
		REGISTER_DECODER(wmav1);
		REGISTER_DECODER(wmav2);
		REGISTER_DECODER(adpcm_swf);
		REGISTER_DECODER(vorbis);
		REGISTER_DECODER(mp1);
		REGISTER_DECODER(mp2);
		REGISTER_DECODER(tta);
		REGISTER_DECODER(dca);
		REGISTER_DECODER(nellymoser);
		REGISTER_DECODER(aac);
		
		REGISTER_DECODER(dvdsub);
		REGISTER_DECODER(tscc);
		REGISTER_DECODER(vp6a);
		REGISTER_DECODER(zmbv);
		REGISTER_DECODER(indeo2);
		REGISTER_DECODER(indeo3);
		REGISTER_DECODER(indeo5);
		REGISTER_DECODER(dxtory);
		
		av_log_set_callback(FFMpegCodecprintf);
	});
}

// List of codec IDs we know about and that map to audio fourccs
// XXX this is probably a duplicate of something inside libavformat
static const struct {
	OSType mFormatID;
	enum CodecID codecID;
} kAudioCodecMap[] =
{
	{ kAudioFormatWMA1MS, CODEC_ID_WMAV1 },
	{ kAudioFormatWMA2MS, CODEC_ID_WMAV2 },
	{ kAudioFormatFlashADPCM, CODEC_ID_ADPCM_SWF },
	{ kAudioFormatXiphVorbis, CODEC_ID_VORBIS },
	{ kAudioFormatMPEGLayer1, CODEC_ID_MP1 },
	{ kAudioFormatMPEGLayer2, CODEC_ID_MP2 },
	{ kAudioFormatMPEGLayer3, CODEC_ID_MP3 },
	{ 'ms\0\0' + 0x50, CODEC_ID_MP2 },
	{ kAudioFormatDTS, CODEC_ID_DTS },
	{ kAudioFormatNellymoser, CODEC_ID_NELLYMOSER },
	{ kAudioFormatTTA, CODEC_ID_TTA },
	
	{ kAudioFormatAC3MS, CODEC_ID_AC3 },
	{ kAudioFormatLinearPCM, CODEC_ID_PCM_S16LE },
	{ kAudioFormatLinearPCM, CODEC_ID_PCM_U8 },
	{ kAudioFormatALaw, CODEC_ID_PCM_ALAW },
	{ kAudioFormatULaw, CODEC_ID_PCM_MULAW },
	{ kMicrosoftADPCMFormat, CODEC_ID_ADPCM_MS },
	{ kAudioFormatMPEG4AAC, CODEC_ID_AAC },
	{ kAudioFormatDTS, CODEC_ID_DTS },
	{ kAudioFormatFlashADPCM, CODEC_ID_ADPCM_SWF },
	{ 0, CODEC_ID_NONE }
};

enum CodecID FFFourCCToCodecID(OSType formatID)
{
	for (int i = 0; kAudioCodecMap[i].codecID != CODEC_ID_NONE; i++) {
		if (kAudioCodecMap[i].mFormatID == formatID)
			return kAudioCodecMap[i].codecID;
	}
	return CODEC_ID_NONE;
}

OSType FFCodecIDToFourCC(enum CodecID codecID)
{
	for (int i = 0; kAudioCodecMap[i].codecID != CODEC_ID_NONE; i++) {
		if (kAudioCodecMap[i].codecID == codecID)
			return kAudioCodecMap[i].mFormatID;
	}
	return CODEC_ID_NONE;
}
