/*
 *  DataHandlerCallback.cpp
 *
 *    DataHandlerCallback.cpp - file I/O for libebml.
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

#include "DataHandlerCallback.h"
#include <QuickTime/QuickTime.h>

#include "ebml/Debug.h"
#include "ebml/EbmlConfig.h"

using namespace std;

CRTError::CRTError(int nError, const std::string & Description)
	:std::runtime_error(Description+": "+strerror(nError))
	,Error(Error)
{
}

CRTError::CRTError(const std::string & Description,int nError)
	:std::runtime_error(Description+": "+strerror(nError))
	,Error(Error)
{
}

DataHandlerCallback::DataHandlerCallback(ComponentInstance dataHandler, const open_mode aMode)
{	
	closeHandler = false;
	supportsWideOffsets = true;
	this->dataHandler = NULL;
	mCurrentPosition = 0;
	filesize = 0;
	this->aMode = aMode;
	
	switch (aMode)
	{
		case MODE_READ:
		case MODE_WRITE:
			this->dataHandler = dataHandler;
			break;
			
		default:
			throw 0;
			break;
	}
	
	// figure out if we support wide offesets
	getFileSize();
}

DataHandlerCallback::DataHandlerCallback(Handle dataRef, OSType dataRefType, const open_mode aMode)
{
    ComponentResult err = noErr;
    Component dataHComponent = NULL;
	
	closeHandler = true;
	supportsWideOffsets = true;
	dataHComponent = NULL;
	mCurrentPosition = 0;
	filesize = 0;
	this->aMode = aMode;
	
	if (aMode == MODE_READ)
		dataHComponent = GetDataHandler(dataRef, dataRefType, kDataHCanRead);
	else if (aMode == MODE_WRITE)
		dataHComponent = GetDataHandler(dataRef, dataRefType, kDataHCanWrite);
	else
		throw 0;
	
	err = OpenAComponent(dataHComponent, &dataHandler);
	if (err) {
		throw CRTError("Error opening data handler component", err);
	}
	
	err = DataHSetDataRef(dataHandler, dataRef);
	if (err) {
		throw CRTError("Error setting data handler ref", err);
	}
	
	if (aMode == MODE_READ) {
		err = DataHOpenForRead(dataHandler);
        if (err) {
            throw CRTError("Error opening data handler for read", err);
        }
	} else if (aMode == MODE_WRITE) {
		err = DataHOpenForWrite(dataHandler);
        if (err) {
            throw CRTError("Error opening data handler for write", err);
        }
	} else {
		throw 0;
	}
	
	// figure out if we support wide offesets
	getFileSize();
}


DataHandlerCallback::~DataHandlerCallback() throw()
{
	close();
}



uint32 DataHandlerCallback::read(void *Buffer, size_t Size)
{
	ComponentResult err = noErr;
	
	if (supportsWideOffsets) {
		wide wideOffset = SInt64ToWide(mCurrentPosition);
		
		err = DataHScheduleData64(dataHandler, (Ptr)Buffer, &wideOffset, Size, 0, NULL, NULL);
	} else {
		err = DataHScheduleData(dataHandler, (Ptr)Buffer, mCurrentPosition, Size, 0, NULL, NULL);
	}
	
	if (err) {
		throw CRTError("Error reading data", err);
	}
	mCurrentPosition += Size;
	
	if (mCurrentPosition > filesize) {
		Size -= mCurrentPosition - filesize;
		mCurrentPosition = filesize;
	}
	
	// does QuickTime tell us how much it's read?
	return Size;
}

void DataHandlerCallback::setFilePointer(int64 Offset, LIBEBML_NAMESPACE::seek_mode Mode)
{
	switch ( Mode )
	{
		case SEEK_CUR:
			mCurrentPosition += Offset;
			break;
		case SEEK_END:
			// I think this is what seeking this way does (was ftell(File))
			mCurrentPosition = getFileSize() + Offset;
			break;
		case SEEK_SET:
			mCurrentPosition = Offset;
			break;
	}
}

size_t DataHandlerCallback::write(const void *Buffer, size_t Size)
{
	ComponentResult err = noErr;
	
	if (supportsWideOffsets) {
		wide wideOffset = SInt64ToWide(mCurrentPosition);
		
		err = DataHWrite64(dataHandler, (Ptr)Buffer, &wideOffset, Size, NULL, 0);
	} else {
		err = DataHWrite(dataHandler, (Ptr)Buffer, mCurrentPosition, Size, NULL, 0);
	}
	
	if (err) {
		throw CRTError("Error writing data", err);
	}
	mCurrentPosition += Size;
	
	// does QT tell us how much it writes?
	return Size;
}

uint64 DataHandlerCallback::getFilePointer()
{
	return mCurrentPosition;
}

void DataHandlerCallback::close()
{
	if (closeHandler) {
		if (aMode == MODE_READ)
			DataHCloseForRead(dataHandler);
		else if (aMode == MODE_WRITE)
			DataHCloseForWrite(dataHandler);
		dataHandler = NULL;
	}
}

SInt64 DataHandlerCallback::getFileSize()
{
	ComponentResult err = noErr;
	wide wideFilesize;
	
	if (filesize > 0) 
		return filesize;
	
	err = DataHGetFileSize64(dataHandler, &wideFilesize);
	if (err == noErr) {
		supportsWideOffsets = true;
		filesize = WideToSInt64(wideFilesize);
	} else {
		long size32;
		supportsWideOffsets = false;
		DataHGetFileSize(dataHandler, &size32);
		filesize = size32;
	}
	
	return filesize;
}
