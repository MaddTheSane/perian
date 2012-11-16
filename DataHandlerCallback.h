/*
 *  DataHandlerCallback.h
 *
 *    DataHandlerCallback.h - file I/O for libebml.
 *
 *
 *  Copyright (c) 2006  David Conrad
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; 
 *  version 2.1 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __DATAHANDLERCALLBACK_H__
#define __DATAHANDLERCALLBACK_H__

#include <QuickTime/QuickTime.h>
#include <ebml/IOCallback.h>

#include <stdexcept>
#include <cerrno>
#include <sys/param.h>

using namespace std;
using namespace libebml;

#define READ_SIZE 32*1024

class DataHBuffer {
private:
	uint8_t *buffer;
	size_t allocatedSize;
	uint64_t fileOffset;
	size_t dataSize;
	
	// this clears the buffer
	void Realloc(size_t bufferSize);
	
public:
	DataHBuffer(size_t bufferSize = READ_SIZE);
	
	~DataHBuffer();
	
	bool ContainsOffset(uint64_t offset);
	
	// this returns a pointer to a buffer at least as big as size, and saves
	// the given offset and size of data that will be stored
	uint8_t * GetBuffer(uint64_t offset, size_t size);
	
	// this copies as much data from the buffer as possible from the given file offset,
	// returning the number of bytes actually copied.
	size_t Read(uint64_t offset, size_t size, uint8_t *store);
};

// QuickTime Data Handler callback for libmatroska
class DataHandlerCallback:public IOCallback
{
private:
	ComponentInstance dataHandler;
	uint64 mCurrentPosition;
	bool closeHandler;
	bool supportsWideOffsets;
	open_mode aMode;
	UInt64 filesize;
	DataHBuffer dataBuffer;
	
	void Initialize(const open_mode aMode);
    
public:
	DataHandlerCallback(ComponentInstance dataHandler, const open_mode aMode);
	DataHandlerCallback(Handle dataRef, OSType dataRefType, const open_mode aMode);
	virtual ~DataHandlerCallback() throw();
    
	virtual uint32 read(void *Buffer, size_t Size);
    
	// Seek to the specified position. The mode can have either SEEK_SET, SEEK_CUR
	// or SEEK_END. The callback should return true(1) if the seek operation succeeded
	// or false (0), when the seek fails.
	virtual void setFilePointer(int64 Offset, seek_mode Mode=seek_beginning);
    
	// This callback just works like its read pendant. It returns the number of bytes written.
	virtual size_t write(const void *Buffer, size_t Size);
    
	// Although the position is always positive, the return value of this callback is signed to
	// easily allow negative values for returning errors. When an error occurs, the implementor
	// should return -1 and the file pointer otherwise.
	//
	// If an error occurs, an exception should be thrown.
	virtual uint64 getFilePointer();
    
	// The close callback flushes the file buffers to disk and closes the file. When using the stdio
	// library, this is equivalent to calling fclose. When the close is not successful, an exception
	// should be thrown.
	virtual void close();
	
	SInt64 getFileSize();
};

#endif
