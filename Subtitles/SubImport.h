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

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>

@interface SubLine : NSObject
{
@private
	NSString *line;
	NSUInteger begin_time, end_time;
	NSInteger num; // line number, used only by SubSerializer
}
@property (readonly, copy) NSString *line;
@property NSUInteger beginTime;
@property NSUInteger endTime;
@property NSInteger num;

- (instancetype)initWithLine:(NSString*)l start:(unsigned)s end:(unsigned)e;
@end

@interface SubSerializer : NSObject
{
	// input lines, sorted by 1. beginning time 2. original insertion order
@private
	NSMutableArray *lines;
	BOOL finished;
	
	NSUInteger last_begin_time, last_end_time;
	NSInteger num_lines_input;
}

@property (assign) BOOL finished;
@property (readonly, getter = isEmpty) BOOL empty;
@property NSUInteger lastBeginTime;
@property NSUInteger lastEndTime;
@property NSInteger numLinesInput;

-(void)addLine:(SubLine *)sline;
-(SubLine*)getSerializedPacket;
@end

@interface VobSubSample : NSObject
{
@private
	long		timeStamp;
	long		fileOffset;
}
@property long timeStamp;
@property long fileOffset;

- (instancetype)initWithTime:(long)time offset:(long)offset;
@end

@interface VobSubTrack : NSObject <NSFastEnumeration>
{
@private
	NSData			*privateData;
	NSString		*language;
	NSInteger		index;
	NSMutableArray	*samples;
}

@property (retain, readonly) NSData *privateData;
@property (copy) NSString *language;
@property NSInteger index;
@property (assign, readonly) NSArray *samples;

- (instancetype)initWithPrivateData:(NSData *)idxPrivateData language:(NSString *)lang andIndex:(int)trackIndex;
- (void)addSample:(VobSubSample *)sample;
- (void)addSampleTime:(long)time offset:(long)offset;

@end

__BEGIN_DECLS

NSString *SubLoadSSAFromPath(NSString *path, SubSerializer *ss);
void SubLoadSRTFromPath(NSString *path, SubSerializer *ss);
void SubLoadSMIFromPath(NSString *path, SubSerializer *ss, int subCount);

__END_DECLS

#endif // ___OBJC__

__BEGIN_DECLS

#if !__LP64__

short GetFilenameLanguage(CFStringRef filename);
ComponentResult LoadExternalSubtitlesFromFileDataRef(Handle dataRef, OSType dataRefType, Movie theMovie);
void SetSubtitleMediaHandlerTransparent(MediaHandler mh);
Track CreatePlaintextSubTrack(Movie theMovie, ImageDescriptionHandle imgDesc, TimeScale timescale, Handle dataRef, OSType dataRefType, FourCharCode subType, Handle imageExtension, Rect movieBox);

#endif

__END_DECLS

#ifdef __cplusplus
#include <string>

#ifndef __OBJC_GC__
#define __strong
#endif

class CXXSubSerializer
{
	__strong void *priv;
	int retainCount;
	
public:
	CXXSubSerializer();
	~CXXSubSerializer();
	
	void pushLine(const char *line, size_t size, unsigned start, unsigned end);
	void pushLine(const std::string &cppstr, unsigned start, unsigned end);
	void setFinished();
	Handle popPacket(unsigned *start, unsigned *end);
	void release();
	void retain();
	bool empty();
};

#endif // __cplusplus

#endif // __SUBIMPORT_H__