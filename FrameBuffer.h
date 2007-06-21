/*
 *  FrameBuffer.h
 *  Perian
 *
 *  Created by Graham Booker on 1/30/07.
 *  Copyright 2007 Perian Project. All rights reserved.
 *
 */

typedef struct FrameData_s FrameData;
struct FrameData_s
{
	uint8_t			*buffer;
	unsigned int	dataSize;
	long			frameNumber;
	short			type;
	short			skippabble;
	int				decoded;
	FrameData		*prereqFrame;  /* This is the frame's data which must be decoded to fully display this frame */
	FrameData		*nextFrame; /* This is the next frame to decode if this one is already decoded.  This is for predictive decoding */
	/* private */
	unsigned int	bufferSize;
};

typedef struct
{
	uint8_t			*buffer;
	unsigned int	bufferSize;
	FrameData		unparsedFrames;
	/* private */
	unsigned int	size;
	unsigned int	read;
	unsigned int	write;
	unsigned int	startBufferSize;
	FrameData		* *frames;
} FFusionData;

void FFusionDataSetup(FFusionData *data, int dataSize, int bufferSize);
void FFusionDataFree(FFusionData *data);
int FFusionCreateDataBuffer(FFusionData *data, uint8_t *buffer, int bufferSize);
FrameData *FFusionDataAppend(FFusionData *data, int dataSize, int type);
void FFusionDataSetUnparsed(FFusionData *data, uint8_t *buffer, int bufferSize);
void FFusionDataCheckPrereq(FFusionData *data, FrameData *toData);
void FFusionDataMarkRead(FFusionData *data, FrameData *toData);
FrameData *FFusionDataFind(FFusionData *data, int frameNumber);

