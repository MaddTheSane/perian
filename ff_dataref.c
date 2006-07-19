/*****************************************************************************
*
*  Avi Import Component dataref interface for libavformat
*
*  Copyright(C) 2006 Christoph Naegeli <chn1@mac.com>
*
*  This program is free software ; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation ; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY ; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program ; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*
****************************************************************************/

#include "avformat.h"

#include <QuickTime/QuickTime.h>

struct _dataref_private {
	Handle dataRef;
	OSType dataRefType;
	DataHandler dh;
	int64_t pos;
	int64_t size;
};
typedef struct _dataref_private dataref_private;

/* DataRef Wrapper for QuickTime */

/* !!! THIS FUNCTION ASSUMES h->priv_data IS VALID in contrary to the other open functions
* found in ffmpeg */
static int dataref_open(URLContext *h, const char *filename, int flags)
{
	ComponentResult result;
	dataref_private *private;
	wide fsize;
	long access = 0;
	
	if(flags & URL_RDWR) {
		access = kDataHCanRead | kDataHCanWrite;
	} else if(flags & URL_WRONLY) {
		access = kDataHCanWrite;
	} else {
		access = kDataHCanRead;
	}
	
	private = h->priv_data;
	result = OpenAComponent(GetDataHandler(private->dataRef, private->dataRefType, access), &private->dh);
	require_noerr(result,bail);
	
	result = DataHSetDataRef(private->dh, private->dataRef);
	require_noerr(result,bail);
	
	if(access & kDataHCanRead) {
		result = DataHOpenForRead(private->dh);
		require_noerr(result,bail);
	}
	if(access & kDataHCanWrite) {
		result = DataHOpenForWrite(private->dh);
		require_noerr(result,bail);
	}
	
	private->pos = 0ll;
	private->size = 0ll;
	
	DataHGetFileSize64(private->dh, &fsize);
	private->size = (int64_t)fsize.lo;
	private->size += (int64_t)fsize.hi << 32;
	
bail:
		return result;
} /* dataref_open() */

static int dataref_read(URLContext *h, unsigned char *buf, int size)
{
	int result;
	int64_t read;
	wide offset;
	dataref_private *p = (dataref_private*)h->priv_data;
	
	read = p->size - p->pos;
	read = (read < size) ? read : size;
	
	offset.hi = p->pos >> 32;
	offset.lo = (UInt32)p->pos;
	
	result = DataHScheduleData64(p->dh, (Ptr)buf, &offset, (long)read, 0, NULL, NULL);
	if(result != noErr) {
		read = -1;
		goto bail;
	}
	
	p->pos += read;
	
bail:
		return (int)read;
} /* dataref_read() */

static int dataref_write(URLContext *h, unsigned char *buf, int size)
{
	int result;
	int written = size;
	wide offset;
	dataref_private *p = (dataref_private*)h->priv_data;
	
	offset.hi = p->pos >> 32;
	offset.lo = (UInt32)p->pos;
	
	result = DataHWrite64(p->dh, (Ptr)buf, &offset, size, NULL, 0);
	if(result != noErr) {
		written = -1;
		goto bail;
	}
	
	p->pos += written;
	
bail:
		return written;
} /* dataref_write() */

static offset_t dataref_seek(URLContext *h, offset_t pos, int whence)
{
	dataref_private *p = (dataref_private*)h->priv_data;
	
	switch(whence) {
		case SEEK_SET:
			p->pos = pos;
			break;
		case SEEK_CUR:
			p->pos += pos;
			break;
		case SEEK_END:
			p->pos = p->size + pos;
			break;
		default:
			break;
	}
	
	return p->pos;
} /* dataref_seek() */

static int dataref_close(URLContext *h)
{
	dataref_private *p = (dataref_private*)h->priv_data;
	
	CloseComponent(p->dh);
	p->dh = 0;
	p->pos = 0;
	p->size = 0;
	
	av_free(p);
	h->priv_data = NULL;
	
	return 0;
} /* dataref_close() */

URLProtocol dataref_protocol = {
    "Data Ref",
    dataref_open,
    dataref_read,
    dataref_write,
    dataref_seek,
    dataref_close,
};

/* This is the public function to open bytecontext withs datarefs */
OSStatus url_open_dataref(ByteIOContext *pb, Handle dataRef, OSType dataRefType)
{
	URLContext *uc;
	URLProtocol *up;
	OSStatus err;
	dataref_private *private;
	
	private = av_mallocz(sizeof(dataref_private));
	private->dataRef = dataRef;
	private->dataRefType = dataRefType;
	
	up = &dataref_protocol;
	
	uc = av_mallocz(sizeof(URLContext));
	if(!uc) {
		err = -ENOMEM;
		return err;
	}
	uc->filename[0] = '\0';
	uc->prot = up;
	uc->flags = URL_RDONLY; // we're just using the read access...
	uc->is_streamed = 0; // not streamed...
	uc->max_packet_size = 0; // stream file
	uc->priv_data = private;
	
	err = up->url_open(uc, uc->filename, URL_RDONLY);
	if(err < 0) {
		av_free(uc);
		return err;
	}
	err = url_fdopen(pb, uc);
	if(err < 0) {
		url_close(uc);
		return err;
	}
	
	return noErr;
} /* url_open_dataref() */
