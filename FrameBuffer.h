/*
 * FrameBuffer.h
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
