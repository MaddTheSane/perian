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

       // ------------------------------------------------

class CRTError:public std::runtime_error
{
    // Variables...
private:
	int Error;
    
    // Methods...
public:
	CRTError(int Error, const std::string &Description);
	CRTError(const std::string &Description, int Error=errno);
    
	int getError() const throw() {return Error;}
};


// QuickTime Data Handler callback for libmatroska
class DataHandlerCallback:public LIBEBML_NAMESPACE::IOCallback
{
private:
	ComponentInstance dataHandler;
	uint64 mCurrentPosition;
	bool closeHandler;
	bool supportsWideOffsets;
	open_mode aMode;
	SInt64 filesize;
    
public:
	DataHandlerCallback::DataHandlerCallback(ComponentInstance dataHandler, const open_mode aMode);
    DataHandlerCallback::DataHandlerCallback(Handle dataRef, OSType dataRefType, const open_mode aMode);
	virtual ~DataHandlerCallback() throw();
    
	virtual uint32 read(void *Buffer, size_t Size);
    
	// Seek to the specified position. The mode can have either SEEK_SET, SEEK_CUR
	// or SEEK_END. The callback should return true(1) if the seek operation succeeded
	// or false (0), when the seek fails.
	virtual void setFilePointer(int64 Offset, LIBEBML_NAMESPACE::seek_mode Mode=LIBEBML_NAMESPACE::seek_beginning);
    
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
