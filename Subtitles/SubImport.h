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
}
-(id)initWithLine:(NSString*)l start:(unsigned)s end:(unsigned)e;
@end

@interface SubSerializer : NSObject
{
	NSMutableArray *lines, *outpackets;
	BOOL finished, write_gap;
	unsigned last_time;
	SubLine *toReturn;
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


extern void SubLoadSSAFromPath(NSString *path, SubContext **meta, SubSerializer **lines, SubRenderer *renderer);
extern void SubLoadSRTFromPath(NSString *path, SubContext **meta, SubSerializer **lines, SubRenderer *renderer);

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