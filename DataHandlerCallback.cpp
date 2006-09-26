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

#include <sstream>

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
	mDataReader = mDataWriter = NULL;
	
	switch (aMode)
	{
		case MODE_READ:
			mDataReader = dataHandler;
			break;
			
		case MODE_WRITE:
			mDataWriter = dataHandler;
			break;
			
		default:
			throw 0;
			break;
	}
	
	mCurrentPosition = 0;
}

DataHandlerCallback::DataHandlerCallback(Handle dataRef, OSType dataRefType, const open_mode aMode)
{
    ComponentResult err = noErr;
    Component dataHComponent = NULL;
	
	closeHandler = true;
	mDataReader = mDataWriter = NULL;
    
	switch (aMode)
	{
	case MODE_READ:
		dataHComponent = GetDataHandler(dataRef, dataRefType, kDataHCanRead);
        err = OpenAComponent(dataHComponent, &mDataReader);
        if (err) {
            stringstream Msg;
            Msg << "Error opening data handler component " << err;
            throw CRTError(Msg.str());
        }
        
        err = DataHSetDataRef(mDataReader, dataRef);
        if (err) {
            stringstream Msg;
            Msg << "Error setting data handler ref " << err;
            throw CRTError(Msg.str());
        }
        
		err = DataHOpenForRead(mDataReader);
        if (err) {
            stringstream Msg;
            Msg << "Error opening data handler for read " << err;
            throw CRTError(Msg.str());
        }
		
		break;
	case MODE_WRITE:
		dataHComponent = GetDataHandler(dataRef, dataRefType, kDataHCanWrite);
        err = OpenAComponent(dataHComponent, &mDataWriter);
        if (err) {
            stringstream Msg;
            Msg << "Error opening data handler component " << err;
            throw CRTError(Msg.str());
        }
			
		err = DataHSetDataRef(mDataWriter, dataRef);
        if (err) {
            stringstream Msg;
            Msg << "Error setting data handler ref " << err;
            throw CRTError(Msg.str());
        }
			
		err = DataHOpenForWrite(mDataWriter);
        if (err) {
            stringstream Msg;
            Msg << "Error opening data handler for write " << err;
            throw CRTError(Msg.str());
        }
		
		break;
	default:
		throw 0;
	}
	
	mCurrentPosition = 0;
}


DataHandlerCallback::~DataHandlerCallback() throw()
{
	close();
}



uint32 DataHandlerCallback::read(void *Buffer, size_t Size)
{
	assert(mDataReader != 0);
	
	ComponentResult err = noErr;
	wide wideOffset = SInt64ToWide((SInt64) mCurrentPosition);
	
	err = DataHScheduleData64(mDataReader, (Ptr)Buffer, &wideOffset, Size, 0, NULL, NULL);
	if (err) {
		stringstream Msg;
		Msg << "Error reading data " << err;
		throw CRTError(Msg.str(), err);
	}
	mCurrentPosition += Size;
	
	// does QuickTime tell us how much it's read?
	return Size;
}

void DataHandlerCallback::setFilePointer(int64 Offset, LIBEBML_NAMESPACE::seek_mode Mode)
{
	assert(mDataReader != NULL || mDataWriter != NULL);

	assert(Offset <= LONG_MAX);
	assert(Offset >= LONG_MIN);

	assert(Mode==SEEK_CUR||Mode==SEEK_END||Mode==SEEK_SET);
	
	ComponentInstance dataHandler = mDataReader != NULL ? mDataReader : mDataWriter;
	
	switch ( Mode )
	{
		case SEEK_CUR:
			mCurrentPosition += Offset;
			break;
		case SEEK_END:
			// I think this is what seeking this way does (was ftell(File))
			wide filesize;
			DataHGetFileSize64(dataHandler, &filesize);
			mCurrentPosition = WideToSInt64(filesize) + Offset;
			break;
		case SEEK_SET:
			mCurrentPosition = Offset;
			break;
	}
}

size_t DataHandlerCallback::write(const void *Buffer, size_t Size)
{
	assert(mDataWriter != NULL);
	
	ComponentResult err = noErr;
	wide wideOffset = SInt64ToWide((SInt64) mCurrentPosition);
	
	err = DataHWrite64(mDataWriter, (Ptr)Buffer, &wideOffset, Size, NULL, 0);
	if (err) {
		stringstream Msg;
		Msg << "Error writing data " << err;
		throw CRTError(Msg.str(), err);
	}
	mCurrentPosition += Size;
	
	// does QT tell us how much it writes?
	return Size;
}

uint64 DataHandlerCallback::getFilePointer()
{
	assert(mDataReader != NULL || mDataWriter != NULL);

	return mCurrentPosition;
}

void DataHandlerCallback::close()
{
	if (closeHandler) {
		if (mDataReader) {
			DataHCloseForRead(mDataReader);
			mDataReader = NULL;
		}
		if (mDataWriter) {
			DataHCloseForWrite(mDataWriter);
			mDataWriter = NULL;
		}
	}
}

SInt64 DataHandlerCallback::getFileSize()
{
	wide filesize;
	if (mDataReader)
		DataHGetFileSize64(mDataReader,&filesize);
	else if (mDataWriter)
		DataHGetFileSize64(mDataWriter,&filesize);
	
	return WideToSInt64(filesize);
}
