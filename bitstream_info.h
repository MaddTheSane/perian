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

#include <AudioToolbox/AudioToolbox.h>
#include <QuickTime/QuickTime.h>

#ifdef __cplusplus
extern "C" {
#endif

int parse_ac3_bitstream(AudioStreamBasicDescription *asbd, AudioChannelLayout *acl, uint8_t *buffer, int buff_size);

typedef struct FFusionParserContext
{
	struct AVCodecParserContext	*pc;
	struct AVCodecContext	*avctx;
	struct FFusionParser	*parserStructure;
	void					*internalContext;
} FFusionParserContext;
	
typedef struct FFusionParser
{
	struct AVCodecParser *avparse;
	int internalContextSize;
	int (*init)(FFusionParserContext *parser);
	int (*extra_data)(FFusionParserContext *parser,
					  const uint8_t *buf, int buf_size);
	int (*parser_parse)(FFusionParserContext *parser,
						const uint8_t *buf, int buf_size,
						int *out_buf_size,
						int *type, int *skippable, int *skipped);
	struct FFusionParser *next;
} FFusionParser;

typedef enum {
	FFUSION_CANNOT_DECODE,
	FFUSION_PREFER_NOT_DECODE,
	FFUSION_PREFER_DECODE,
} FFusionDecodeAbilities;

void initFFusionParsers();
FFusionParserContext *ffusionParserInit(int codec_id);
void ffusionParserFree(FFusionParserContext *parser);
int ffusionParseExtraData(FFusionParserContext *parser, const uint8_t *buf, int buf_size);
int ffusionParse(FFusionParserContext *parser, const uint8_t *buf, int buf_size, int *out_buf_size, int *type, int *skippable, int *skipped);
void ffusionLogDebugInfo(FFusionParserContext *parser, FILE *log);
FFusionDecodeAbilities ffusionIsParsedVideoDecodable(FFusionParserContext *parser);

#ifdef __cplusplus
}
#endif
