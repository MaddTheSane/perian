/*
 *  bitstream_info.h
 *  Perian
 *
 *  Created by Graham Booker on 1/6/07.
 *  Copyright 2007 Graham Booker. All rights reserved.
 *
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
						int *type, int *skippable);
	struct FFusionParser *next;
} FFusionParser;

void initFFusionParsers();
FFusionParserContext *ffusionParserInit(int codec_id);
int ffusionParseExtraData(FFusionParserContext *parser, const uint8_t *buf, int buf_size);
int ffusionParse(FFusionParserContext *parser, const uint8_t *buf, int buf_size, int *out_buf_size, int *type, int *skippable);
int ffusionIsParsedVideoDecodable(FFusionParserContext *parser);

#ifdef __cplusplus
}
#endif
