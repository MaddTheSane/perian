/*
 * FrameBuffer.c
 * Created by Graham Booker on 1/30/07.
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

#include "FrameBuffer.h"
#include "avcodec.h"
#include "CommonUtils.h"
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

//Expands both the frame array and the ring buffer.
//Old data is kept in the previousData pointer to be dealloced when done
static void expansion(FFusionData *data, int dataSize)
{
	//Create the prev structure to hold all existing frames
	FFusionData *prev = malloc(sizeof(FFusionData));
	//Move all frames to prev
	memcpy(prev, data, sizeof(FFusionData));
	int i;
	for(i=0; i<data->frameSize; i++)
		data->frames[i].data = prev;
	
	//Create new data
	int newRingSize = MAX(dataSize * 10, data->ringSize * 2);
	FFusionDataSetup(data, data->frameSize * 2, newRingSize);
	//Preserve pointer to old data
	data->previousData = prev;
}

uint8_t *FFusionCreateEntireDataBuffer(FFusionData *data, uint8_t *buffer, int bufferSize)
{
	data->ringBuffer = fast_realloc_with_padding(data->ringBuffer, &data->ringSize, bufferSize);
	if (data->ringBuffer) {
		memcpy(data->ringBuffer, buffer, bufferSize);
	}
	return data->ringBuffer;
}

//Find dataSize bytes in ringbuffer, expand if none available
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

//Insert buffer into ring buffer
static uint8_t *insertIntoBuffer(FFusionData *data, uint8_t *buffer, int dataSize)
{
	uint8_t *ret = createBuffer(data, dataSize);
	memcpy(ret, buffer, dataSize);
	memset(ret + dataSize, 0, FF_INPUT_BUFFER_PADDING_SIZE);
	return ret;
}

FrameData *FFusionDataAppend(FFusionData *data, uint8_t *buffer, int dataSize, int type)
{
	//Find an available frame
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
	else
	{
		unparsed->buffer = insertIntoBuffer(data, buffer, bufferSize);
		if (unparsed->buffer) {
			unparsed->dataSize = bufferSize;
		}		
	}
}

//Seems to be unused
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
		//If there's previous data, free it since we are now done with it
		FFusionDataFree(data->previousData);
		free(data->previousData);
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
		//Check previous data as well
		return FFusionDataFind(data->previousData, frameNumber);
	
	return NULL;
}
