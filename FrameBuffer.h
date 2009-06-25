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

/*
 * This file implements a mechanisms for tracking frame data and dependencies
 * as well as yet to be parsed data. It is intented to minimize memcpys
 */

typedef struct FrameData_s FrameData;
typedef struct FFusionData_s FFusionData;

struct FrameData_s
{
	uint8_t			*buffer;		/*!< @brief Compressed frame data*/
	unsigned int	dataSize;		/*!< @brief Size of buffer*/
	long			frameNumber;	/*!< @brief Frame number as counted by QT*/
	short			type;			/*!< @brief FFmpeg frame type*/
	short			skippabble;		/*!< @brief True if this frame is not a reference for any other frames (B Frame)*/
	short			decoded;		/*!< @brief True if this frame is already decoded*/
	FrameData		*prereqFrame;	/*!< @brief The frame's data which must be decoded to fully display this frame */
	FrameData		*nextFrame;		/*!< @brief The next frame to decode if this one is already decoded.  This is for predictive decoding */
	FFusionData		*data;			/*!< @brief Pointer to the data set*/
};

typedef struct DataRingBuffer_s {
} DataRingBuffer;

struct FFusionData_s
{
	FrameData		unparsedFrames;	/*!< @brief buffer storage for data that's not yet parsed (packed data for subsequent frames)*/
	/* private */
	unsigned int	frameSize;		/*!< @brief Size of frames array in elements*/
	unsigned int	frameRead;		/*!< @brief Current frame read index*/
	unsigned int	frameWrite;		/*!< @brief Current frame write index*/
	FrameData		*frames;		/*!< @brief array of frames*/
	unsigned int	ringSize;		/*!< @brief Size of ring buffer in bytes*/
	unsigned int	ringRead;		/*!< @brief Current read byte index*/
	unsigned int	ringWrite;		/*!< @brief Current write byte index*/
	uint8_t			*ringBuffer;	/*!< @brief Ring buffer for all data*/
	FFusionData		*previousData;	/*!< @brief Pointer to previous FFusionData struct that's kept during expansion and freed when it's data isn't used anymore*/
};

/*!
 * @brief Initialize a FFusionData structure
 *
 * @param data The data to initialize
 * @param dataSize The frame count to start with
 * @param bufferSize The ring buffer size to start with
 */
void FFusionDataSetup(FFusionData *data, int dataSize, int bufferSize);

/*!
 * @brief Deinitialize a FFusionData structure
 *
 * @param data The data to deinitialize (data is not freed)
 */
void FFusionDataFree(FFusionData *data);

/*!
 * @brief Sets the ring buffer pointer to the given buffer (no parsing used)
 *
 * @param data The FFusionData
 * @param buffer The buffer to place in data
 * @param bufferSize The size of the buffer
 */
uint8_t *FFusionCreateEntireDataBuffer(FFusionData *data, uint8_t *buffer, int bufferSize);

/*!
 * @brief Append a parsed frame's data
 *
 * @param data The FFusionData
 * @param buffer The frame's data
 * @param bufferSize The size of the buffer
 * @param type The FFmpeg type for this frame
 */
FrameData *FFusionDataAppend(FFusionData *data, uint8_t *buffer, int dataSize, int type);

/*!
 * @brief Sets the unparsed data buffer
 *
 * @param data The FFusionData
 * @param buffer The unparsed data
 * @param bufferSize The size of the buffer
 */
void FFusionDataSetUnparsed(FFusionData *data, uint8_t *buffer, int bufferSize);

void FFusionDataReadUnparsed(FFusionData *data);

/*!
 * @brief Mark a frame as no longer needed
 *
 * @param toData the completed Frame
 */
void FFusionDataMarkRead(FrameData *toData);

/*!
 * @brief Find a frame's data
 *
 * @param data The FFusionData
 * @param frameNumber The frame number as assigned by QT
 * @return The frame data if found, NULL otherwise
 */
FrameData *FFusionDataFind(FFusionData *data, int frameNumber);


/*!
 * @brief Get a frame's prerequisite for decode
 *
 * @param toData the frame to check
 * @return The frame's prereq if exists, NULL otherwise
 */
FrameData *FrameDataCheckPrereq(FrameData *toData);
