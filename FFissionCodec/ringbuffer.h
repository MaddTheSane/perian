/*
 *  ringbuffer.h
 *
 *    RingBuffer class definition. Simple ring buffer implementation.
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
 *  Last modified: $Id: ringbuffer.h 12356 2007-01-20 00:18:04Z arek $
 *
 */

#if !defined(__ringbuffer_h__)
#define __ringbuffer_h__

#include <list>

using namespace std;

class RingBuffer
{
 public:
    RingBuffer();
    virtual ~RingBuffer();

 public:
    virtual void   Initialize(UInt32 inBufferByteSize);
    virtual void   Uninitialize();
    virtual void   Reset();
    virtual UInt32 Reallocate(UInt32 inBufferByteSize);

    virtual UInt32 GetBufferByteSize() const;
    virtual UInt32 GetDataAvailable() const;
    virtual UInt32 GetSpaceAvailable() const;

	// packet sizes are considered to be the same as each call to In()
	virtual UInt32 GetCurrentPacketSize() const { return packetSizes.front(); }
	virtual UInt32 GetNumPackets() const { return packetSizes.size(); }

	// reallocates the buffer as needed
    virtual void   In(const void* data, UInt32& ioBytes);
    virtual void   Zap(UInt32 inBytes);

    virtual Byte * GetData();
    virtual Byte * GetDataEnd();

 protected:
    Byte *  mBuffer;

    UInt32  mBStart;
    UInt32  mBEnd;

    UInt32  mBSize;

    Boolean mNeedsWrapping;

	list<UInt32> packetSizes;
};


#endif /* __ringbuffer_h__ */
