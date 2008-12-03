/*
 *  FrameBuffer.c
 *  Perian
 *
 *  Created by Graham Booker on 1/30/07.
 *  Copyright 2007 Perian Project. All rights reserved.
 *
 */

#include "FrameBuffer.h"
#include "avcodec.h"
#include <sys/param.h>

void FFusionDataSetup(FFusionData *data, int dataSize, int bufferSize)
{
	memset(data, 0, sizeof(FFusionData));
	int framesSize = sizeof(FrameData) * dataSize;
	data->frames = malloc(framesSize);
	memset(data->frames, 0, framesSize);
	
	int i;
	for(i=0; i<dataSize; i++)
	{
		FrameData *fdata = data->frames + i;
		fdata->buffer = NULL;
		fdata->data = data;
	}
	data->frameSize = dataSize;

	data->ringBuffer = av_malloc(bufferSize);
	data->ringSize = bufferSize;
}

void FFusionDataFree(FFusionData *data)
{
	free(data->frames);
	if(data->previousData != NULL)
	{
		FFusionDataFree(data->previousData);
		free(data->previousData);
	}
	av_free(data->ringBuffer);
}

static void expansion(FFusionData *data, int dataSize)
{
	FFusionData *prev = malloc(sizeof(FFusionData));
	memcpy(prev, data, sizeof(FFusionData));
	int i;
	for(i=0; i<data->frameSize; i++)
		data->frames[i].data = prev;
	
	int newRingSize = MAX(dataSize * 10, data->ringSize * 2);
	FFusionDataSetup(data, data->frameSize * 2, newRingSize);
	data->previousData = prev;
}

uint8_t *FFusionCreateEntireDataBuffer(FFusionData *data, uint8_t *buffer, int bufferSize)
{
	data->ringBuffer = av_fast_realloc(data->ringBuffer, &(data->ringSize), bufferSize + FF_INPUT_BUFFER_PADDING_SIZE);
	if (data->ringBuffer) {
		uint8_t *dataPtr = data->ringBuffer;
		memcpy(dataPtr, buffer, bufferSize);
		memset(dataPtr + bufferSize, 0, FF_INPUT_BUFFER_PADDING_SIZE);
	}
	return data->ringBuffer;
}

static uint8_t *createBuffer(FFusionData *data, int dataSize)
{
	if(data->ringWrite >= data->ringRead)
	{
		//Write is after read
		if(data->ringWrite + dataSize + FF_INPUT_BUFFER_PADDING_SIZE < data->ringSize)
		{
			//Found at end
			int offset = data->ringWrite;
			data->ringWrite = offset + dataSize;
			return data->ringBuffer + offset;
		}
		else
			//Can't fit at end, loop
			data->ringWrite = 0;
	}
	if(data->ringWrite + dataSize + FF_INPUT_BUFFER_PADDING_SIZE < data->ringRead)
	{
		//Found at write
		int offset = data->ringWrite;
		data->ringWrite = offset + dataSize;
		return data->ringBuffer + offset;
	}
	else
	{
		expansion(data, dataSize);
		data->ringWrite = dataSize;
		return data->ringBuffer;
	}
}

static uint8_t *insertIntoBuffer(FFusionData *data, uint8_t *buffer, int dataSize)
{
	uint8_t *ret = createBuffer(data, dataSize);
	memcpy(ret, buffer, dataSize);
	memset(ret + dataSize, 0, FF_INPUT_BUFFER_PADDING_SIZE);
	return ret;
}

FrameData *FFusionDataAppend(FFusionData *data, uint8_t *buffer, int dataSize, int type)
{
	if((data->frameWrite + 1) % data->frameSize == data->frameRead)
	{
		expansion(data, dataSize);
	}
	
	FrameData *dest = data->frames + data->frameWrite;
	
	if(data->unparsedFrames.buffer == buffer)
	{		
		//This was an unparsed frame, don't memcpy; it's already in the correct place.
		dest->buffer = buffer;
		data->unparsedFrames.buffer += dataSize;
		data->unparsedFrames.dataSize -= dataSize;
	}
	else
	{
		uint8_t *saveBuffer = insertIntoBuffer(data, buffer, dataSize);
		dest->buffer = saveBuffer;
	}
	dest->dataSize = dataSize;
	dest->type = type;
	dest->prereqFrame = NULL;
	dest->decoded = FALSE;
	dest->nextFrame = NULL;
	
	data->frameWrite = (data->frameWrite + 1) % data->frameSize;
	return dest;
}

void FFusionDataSetUnparsed(FFusionData *data, uint8_t *buffer, int bufferSize)
{
	FrameData *unparsed = &(data->unparsedFrames);
	
	if(unparsed->buffer == buffer)
	{
		//This part was already unparsed; don't memcpy again
		unparsed->dataSize = bufferSize;
	}
	{
		unparsed->buffer = insertIntoBuffer(data, buffer, bufferSize);
		if (unparsed->buffer) {
			memcpy(unparsed->buffer, buffer, bufferSize);
			unparsed->dataSize = bufferSize;
		}		
	}
}

void FFusionDataReadUnparsed(FFusionData *data)
{
	data->ringWrite -= data->unparsedFrames.dataSize;
	data->unparsedFrames.dataSize = 0;
}

FrameData *FrameDataCheckPrereq(FrameData * toData)
{
	FrameData *prereq = toData->prereqFrame;
	if(prereq && prereq->decoded)
		return NULL;
	return prereq;
}

void FFusionDataMarkRead(FrameData *toData)
{
	if(toData == NULL)
		return;
	
	if(toData->prereqFrame != NULL)
		return;
	
	FFusionData *data = toData->data;
	data->frameRead = toData - data->frames + 1;
	data->ringRead = toData->buffer + toData->dataSize - data->ringBuffer;
	if(data->previousData != NULL)
	{
		FFusionDataFree(data->previousData);
		data->previousData = NULL;
	}
}

FrameData *FFusionDataFind(FFusionData *data, int frameNumber)
{
	int i;
	for(i=data->frameRead; i!=data->frameWrite; i = (i + 1) % data->frameSize)
	{
		if(data->frames[i].frameNumber == frameNumber)
			return data->frames + i;
	}
	if(data->previousData != NULL)
		return FFusionDataFind(data->previousData, frameNumber);
	
	return NULL;
}
