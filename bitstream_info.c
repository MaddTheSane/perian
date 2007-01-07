/*
 *  bitstream_info.c
 *  Perian
 *
 *  Created by Graham Booker on 1/6/07.
 *  Copyright 2007 Graham Booker. All rights reserved.
 *
 */

#include "bitstream_info.h"

#include <AudioToolbox/AudioToolbox.h>
#include <QuickTime/QuickTime.h>

#include "ac3tab.h"
static const int nfchans_tbl[8] = { 2, 1, 2, 3, 3, 4, 4, 5 };
static const int ac3_layout_no_lfe[8] = {
	kAudioChannelLayoutTag_Stereo,
	kAudioChannelLayoutTag_Mono,
	kAudioChannelLayoutTag_Stereo,
	kAudioChannelLayoutTag_ITU_3_0,
	kAudioChannelLayoutTag_ITU_2_1,
	kAudioChannelLayoutTag_ITU_3_1,
	kAudioChannelLayoutTag_ITU_2_2,
	kAudioChannelLayoutTag_ITU_3_2};

static const int ac3_layout_lfe[8] = {
	kAudioChannelLayoutTag_DVD_4,
	kAudioChannelLayoutTag_Mono,  //No layout for this, hopefully we never have it
	kAudioChannelLayoutTag_DVD_4,
	kAudioChannelLayoutTag_DVD_10,
	kAudioChannelLayoutTag_DVD_5,
	kAudioChannelLayoutTag_DVD_11,
	kAudioChannelLayoutTag_DVD_6,
	kAudioChannelLayoutTag_ITU_3_2_1};

/* From: http://svn.mplayerhq.hu/ac3/ (LGPL)
 * Synchronize to ac3 bitstream.
 * This function searches for the syncword '0xb77'.
 *
 * @param buf Pointer to "probable" ac3 bitstream buffer
 * @param buf_size Size of buffer
 * @return Returns the position where syncword is found, -1 if no syncword is found
 */
static int ac3_synchronize(uint8_t *buf, int buf_size)
{
    int i;
	
    for (i = 0; i < buf_size - 1; i++)
        if (buf[i] == 0x0b && buf[i + 1] == 0x77)
            return i;
	
    return -1;
}

/* A lot of this was stolen from: http://svn.mplayerhq.hu/ac3/ (LGPL)
 * Fill info from an ac3 stream
 * 
 * @param asdb Pointer to the AudioStreamBasicDescription to fill
 * @param acl Pointer to the AudioChannelLayout to fill
 * @param buffer Pointer to the buffer data to scan
 * @param buff_size Size of the buffer
 * @return 1 if successfull, 0 otherwise
 */

int parse_ac3_bitstream(AudioStreamBasicDescription *asbd, AudioChannelLayout *acl, uint8_t *buffer, int buff_size)
{
	int offset = ac3_synchronize(buffer, buff_size);
	if(offset == -1)
		return 0;
	
	if(buff_size < offset + 7)
		return 0;
	
	uint8_t fscod_and_frmsizecod = buffer[offset + 4];
	
	uint8_t fscod = fscod_and_frmsizecod >> 6;
	uint8_t frmsizecod = fscod_and_frmsizecod & 0x3f;
	if(frmsizecod >= 38)
		return 0;
	
	uint8_t bsid = buffer[offset + 5] >> 3;
	if(bsid > 0x08)
		return 0;
	
	uint8_t acmod = buffer[offset + 6] >> 5;
	uint8_t shift = 4;
	if(acmod & 0x01 && acmod != 0x01)
		shift -= 2;
	if(acmod & 0x04)
		shift -= 2;
	if(acmod == 0x02)
		shift -= 2;
	uint8_t lfe = (buffer[offset + 6] >> shift) & 0x01;
	
	//This is a valid frame!!!
	uint8_t bitrate = ac3_bitratetab[frmsizecod >> 1];
	int sample_rate = ac3_freqs[fscod];
	
	asbd->mSampleRate = sample_rate;
	asbd->mChannelsPerFrame = nfchans_tbl[acmod] + lfe;
	if(lfe)
		acl->mChannelLayoutTag = ac3_layout_lfe[acmod];
	else
		acl->mChannelLayoutTag = ac3_layout_no_lfe[acmod];
	
	return 1;
}
