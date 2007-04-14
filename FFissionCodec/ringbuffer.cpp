/*
 *  ringbuffer.cpp
 *
 *    RingBuffer class implementation. Simple ring buffer functionality
 *    expressed as a c++ class.
 *
 *
 *  Copyright (c) 2005,2007  Arek Korbik
 *
 *  This file is part of XiphQT, the Xiph QuickTime Components.
 *
 *  XiphQT is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  XiphQT is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with XiphQT; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  Last modified: $Id: ringbuffer.cpp 12356 2007-01-20 00:18:04Z arek $
 *
 */

#include "ringbuffer.h"

RingBuffer::RingBuffer() :
mBuffer(NULL),
mBStart(0),
mBEnd(0),
mBSize(0),
mNeedsWrapping(false)
{
}

RingBuffer::~RingBuffer() {
    delete[] mBuffer;
}


void RingBuffer::Initialize(UInt32 inBufferByteSize) {
    mBSize = inBufferByteSize;

    if (mBuffer)
        delete[] mBuffer;

    mBuffer = new Byte[mBSize * 2];

    mBStart = 0;
    mBEnd = 0;
    mNeedsWrapping = false;
	packetSizes.clear();
}

void RingBuffer::Uninitialize() {
    mBSize = 0;

    if (mBuffer) {
        delete[] mBuffer;
        mBuffer = NULL;
    }

    Reset();

}

void RingBuffer::Reset() {
    mBStart = 0;
    mBEnd = 0;
    mNeedsWrapping = false;
	packetSizes.clear();
}

UInt32 RingBuffer::Reallocate(UInt32 inBufferByteSize) {
    Byte *bptr = NULL;
    UInt32 data_size = 0;

    // can't decrease the size at the moment
    if (inBufferByteSize > mBSize) {
        bptr = new Byte[inBufferByteSize * 2];
        data_size = GetDataAvailable();
        if (mNeedsWrapping) {
            UInt32 headBytes = mBSize - mBStart;
            BlockMoveData(mBuffer + mBStart, bptr, headBytes);
            BlockMoveData(mBuffer, bptr + headBytes, mBEnd);
            mNeedsWrapping = false;
        } else {
            BlockMoveData(mBuffer + mBStart, bptr, data_size);
        }
        mBEnd = data_size;
        mBStart = 0;

        delete[] mBuffer;
        mBuffer = bptr;
        mBSize = inBufferByteSize;
    }

    return mBSize;
}

UInt32 RingBuffer::GetBufferByteSize() const {
    return mBSize;
}

UInt32 RingBuffer::GetDataAvailable() const {
    UInt32 ret = 0;

    if (mBStart < mBEnd)
        ret = mBEnd - mBStart;
    else if (mBEnd < mBStart)
        ret = mBSize + mBEnd - mBStart;

    return ret;
}

UInt32 RingBuffer::GetSpaceAvailable() const {
    UInt32 ret = mBSize;

    if (mBStart > mBEnd)
        ret =  mBStart - mBEnd;
    else if (mBEnd > mBStart)
        ret = mBSize - mBEnd + mBStart;

    return ret;

}


void RingBuffer::In(const void* data, UInt32& ioBytes) {
	UInt32 copiedBytes = ioBytes;

	UInt32 sizeNeeded = GetDataAvailable() + ioBytes;
	if (sizeNeeded > GetBufferByteSize())
		if (Reallocate(sizeNeeded) < sizeNeeded)
			copiedBytes = GetSpaceAvailable();

    if (mBEnd + copiedBytes <= mBSize) {
        BlockMoveData(data, mBuffer + mBEnd, copiedBytes);
        mBEnd += copiedBytes;
        if (mBEnd < mBStart)
            mNeedsWrapping = true;
    } else {
        UInt32 wrappedBytes = mBSize - mBEnd;
        const Byte* dataSplit = static_cast<const Byte*>(data) + wrappedBytes;
        BlockMoveData(data, mBuffer + mBEnd, wrappedBytes);

        mBEnd = copiedBytes - wrappedBytes;
        BlockMoveData(dataSplit, mBuffer, mBEnd);

        mNeedsWrapping = true;
    }

    ioBytes -= copiedBytes;
	packetSizes.push_back(copiedBytes);
}

void RingBuffer::Zap(UInt32 inBytes) {
	UInt32 packetsZapped = 0;
	while (packetsZapped < inBytes) {
		if (packetsZapped + packetSizes.front() > inBytes) {
			packetSizes.front() -= inBytes - packetsZapped;
			packetsZapped = inBytes;
		} else {
			packetsZapped += packetSizes.front();
			packetSizes.pop_front();
		}
	}
	
    if (inBytes >= GetDataAvailable()) {
        mBStart = 0;
        mBEnd = 0;
        mNeedsWrapping = false;
    } else if (mBStart < mBEnd || mBStart + inBytes < mBSize) {
        mBStart += inBytes;
    } else {
        mBStart += inBytes - mBSize;
        mNeedsWrapping = false;
    }
}

Byte* RingBuffer::GetData() {
    if (GetDataAvailable() == 0)
        return mBuffer;
    else {
        if (mNeedsWrapping) {
            BlockMoveData(mBuffer, mBuffer + mBSize, mBEnd);
            mNeedsWrapping = false;
        }
        return mBuffer + mBStart;
    }
}

Byte* RingBuffer::GetDataEnd() {
    UInt32 available = GetDataAvailable();
    if (available == 0)
        return mBuffer;
    else {
        if (mNeedsWrapping) {
            BlockMoveData(mBuffer, mBuffer + mBSize, mBEnd);
            mNeedsWrapping = false;
        }
        return mBuffer + mBStart + available;
    }
}
