/*
 * bitstream_info.h
 * Created by Graham Booker on 1/6/07.
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

#include "bitstream_info.h"

#include <AudioToolbox/AudioToolbox.h>
#include <QuickTime/QuickTime.h>

#import "ac3tab.h"
#import "CommonUtils.h"
//ffmpeg's struct Picture conflicts with QuickDraw's
#define Picture MPEGPICTURE

#include "avcodec.h"

#include "bswap.h"
#include "libavutil/internal.h"
#include "mpegvideo.h"
#include "parser.h"
#include "golomb.h"
#include "mpeg4video_parser.h"
#include "Codecprintf.h"

#include "CodecIDs.h"

#undef malloc
#undef free

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

static const uint16_t ac3_freqs[3] = { 48000, 44100, 32000 };
static const uint16_t ac3_bitratetab[] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};
static const uint8_t ac3_halfrate[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3};

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
	if(bsid >= 0x12)
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
	
	/* This is a valid frame!!! */
	uint16_t bitrate = ac3_bitratetab[frmsizecod >> 1];
	uint8_t half = ac3_halfrate[bsid];
	int sample_rate = ac3_freqs[fscod] >> half;
	int framesize = 0;
	switch (fscod) {
		case 0:
			framesize = 4 * bitrate;
			break;
		case 1:
			framesize = (320 * bitrate / 147 + (frmsizecod & 1 ? 1 : 0)) * 2;
			break;
		case 2:
			framesize = 6 * bitrate;
			break;
		default:
			break;
	}
	
	shift = 0;
	if(bsid > 8)
		shift = bsid - 8;
	
	/* Setup the AudioStreamBasicDescription and AudioChannelLayout */
	memset(asbd, 0, sizeof(AudioStreamBasicDescription));
	asbd->mSampleRate = sample_rate >> shift;
	asbd->mFormatID = kAudioFormatAC3MS;
	asbd->mChannelsPerFrame = nfchans_tbl[acmod] + lfe;
	
	memset(acl, 0, sizeof(AudioChannelLayout));
	if(lfe)
		acl->mChannelLayoutTag = ac3_layout_lfe[acmod];
	else
		acl->mChannelLayoutTag = ac3_layout_no_lfe[acmod];
	
	return 1;
}

static int parse_mpeg4_extra(FFusionParserContext *parser, const uint8_t *buf, int buf_size)
{
	ParseContext1 *pc1 = (ParseContext1 *)parser->pc->priv_data;
	pc1->pc.frame_start_found = 0;
	
	MpegEncContext *s = pc1->enc;
	GetBitContext gb1, *gb = &gb1;
	
	s->avctx = parser->avctx;
	s->current_picture_ptr = &s->current_picture;
	
	init_get_bits(gb, buf, 8 * buf_size);
	ff_mpeg4_decode_picture_header(s, gb);
	return 1;
}

/*
 * Long story short, FFMpeg's parsers suck for our use.  This function parses an mpeg4 bitstream,
 * and assumes that it is given at least a full frame of data.
 * @param parser A FFusionParserContext structure containg all our info
 * @param buf The buffer to parse
 * @param buf_size Size of the input buffer
 * @param out_buf_size The number of bytes present in the first frame of data
 * @param type The frame Type: FF_*_TYPE
 * @param pts The PTS of the frame
 * @return 1 if a frame is found, 0 otherwise
 */
static int parse_mpeg4_stream(FFusionParserContext *parser, const uint8_t *buf, int buf_size, int *out_buf_size, int *type, int *skippable)
{
	ParseContext1 *pc1 = (ParseContext1 *)parser->pc->priv_data;
	pc1->pc.frame_start_found = 0;
	
	int endOfFrame = ff_mpeg4_find_frame_end(&(pc1->pc), buf, buf_size);
	
	MpegEncContext *s = pc1->enc;
	GetBitContext gb1, *gb = &gb1;
	
	s->avctx = parser->avctx;
	s->current_picture_ptr = &s->current_picture;
	
	init_get_bits(gb, buf, 8 * buf_size);
	if(ff_mpeg4_decode_picture_header(s, gb) != 0)
		return 0;
	
	*type = s->pict_type;
	*skippable = (*type == FF_B_TYPE);
#if 0 /*this was an attempt to figure out the PTS information and detect an out of order P frame before we hit its B frame */
	int64_t *lastPtsPtr = (int64_t *)parser->internalContext;
	int64_t lastPts = *lastPtsPtr;
	int64_t currentPts = s->current_picture.pts;
	
	switch(s->pict_type)
	{
		case FF_I_TYPE:
			*lastPtsPtr = currentPts;
			*precedesAPastFrame = 0;
			break;
		case FF_P_TYPE:
			if(currentPts > lastPts + 1)
				*precedesAPastFrame = 1;
			else
				*precedesAPastFrame = 0;
			*lastPtsPtr = currentPts;
			break;
		case FF_B_TYPE:
			*precedesAPastFrame = 0;
			break;
		default:
			break;
	}
#endif
	
	if(endOfFrame == END_NOT_FOUND)
		*out_buf_size = buf_size;
	else
		*out_buf_size = endOfFrame;
	return 1;
}

static int parse_mpeg12_stream(FFusionParserContext *ffparser, const uint8_t *buf, int buf_size, int *out_buf_size, int *type, int *skippable)
{
	const uint8_t *out_unused;
	int size_unused;
	AVCodecParser *parser = ffparser->pc->parser;
	
	parser->parser_parse(ffparser->pc, ffparser->avctx, &out_unused, &size_unused, buf, buf_size);
	
	*out_buf_size = buf_size;
	*type = ffparser->pc->pict_type;
	*skippable = *type == FF_B_TYPE;
	
	return 1;
}

extern AVCodecParser mpeg4video_parser;

static FFusionParser ffusionMpeg4VideoParser = {
	&mpeg4video_parser,
	0,
	NULL,
	parse_mpeg4_extra,
	parse_mpeg4_stream,
};

extern AVCodecParser mpegvideo_parser;

static FFusionParser ffusionMpeg12VideoParser = {
	&mpegvideo_parser,
	0,
	NULL,
	NULL,
	parse_mpeg12_stream,
};

typedef struct H264ParserContext_s
{
	int is_avc;
	int nal_length_size;
	int prevPts;
	
	int profile_idc;
	int level_idc;
	
	int poc_type;
	int log2_max_frame_num;
	int frame_mbs_only_flag;
	int num_reorder_frames;
	int pic_order_present_flag;

	int log2_max_poc_lsb;
	int poc_msb;
	int prev_poc_lsb;

	int delta_pic_order_always_zero_flag;
	int offset_for_non_ref_pic;
	int num_ref_frames_in_pic_order_cnt_cycle;
	int sum_of_offset_for_ref_frames;
	
	int chroma_format_idc;
	
	int gaps_in_frame_num_value_allowed_flag;
}H264ParserContext;

static int decode_nal(const uint8_t *buf, int buf_size, uint8_t *out_buf, int *out_buf_size, int *type, int *nal_ref_idc)
{
	int i;
	int outIndex = 0;
	int numNulls = 0;
	
	for(i=1; i<buf_size; i++)
	{
		if(buf[i] == 0)
			numNulls++;
		else if(numNulls == 2)
		{
			numNulls = 0;
			if(buf[i] < 3)
			{
				/* end of nal */
				outIndex -= 2;
				break;
			}
			else if(buf[i] == 3)
				/* This is just a simple escape of 0x00 00 03 */
				continue;
		}
		out_buf[outIndex] = buf[i];
		outIndex++;
	}
	
	if(outIndex <= 0)
		return 0;
	
	*type = buf[0] & 0x1f;
	*nal_ref_idc = (buf[0] >> 5) & 0x03;
	*out_buf_size = outIndex;
	return 1;
}

static void skip_scaling_list(GetBitContext *gb, int size){
	int i, next = 8, last = 8;
	
    if(get_bits1(gb)) /* matrix not written, we use the predicted one */
		for(i=0;i<size;i++){
			if(next)
				next = (last + get_se_golomb(gb)) & 0xff;
			if(!i && !next){ /* matrix not written, we use the preset one */
				break;
			}
			last = next ? next : last;
		}
}

static void skip_scaling_matrices(GetBitContext *gb){
    if(get_bits1(gb)){
        skip_scaling_list(gb, 16); // Intra, Y
        skip_scaling_list(gb, 16); // Intra, Cr
        skip_scaling_list(gb, 16); // Intra, Cb
        skip_scaling_list(gb, 16); // Inter, Y
        skip_scaling_list(gb, 16); // Inter, Cr
        skip_scaling_list(gb, 16); // Inter, Cb
		skip_scaling_list(gb, 64);  // Intra, Y
		skip_scaling_list(gb, 64);  // Inter, Y
	}
}

static void skip_hrd_parameters(GetBitContext *gb)
{
	int cpb_cnt_minus1 = get_ue_golomb(gb);
	get_bits(gb, 4);	//bit_rate_scale
	get_bits(gb, 4);	//cpb_size_scale
	int i;
	for(i=0; i<=cpb_cnt_minus1; i++)
	{
		get_ue_golomb(gb);	//bit_rate_value_minus1[i]
		get_ue_golomb(gb);	//cpb_size_value_minus1[i]
		get_bits1(gb);		//cbr_flag[i]
	}
	get_bits(gb, 5);	//initial_cpb_removal_delay_length_minus1
	get_bits(gb, 5);	//cpb_removal_delay_length_minus1
	get_bits(gb, 5);	//dpb_output_delay_length_minus1
	get_bits(gb, 5);	//time_offset_length
}

static void decode_sps(H264ParserContext *context, const uint8_t *buf, int buf_size)
{
	GetBitContext getbit, *gb = &getbit;
	
	init_get_bits(gb, buf, 8 * buf_size);
    context->profile_idc= get_bits(gb, 8);
    get_bits1(gb);		//constraint_set0_flag
    get_bits1(gb);		//constraint_set1_flag
    get_bits1(gb);		//constraint_set2_flag
    get_bits1(gb);		//constraint_set3_flag
    get_bits(gb, 4);	//reserved
	context->level_idc = get_bits(gb, 8);	//level_idc
	get_ue_golomb(gb);	//seq_parameter_set_id
	if(context->profile_idc >= 100)
	{
		context->chroma_format_idc = get_ue_golomb(gb);
		//high profile
		if(context->chroma_format_idc == 3)	//chroma_format_idc
			get_bits1(gb);			//residual_color_transfrom_flag
		get_ue_golomb(gb);			//bit_depth_luma_minus8
		get_ue_golomb(gb);			//bit_depth_chroma_minus8
		get_bits1(gb);				//transform_bypass
		skip_scaling_matrices(gb);
	}
	context->log2_max_frame_num = get_ue_golomb(gb) + 4;
	context->poc_type = get_ue_golomb(gb);
	if(context->poc_type == 0)
		context->log2_max_poc_lsb = get_ue_golomb(gb) + 4;
	else if(context->poc_type == 1)
	{
		int i;
		
		context->delta_pic_order_always_zero_flag = get_bits1(gb);
		context->offset_for_non_ref_pic = get_se_golomb(gb);
		get_se_golomb(gb);	//offset_for_top_to_bottom_field
		context->num_ref_frames_in_pic_order_cnt_cycle = get_ue_golomb(gb);
		context->sum_of_offset_for_ref_frames = 0;
		for(i=0; i<context->num_ref_frames_in_pic_order_cnt_cycle; i++)
			context->sum_of_offset_for_ref_frames += get_se_golomb(gb); //offset_for_ref_frame[i]
	}
	get_ue_golomb(gb);	//num_ref_frames
	context->gaps_in_frame_num_value_allowed_flag = get_bits1(gb);		//gaps_in_frame_num_value_allowed_flag
	get_ue_golomb(gb);	//pic_width_in_mbs_minus1
	get_ue_golomb(gb);	//pic_height_in_map_units_minus1
	int mbs_only = get_bits1(gb);
	context->frame_mbs_only_flag = mbs_only;
#if 0		//This is a test to get num_reorder_frames
	if(!mbs_only)
		get_bits1(gb);	//mb_adaptive_frame_field_flag
	get_bits1(gb);		//direct_8x8_inference_flag
	if(get_bits1(gb))	//frame_cropping_flag
	{
		get_ue_golomb(gb);	//frame_crop_left_offset
		get_ue_golomb(gb);	//frame_crop_right_offset
		get_ue_golomb(gb);	//frame_crop_top_offset
		get_ue_golomb(gb);	//frame_crop_bottom_offset
	}
	if(get_bits1(gb))	//vui_parameters_present_flag
	{
		if(get_bits1(gb))	//aspect_ratio_info_present_flag
		{
			if(get_bits(gb, 8) == 255)	//aspect_ratio_idc
			{
				get_bits(gb, 16);	//sar_width
				get_bits(gb, 16);	//sar_height
			}
		}
		if(get_bits1(gb))	//overscan_info_present_flag
			get_bits1(gb);	//overscan_appropriate_flag
		if(get_bits1(gb))	//video_signal_type_present_flag
		{
			get_bits(gb, 3);	//video_format
			get_bits1(gb);		//video_full_range_flag
			if(get_bits1(gb))	//colour_description_present_flag
			{
				get_bits(gb, 8);	//colour_primaries
				get_bits(gb, 8);	//transfer_characteristics
				get_bits(gb, 8);	//matrix_coefficients
			}
		}
		if(get_bits1(gb))	//chroma_loc_info_present_flag
		{
			get_ue_golomb(gb);	//chroma_sample_loc_type_top_field
			get_ue_golomb(gb);	//chroma_sample_loc_type_bottom_field
		}
		if(get_bits1(gb))	//timing_info_present_flag
		{
			get_bits(gb, 32);	//num_units_in_tick
			get_bits(gb, 32);	//time_scale
			get_bits1(gb);	//fixed_frame_rate_flag
		}
		int nal_hrd_parameters_present_flag = get_bits1(gb);
		if(nal_hrd_parameters_present_flag)
		{
			skip_hrd_parameters(gb);
		}
		int vcl_hrd_parameters_present_flag = get_bits1(gb);
		if(vcl_hrd_parameters_present_flag)
		{
			skip_hrd_parameters(gb);
		}
		if(nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag)
			get_bits1(gb);	//low_delay_hrd_flag
		get_bits1(gb);	//pic_struct_present_flag
		if(get_bits1(gb))	//bitstream_restriction_flag
		{
			get_bits1(gb);	//motion_vectors_over_pic_boundaries_flag
			get_ue_golomb(gb);	//max_bytes_per_pic_denom
			get_ue_golomb(gb);	//max_bits_per_mb_denom
			get_ue_golomb(gb);	//log2_max_mv_length_horizontal
			get_ue_golomb(gb);	//log2_max_mv_length_vertical
			context->num_reorder_frames = get_ue_golomb(gb);
			get_ue_golomb(gb);	//max_dec_frame_buffering			
		}
	}
#endif
}

static void decode_pps(H264ParserContext *context, const uint8_t *buf, int buf_size)
{
	GetBitContext getbit, *gb = &getbit;
	
	init_get_bits(gb, buf, 8 * buf_size);
	get_ue_golomb(gb); //pic_parameter_set_id
	get_ue_golomb(gb); //seq_parameter_set_id
	get_bits1(gb); //entropy_coding_mode_flag
	context->pic_order_present_flag = get_bits1(gb);
}

static int inline decode_slice_header(H264ParserContext *context, const uint8_t *buf, int buf_size, int nal_ref_idc, int nal_type, int just_type, int *type, int *pts)
{
	GetBitContext getbit, *gb = &getbit;
	int slice_type;
	int field_pic_flag = 0;
	int bottom_field_flag = 0;
	int frame_number;
//	static const uint8_t slice_type_map[5] = {FF_P_TYPE, FF_B_TYPE, FF_I_TYPE, FF_SP_TYPE, FF_SI_TYPE};
	static const uint8_t slice_type_map[5] = {FF_P_TYPE, FF_P_TYPE, FF_I_TYPE, FF_SP_TYPE, FF_SI_TYPE};
	
	init_get_bits(gb, buf, 8 * buf_size);
	
	get_ue_golomb(gb);	//first_mb_in_slice
	slice_type = get_ue_golomb(gb);
	if(slice_type > 9)
		return 0;
	
	if(slice_type > 4)
		slice_type -= 5;
	
	*type = slice_type_map[slice_type];
	if(just_type)
		return 1;
	
	get_ue_golomb(gb); //pic_parameter_set_id
	frame_number = get_bits(gb, context->log2_max_frame_num);
	if(!context->frame_mbs_only_flag)
	{
		field_pic_flag = get_bits1(gb);
		if(field_pic_flag)
		{
			bottom_field_flag = get_bits1(gb);
		}
	}
	if(nal_type == 5)
		get_ue_golomb(gb);  //idr_pic_id
	if(context->poc_type == 0)
	{
		int pts_lsb = get_bits(gb, context->log2_max_poc_lsb);
		int delta_pic_order_cnt_bottom = 0;
		int maxPicOrderCntLsb = 1 << context->log2_max_poc_lsb;
		int pic_order_msb;
		
		if(context->pic_order_present_flag && !field_pic_flag)
			delta_pic_order_cnt_bottom = get_se_golomb(gb);
		if((pts_lsb < context->prev_poc_lsb) && (context->prev_poc_lsb - pts_lsb) >= maxPicOrderCntLsb)
			pic_order_msb = context->poc_msb + maxPicOrderCntLsb;
		else if((pts_lsb > context->prev_poc_lsb) && (pts_lsb - context->prev_poc_lsb) > maxPicOrderCntLsb)
			pic_order_msb = context->poc_msb - maxPicOrderCntLsb;
		else
			pic_order_msb = context->poc_msb;
		
		context->poc_msb = pic_order_msb;
		
		*pts = pic_order_msb + pts_lsb;
		if(delta_pic_order_cnt_bottom < 0)
			*pts += delta_pic_order_cnt_bottom;
			
	}
	else if(context->poc_type == 1 && !context->delta_pic_order_always_zero_flag)
	{
		int delta_pic_order_cnt[2] = {0, 0};
		delta_pic_order_cnt[0] = get_se_golomb(gb);
		if(context->pic_order_present_flag && !field_pic_flag)
			delta_pic_order_cnt[1] = get_se_golomb(gb);
		
		int frame_num_offset = 0;  //I think this is wrong, but the pts code isn't used anywhere, so no harm yet and this removes a warning.
		
		int abs_frame_num = 0;
		int num_ref_frames_in_pic_order_cnt_cycle = context->num_ref_frames_in_pic_order_cnt_cycle;
		if(num_ref_frames_in_pic_order_cnt_cycle != 0)
			abs_frame_num = frame_num_offset + frame_number;
		
		if(nal_ref_idc == 0 && abs_frame_num > 0)
			abs_frame_num--;
		
		int expected_delta_per_poc_cycle = context->sum_of_offset_for_ref_frames;
		int expectedpoc = 0;
		if(abs_frame_num > 0)
		{
			int poc_cycle_cnt = (abs_frame_num - 1) / num_ref_frames_in_pic_order_cnt_cycle;
			
			expectedpoc = poc_cycle_cnt * expected_delta_per_poc_cycle + context->sum_of_offset_for_ref_frames;
		}
		
		if(nal_ref_idc == 0)
			expectedpoc = expectedpoc + context->offset_for_non_ref_pic;
		*pts = expectedpoc + delta_pic_order_cnt[0];
	}
	
	return 1;
}

#define NAL_PEEK_SIZE 32

static int inline decode_nals(H264ParserContext *context, const uint8_t *buf, int buf_size, int *type, int *skippable)
{
	int nalsize = 0;
	int buf_index = 0;
	int ret = 0;
	int pts_decoded = 0;
	int lowestType = 20;
	
	*skippable = 1;
	
#if 0 /*this was an attempt to figure out the PTS information and detect an out of order P frame before we hit its B frame */
	if(context->poc_type == 2)
	{
		//decode and display order are the same
		pts_decoded = 1;
		*precedesAPastFrame = 0;
	}
#endif
	
	for(;;)
	{
		if(context->is_avc)
		{
			if(buf_index >= buf_size)
				break;
			nalsize = 0;
			switch (context->nal_length_size) {
				case 1:
					nalsize = buf[buf_index];
					buf_index++;
					break;
				case 2:
					nalsize = (buf[buf_index] << 8) | buf[buf_index+1];
					buf_index += 2;
					break;
				case 3:
					nalsize = (buf[buf_index] << 16) | (buf[buf_index+1] << 8) | buf[buf_index + 2];
					buf_index += 3;
					break;
				case 4:
					nalsize = (buf[buf_index] << 24) | (buf[buf_index+1] << 16) | (buf[buf_index + 2] << 8) | buf[buf_index + 3];
					buf_index += 4;
					break;
				default:
					break;
			}
			if(nalsize <= 1 || nalsize > buf_size)
			{
				if(nalsize == 1)
				{
					buf_index++;
					continue;
				}
				else
					break;
			}
		}
		else
		{
			int num_nuls = 0;
			int start_offset = 0;
			//do start code prefix search
			for(; buf_index < buf_size; buf_index++)
			{
				if(buf[buf_index] == 0)
					num_nuls++;
				else
				{
					if(num_nuls >= 2 && buf[buf_index] == 1)
						break;
					num_nuls = 0;
				}
			}
			start_offset = buf_index + 1;
			//do start code prefix search
			for(buf_index++; buf_index < buf_size; buf_index++)
			{
				if(buf[buf_index] == 0)
				{
					if(num_nuls == 2)
						break;
					num_nuls++;
				}
				else
				{
					if(num_nuls == 2 && buf[buf_index] == 1)
						break;
					num_nuls = 0;
				}
			}
			if(start_offset >= buf_size)
				//no more
				break;
			nalsize = buf_index - start_offset;
			if(buf_index < buf_size)
				//Take off the next NAL's startcode
				nalsize -= 2;
			//skip the start code
			buf_index = start_offset;
		}

		uint8_t partOfNal[NAL_PEEK_SIZE];
		int decodedNalSize, nalType;
		int nal_ref_idc;
		int slice_type = 0;
		
		if(decode_nal(buf + buf_index, FFMIN(nalsize, NAL_PEEK_SIZE), partOfNal, &decodedNalSize, &nalType, &nal_ref_idc))
		{
			int pts = 0;
			if(nalType == 1 || nalType == 2)
			{
				if(decode_slice_header(context, partOfNal, decodedNalSize, nal_ref_idc, nalType, pts_decoded, &slice_type, &pts))
				{
					ret = 1;
					if(slice_type < lowestType)
						lowestType = slice_type;
					if(nal_ref_idc)
						*skippable = 0;
					if(pts_decoded == 0)
					{
						pts_decoded = 1;
						if(pts > context->prevPts)
						{
							if(pts < context->prevPts)
								lowestType = FF_B_TYPE;
							context->prevPts = pts;
						}
					}
				}
				
				// Parser users assume I-frames are IDR-frames
				// but in H.264 they don't have to be.
				// Mark these as P-frames if they effectively are.
				if (lowestType == FF_I_TYPE) lowestType = FF_P_TYPE;
			}
			else if(nalType == 5)
			{
				ret = 1;
#if 0 /*this was an attempt to figure out the PTS information and detect an out of order P frame before we hit its B frame */
				context->prev_poc_lsb = 0;
				context->poc_msb = 0;
				context->prevPts = 0;
				*precedesAPastFrame = 0;
#endif
				*skippable = 0;
				lowestType = FF_I_TYPE;
			}
		}
		buf_index += nalsize;
	}
	if(lowestType != 20)
		*type = lowestType;
	
	return ret;
}

/*
 * This function parses an h.264 bitstream, and assumes that it is given at least a full frame of data.
 * @param parser A FFusionParserContext structure containg all our info
 * @param buf The buffer to parse
 * @param buf_size Size of the input buffer
 * @param out_buf_size The number of bytes present in the first frame of data
 * @param type The frame Type: FF_*_TYPE
 * @param pts The PTS of the frame
 * @return 1 if a frame is found, 0 otherwise
 */
static int parse_h264_stream(FFusionParserContext *parser, const uint8_t *buf, int buf_size, int *out_buf_size, int *type, int *skippable)
{
	int endOfFrame;
	int size = 0;
	const uint8_t *parseBuf = buf;
	int parseSize;

	/*
	 * Somehow figure out of frame type
	 * For our use, a frame with any B slices is a B frame, and then a frame with any P slices is a P frame.
	 * An I frame has only I slices.
	 * I expect this involves a NAL decoder, and then look at the slice types.
	 * Nal is a f(1) always set to 0, u(2) of nal_ref_idc, and then u(5) of nal_unit_type.
	 * Nal types 1, 2 start a non-I frame, and type 5 starts an I frame.  Each start with a slice header.
	 * Slice header has a ue(v) for first_mb_in_slice and then a ue(v) for the slice_type
	 * Slice types 0, 5 are P, 1, 6 are B, 2, 7 are I
	 */
	
	do
	{
		parseBuf = parseBuf + size;
		parseSize = buf_size - size;
		endOfFrame = (parser->parserStructure->avparse->split)(parser->avctx, parseBuf, parseSize);
		if(endOfFrame == 0)
			size = buf_size;
		else
		{
			size += endOfFrame;
			parseSize = endOfFrame;
		}
	}while(decode_nals(parser->internalContext, parseBuf, parseSize, type, skippable) == 0 && size < buf_size);
		
	*out_buf_size = size;
	return 1;
}

static int init_h264_parser(FFusionParserContext *parser)
{
	H264ParserContext *context = parser->internalContext;
	
	context->nal_length_size = 2;
	context->is_avc = 0;
	return 1;
}

static int parse_extra_data_h264(FFusionParserContext *parser, const uint8_t *buf, int buf_size)
{
	H264ParserContext *context = parser->internalContext;
	const uint8_t *cur = buf;
	int count, i, type, ref;
	
	context->is_avc = 1;
	count = *(cur+5) & 0x1f;
	cur += 6;
	for (i=0; i<count; i++)
	{
		int size = AV_RB16(cur);
		int out_size = 0;
		uint8_t *decoded = malloc(size);
		if(decode_nal(cur + 2, size, decoded, &out_size, &type, &ref))
			decode_sps(context, decoded, out_size);
		cur += size + 2;
		free(decoded);
	}
	count = *(cur++);
	for (i=0; i<count; i++)
	{
		int size = AV_RB16(cur);
		int out_size = 0;
		uint8_t *decoded = malloc(size);
		if(decode_nal(cur + 2, size, decoded, &out_size, &type, &ref))
			decode_pps(context, decoded, out_size);
		cur += size + 2;
		free(decoded);
	}
	
	context->nal_length_size = ((*(buf+4)) & 0x03) + 1;
	
	return 1;
}

extern AVCodecParser h264_parser;

static FFusionParser ffusionH264Parser = {
	&h264_parser,
	sizeof(H264ParserContext),
	init_h264_parser,
	parse_extra_data_h264,
	parse_h264_stream,
};

static FFusionParser *ffusionFirstParser = NULL;

void registerFFusionParsers(FFusionParser *parser)
{
    parser->next = ffusionFirstParser;
    ffusionFirstParser = parser;
}

void initFFusionParsers()
{
	static Boolean inited = FALSE;
	int unlock = PerianInitEnter(&inited);
	
	if(!inited)
	{
		inited = TRUE;
		registerFFusionParsers(&ffusionMpeg4VideoParser);
		registerFFusionParsers(&ffusionH264Parser);
		registerFFusionParsers(&ffusionMpeg12VideoParser);
	}
	
	PerianInitExit(unlock);
}

void ffusionParserFree(FFusionParserContext *parser)
{
	AVCodecParser *avparse = parser->parserStructure->avparse;
	
	if(parser->pc)
	{
		if (avparse->parser_close)
			avparse->parser_close(parser->pc);
		av_free(parser->pc->priv_data);
		av_free(parser->pc);
	}
	av_free(parser->avctx);
	av_free(parser->internalContext);
	free(parser);
}

FFusionParserContext *ffusionParserInit(int codec_id)
{
    AVCodecParserContext *s;
    AVCodecParser *parser;
	FFusionParser *ffParser;
    int ret, i;
	
    if(codec_id == CODEC_ID_NONE)
        return NULL;
	
	if (!ffusionFirstParser) initFFusionParsers();
	
    for(ffParser = ffusionFirstParser; ffParser != NULL; ffParser = ffParser->next) {
		parser = ffParser->avparse;
		
		for (i = 0; i < 5; i++)
			if (parser->codec_ids[i] == codec_id)
				goto found;
    }
    return NULL;
found:
	s = av_mallocz(sizeof(AVCodecParserContext));
    if (!s)
        return NULL;
    s->parser = parser;
    s->priv_data = av_mallocz(parser->priv_data_size);
    if (!s->priv_data) {
        av_free(s);
        return NULL;
    }
    if (parser->parser_init) {
        ret = parser->parser_init(s);
        if (ret != 0) {
            av_free(s->priv_data);
            av_free(s);
            return NULL;
        }
    }
    s->fetch_timestamp=1;
	s->flags |= PARSER_FLAG_COMPLETE_FRAMES;
	
	FFusionParserContext *parserContext = malloc(sizeof(FFusionParserContext));
	parserContext->avctx = avcodec_alloc_context();
	parserContext->pc = s;
	parserContext->parserStructure = ffParser;
	if(ffParser->internalContextSize)
		parserContext->internalContext = av_mallocz(ffParser->internalContextSize);
	else
		parserContext->internalContext = NULL;
	if(ffParser->init)
		(ffParser->init)(parserContext);
    return parserContext;
}

/*
 * @param parser FFusionParserContext pointer
 * @param buf The buffer to parse
 * @param buf_size Size of the input buffer
 * @return 1 if successful, 0 otherwise
 */
 int ffusionParseExtraData(FFusionParserContext *parser, const uint8_t *buf, int buf_size)
{
	 if(parser->parserStructure->extra_data)
		 return (parser->parserStructure->extra_data)(parser, buf, buf_size);
	 return 1;	 
}

/*
 * @param parser FFusionParserContext pointer
 * @param buf The buffer to parse
 * @param buf_size Size of the input buffer
 * @param out_buf_size The number of bytes present in the first frame of data
 * @param type The frame Type: FF_*_TYPE
 * @param pts The PTS of the frame
 * @return 1 if a frame is found, 0 otherwise
 */
int ffusionParse(FFusionParserContext *parser, const uint8_t *buf, int buf_size, int *out_buf_size, int *type, int *skippable)
{
	if(parser->parserStructure->parser_parse)
		return (parser->parserStructure->parser_parse)(parser, buf, buf_size, out_buf_size, type, skippable);
	return 0;
}

void ffusionLogDebugInfo(FFusionParserContext *parser, FILE *log)
{
	if (parser) {
		if (parser->parserStructure == &ffusionH264Parser) {
			H264ParserContext *h264parser = parser->internalContext;
			
			Codecprintf(log, "H.264 format: profile %d level %d\n\tis_avc %d\n\tframe_mbs_only %d\n\tchroma_format_idc %d\n\tframe_num_gaps %d\n\tnum_reorder_frames %d\n",
						h264parser->profile_idc, h264parser->level_idc, h264parser->is_avc, h264parser->frame_mbs_only_flag, h264parser->chroma_format_idc, h264parser->gaps_in_frame_num_value_allowed_flag,
						h264parser->num_reorder_frames);
		}
	}
}

FFusionDecodeAbilities ffusionIsParsedVideoDecodable(FFusionParserContext *parser)
{
	if (!parser) return FFUSION_PREFER_DECODE;
	
	if (parser->parserStructure == &ffusionH264Parser) {
		H264ParserContext *h264parser = parser->internalContext;
		FFusionDecodeAbilities ret = FFUSION_PREFER_DECODE;
		
		//QT is bad at high profile
		//and x264 B-pyramid (sps.vui.num_reorder_frames > 1)
		//FIXME x264 will generate pyramid differently soon (core 78+), and that might work better
		if(h264parser->profile_idc < 100 && h264parser->num_reorder_frames < 2 && 
		   !CFPreferencesGetAppBooleanValue(CFSTR("DecodeAllProfiles"), PERIAN_PREF_DOMAIN, NULL))
			ret = FFUSION_PREFER_NOT_DECODE;
		
		//PAFF/MBAFF
		//ffmpeg is ok at this now but we can't test it (not enough AVCHD samples)
		//and the quicktime api for it may or may not actually exist
		if(!h264parser->frame_mbs_only_flag)
			ret = FFUSION_CANNOT_DECODE;
		
		//4:2:2 chroma
		if(h264parser->chroma_format_idc > 1)
			ret = FFUSION_CANNOT_DECODE;
		
		return ret;
	}
	
	return FFUSION_PREFER_DECODE;
}
#ifdef DEBUG_BUILD
//FFMPEG doesn't configure properly (their fault), so define their idoticly undefined symbols.
int ff_epzs_motion_search(MpegEncContext * s, int *mx_ptr, int *my_ptr, int P[10][2], int src_index, int ref_index, int16_t (*last_mv)[2], int ref_mv_scale, int size, int h){return 0;}
int ff_get_mb_score(MpegEncContext * s, int mx, int my, int src_index,int ref_index, int size, int h, int add_rate){return 0;}
int ff_init_me(MpegEncContext *s){return 0;}
int ff_rate_control_init(struct MpegEncContext *s){return 0;}
float ff_rate_estimate_qscale(struct MpegEncContext *s, int dry_run){return 0;}
void ff_write_pass1_stats(struct MpegEncContext *s){}
void h263_encode_init(MpegEncContext *s){}
#endif