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

void FFusionDataSetup(FFusionData *data, int dataSize, int bufferSize)
{
	data->read = 0;
	data->write = 0;
	data->frames = malloc(sizeof(FrameData *) * dataSize);
	
	int i;
	for(i=0; i<dataSize; i++)
	{
		FrameData *fdata = malloc(sizeof(FrameData));
		data->frames[i] = fdata;
		fdata->buffer = av_malloc(bufferSize);
		fdata->bufferSize = bufferSize;
	}
	data->size = dataSize;
	data->buffer = av_malloc(bufferSize);
	data->bufferSize = bufferSize;
	data->startBufferSize = bufferSize;
}

void FFusionDataFree(FFusionData *data)
{
	int i;
	for(i=0; i<data->size; i++)
	{
		av_free(data->frames[i]->buffer);
		free(data->frames[i]);
	}
	free(data->frames);
	av_free(data->buffer);
}

int FFusionCreateDataBuffer(FFusionData *data, uint8_t *buffer, int bufferSize)
{
	data->buffer = av_fast_realloc(data->buffer, &data->bufferSize, bufferSize + FF_INPUT_BUFFER_PADDING_SIZE);
	if (data->buffer) {
		uint8_t *dataPtr = data->buffer;
		memcpy(dataPtr, buffer, bufferSize);
		memset(dataPtr + bufferSize, 0, FF_INPUT_BUFFER_PADDING_SIZE);
	}
	return data->buffer != NULL;
}

FrameData *FFusionDataAppend(FFusionData *data, int dataSize, int type)
{
	if((data->write + 1) % data->size == data->read)
	{
		int newSize = data->size * 2;
		FrameData * *newData = malloc(sizeof(FrameData *) * newSize);
		memcpy(newData, data->frames + data->read, (data->size - data->read) * sizeof(FrameData));
		if(data->read != 0)
			memcpy(newData + (data->size - data->read), data->frames, data->read * sizeof(FrameData));
		
		int i;
		for(i=data->size; i<newSize; i++)
		{
			FrameData *newFrame = malloc(sizeof(FrameData));
			newData[i] = newFrame;
			newFrame->buffer = av_malloc(data->startBufferSize);
			newFrame->bufferSize = data->startBufferSize;
		}
		data->read = 0;
		data->write = data->size;
		data->size = newSize;
		free(data->frames);
		data->frames = newData;
	}
	
	FrameData *dest = data->frames[data->write];
	uint8_t *tbuff = dest->buffer;
	dest->buffer = data->buffer;
	data->buffer = tbuff;
	
	unsigned int tsize = dest->bufferSize;
	dest->bufferSize = data->bufferSize;
	data->bufferSize = tsize;
	
	dest->dataSize = dataSize;
	dest->type = type;
	dest->prereqFrame = NULL;
	dest->decoded = FALSE;
	dest->nextFrame = NULL;
	
	data->write = (data->write + 1) % data->size;
	return dest;
}

void FFusionDataSetUnparsed(FFusionData *data, uint8_t *buffer, int bufferSize)
{
	FrameData *unparsed = &(data->unparsedFrames);
	
	unparsed->buffer = av_fast_realloc(unparsed->buffer, &unparsed->bufferSize, bufferSize);
	if (unparsed->buffer) {
		memcpy(unparsed->buffer, buffer, bufferSize);
		unparsed->dataSize = bufferSize;
	}
}

FrameData *FrameDataCheckPrereq(FrameData * toData)
{
	FrameData *prereq = toData->prereqFrame;
	if(prereq && prereq->decoded)
		return NULL;
	return prereq;
}

void FFusionDataMarkRead(FFusionData *data, FrameData *toData)
{
	if(toData == NULL)
		return;
	
	if(toData->prereqFrame != NULL && toData->prereqFrame->hold)
		return;
	
	int i;
	for(i=data->read; i!=data->write; i = (i + 1) % data->size)
	{
		if(data->frames[i] == toData)
		{
			data->read = (i + 1) % data->size;
			break;
		}		
	}
}

FrameData *FFusionDataFind(FFusionData *data, int frameNumber)
{
	int i;
	for(i=data->read; i!=data->write; i = (i + 1) % data->size)
	{
		if(data->frames[i]->frameNumber == frameNumber)
			return data->frames[i];
	}
	return NULL;
}
