//
//  SubImport.h
//  SSARender2
//
//  Created by Alexander Strange on 7/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#ifndef __SUBIMPORT_H__
#define __SUBIMPORT_H__

#include <QuickTime/QuickTime.h>

#ifdef __cplusplus
extern "C"
{
#endif

short GetFilenameLanguage(CFStringRef filename);
ComponentResult LoadExternalSubtitles(const FSRef *theFile, Movie theMovie);
Track CreatePlaintextSubTrack(Movie theMovie, ImageDescriptionHandle imgDesc, TimeScale timescale, Handle dataRef, OSType dataRefType, FourCharCode subType, Handle imageExtension, Rect movieBox);

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#import "SubContext.h"

@interface SubLine : NSObject
{
	@public
	NSString *line;
	unsigned begin_time, end_time;
}
-(id)initWithLine:(NSString*)l start:(unsigned)s end:(unsigned)e;
@end

@interface SubSerializer : NSObject
{
	NSMutableArray *lines, *outpackets;
	BOOL finished, write_gap;
}
-(void)addLine:(SubLine *)sline;
-(void)setFinished:(BOOL)finished;
-(SubLine*)getSerializedPacket;
-(BOOL)isEmpty;
@end

extern void SubLoadSSAFromPath(NSString *path, SubContext **meta, SubSerializer **lines, SubRenderer *renderer);
extern void SubLoadSRTFromPath(NSString *path, SubContext **meta, SubSerializer **lines, SubRenderer *renderer);

#endif

#ifdef __cplusplus
}

class CXXSubSerializer
{
	void *priv;
	
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
	void *pool;
public:
	CXXAutoreleasePool();
	~CXXAutoreleasePool();
};
#endif

#endif