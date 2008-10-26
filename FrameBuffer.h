/*
 *  FrameBuffer.h
 *  Perian
 *
 *  Created by Graham Booker on 1/30/07.
 *  Copyright 2007 Perian Project. All rights reserved.
 *
 */

typedef struct FrameData_s FrameData;
typedef struct FFusionData_s FFusionData;

struct FrameData_s
{
	uint8_t			*buffer;
	unsigned int	dataSize;
	long			frameNumber;
	short			type;
	short			skippabble;
	short			decoded;
	short			hold;
	FrameData		*prereqFrame;  /* This is the frame's data which must be decoded to fully display this frame */
	FrameData		*nextFrame; /* This is the next frame to decode if this one is already decoded.  This is for predictive decoding */
	FFusionData		*data;
};

typedef struct DataRingBuffer_s {
} DataRingBuffer;

struct FFusionData_s
{
	FrameData		unparsedFrames;
	/* private */
	unsigned int	frameSize;
	unsigned int	frameRead;
	unsigned int	frameWrite;
	FrameData		*frames;
	unsigned int	ringSize;
	unsigned int	ringRead;
	unsigned int	ringWrite;
	uint8_t			*ringBuffer;
	FFusionData		*previousData;
};

void FFusionDataSetup(FFusionData *data, int dataSize, int bufferSize);
void FFusionDataFree(FFusionData *data);
uint8_t *FFusionCreateEntireDataBuffer(FFusionData *data, uint8_t *buffer, int bufferSize);
FrameData *FFusionDataAppend(FFusionData *data, uint8_t *buffer, int dataSize, int type);
void FFusionDataSetUnparsed(FFusionData *data, uint8_t *buffer, int bufferSize);
void FFusionDataMarkRead(FrameData *toData);
FrameData *FFusionDataFind(FFusionData *data, int frameNumber);

FrameData *FrameDataCheckPrereq(FrameData *toData);
