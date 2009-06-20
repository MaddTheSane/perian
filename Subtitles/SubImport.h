/*
 * SubParsing.h
 * Created by Alexander Strange on 7/24/07.
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

#ifndef __SUBIMPORT_H__
#define __SUBIMPORT_H__

#include <QuickTime/QuickTime.h>
#ifndef __OBJC__
#include "SubATSUIRenderer.h"
#endif

#ifndef __OBJC_GC__
#ifndef __strong
#define __strong
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

short GetFilenameLanguage(CFStringRef filename);
ComponentResult LoadExternalSubtitles(const FSRef *theFile, Movie theMovie);
ComponentResult LoadExternalSubtitlesFromFileDataRef(Handle dataRef, OSType dataRefType, Movie theMovie);
Track CreatePlaintextSubTrack(Movie theMovie, ImageDescriptionHandle imgDesc, TimeScale timescale, Handle dataRef, OSType dataRefType, FourCharCode subType, Handle imageExtension, Rect movieBox);

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#import "SubContext.h"
#import "SubATSUIRenderer.h"

@interface SubLine : NSObject
{
	@public
	NSString *line;
	unsigned begin_time, end_time;
	unsigned no; // line number, used only by SubSerializer
}
-(id)initWithLine:(NSString*)l start:(unsigned)s end:(unsigned)e;
@end

@interface SubSerializer : NSObject
{
	// input lines, sorted by 1. beginning time 2. original insertion order
	NSMutableArray *lines;
	BOOL finished;
	
	unsigned last_begin_time, last_end_time;
	unsigned linesInput;
}
-(void)addLine:(SubLine *)sline;
-(void)setFinished:(BOOL)finished;
-(SubLine*)getSerializedPacket;
-(BOOL)isEmpty;
@end

@interface VobSubSample : NSObject
{
	@public
	long		timeStamp;
	long		fileOffset;
}

- (id)initWithTime:(long)time offset:(long)offset;
@end


@interface VobSubTrack : NSObject
{
	@public
	NSData			*privateData;
	NSString		*language;
	int				index;
	NSMutableArray	*samples;
}

- (id)initWithPrivateData:(NSData *)idxPrivateData language:(NSString *)lang andIndex:(int)trackIndex;
- (void)addSample:(VobSubSample *)sample;
- (void)addSampleTime:(long)time offset:(long)offset;

@end

extern NSString *LoadSSAFromPath(NSString *path, SubSerializer *ss);
	
#endif

#ifdef __cplusplus
}

class CXXSubSerializer
{
	__strong void *priv;
	
public:
	CXXSubSerializer();
	~CXXSubSerializer();
	
	void pushLine(const char *line, size_t size, unsigned start, unsigned end);
	void setFinished();
	const char *popPacket(size_t *size, unsigned *start, unsigned *end);
	void release();
	void retain();
	bool empty();
};

class CXXAutoreleasePool
{
	__strong void *pool;
public:
	CXXAutoreleasePool();
	~CXXAutoreleasePool();
};
#endif

#endif