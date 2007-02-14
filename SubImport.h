/*
 *  SubImport.h
 *  Perian
 *
 *  Created by David Conrad on 10/12/06.
 *  Copyright 2006 Perian Project. All rights reserved.
 *
 */

#ifndef __SUBIMPORT_H__
#define __SUBIMPORT_H__

#include <QuickTime/QuickTime.h>
#include "CodecIDs.h"

#ifdef __cplusplus
extern "C"
{
#endif

ComponentResult LoadExternalSubtitles(const FSRef *theFile, Movie theMovie);

Track CreatePlaintextSubTrack(Movie theMovie, ImageDescriptionHandle imgDesc, 
                              TimeScale timescale, Handle dataRef, OSType dataRefType, FourCharCode subType, Handle imageExtension, Rect movieBox);

short GetFilenameLanguage(CFStringRef filename);

#ifdef __cplusplus
}
#endif

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>

@interface SubLine : NSObject
{
	@public
	NSString *line;
	unsigned begin_time, end_time;
}
-(id)initWithLine:(NSString*)l start:(unsigned)s end:(unsigned)e;
@end

@interface SubtitleSerializer : NSObject
{
	NSMutableArray *lines, *outpackets;
	BOOL finished;
}
-(void)addLine:(SubLine *)sline;
-(void)setFinished:(BOOL)finished;
-(SubLine*)getSerializedPacket;
@end
#endif

#ifdef __cplusplus
class CXXSubtitleSerializer
{
	void *priv;
	
public:
	CXXSubtitleSerializer();
	~CXXSubtitleSerializer();
	
	void pushLine(const char *line, size_t size, unsigned start, unsigned end);
	void setFinished();
	const char *popPacket(size_t *size, unsigned *start, unsigned *end);
	void release();
	void retain();
	bool empty();
};

class CXXAutoreleasePool
{
	void *pool;
public:
	CXXAutoreleasePool();
	~CXXAutoreleasePool();
};
#endif
#endif
