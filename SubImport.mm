/*
 *  SubImport.c
 *  Perian
 *
 *  Created by David Conrad on 10/12/06.
 *  Copyright 2006 Perian Project. All rights reserved.
 *
 */

#include <QuickTime/QuickTime.h>
#import "SubImport.h"
#import "Categories.h"
#include "CommonUtils.h"

//#define SS_DEBUG

// if the subtitle filename is something like title.en.srt or movie.fre.srt
// this function detects it and returns the subtitle language
short GetFilenameLanguage(CFStringRef filename)
{
	CFRange findResult;
	CFStringRef baseName = NULL;
	CFStringRef langStr = NULL;
	short lang = langUnspecified;

	// find and strip the extension
	findResult = CFStringFind(filename, CFSTR("."), kCFCompareBackwards);
	findResult.length = findResult.location;
	findResult.location = 0;
	baseName = CFStringCreateWithSubstring(NULL, filename, findResult);

	// then find the previous period
	findResult = CFStringFind(baseName, CFSTR("."), kCFCompareBackwards);
	findResult.location++;
	findResult.length = CFStringGetLength(baseName) - findResult.location;

	// check for 3 char language code
	if (findResult.length == 3) {
		char langCStr[4] = "";

		langStr = CFStringCreateWithSubstring(NULL, baseName, findResult);
		CFStringGetCString(langStr, langCStr, 4, kCFStringEncodingASCII);
		lang = ISO639_2ToQTLangCode(langCStr);

		CFRelease(langStr);
	
	// and for a 2 char language code
	} else if (findResult.length == 2) {
		char langCStr[3] = "";
		
		langStr = CFStringCreateWithSubstring(NULL, baseName, findResult);
		CFStringGetCString(langStr, langCStr, 3, kCFStringEncodingASCII);
		lang = ISO639_1ToQTLangCode(langCStr);

		CFRelease(langStr);
	}
	
	CFRelease(baseName);
	return lang;
}

Track CreatePlaintextSubTrack(Movie theMovie, ImageDescriptionHandle imgDesc, 
                              TimeScale timescale, Handle dataRef, OSType dataRefType, FourCharCode subType, Handle imageExtension, Rect movieBox)
{
	Fixed trackWidth, trackHeight;
	Track theTrack;
	Media theMedia;

	// plain text subs have no size on their own
	trackWidth = IntToFixed(movieBox.right - movieBox.left);
	trackHeight = IntToFixed(movieBox.bottom - movieBox.top);

	(*imgDesc)->idSize = sizeof(ImageDescription);
	(*imgDesc)->cType = subType;
	(*imgDesc)->width = FixedToInt(trackWidth);
	(*imgDesc)->height = FixedToInt(trackHeight);
	(*imgDesc)->frameCount = 1;
	(*imgDesc)->depth = 32;
	(*imgDesc)->clutID = -1;

	if (imageExtension) AddImageDescriptionExtension(imgDesc, imageExtension, subType);

	theTrack = NewMovieTrack(theMovie, trackWidth, trackHeight, kNoVolume);
	if (theTrack != NULL) {
		theMedia = NewTrackMedia(theTrack, VideoMediaType, timescale, dataRef, dataRefType);

		if (theMedia != NULL) {
			// finally, say that we're transparent
			MediaHandler mh = GetMediaHandler(theMedia);
			MediaSetGraphicsMode(mh, graphicsModePreBlackAlpha, NULL);
			
			// subtitle tracks should be above the video track, which should be layer 0
			SetTrackLayer(theTrack, -1);
		} else {
			DisposeMovieTrack(theTrack);
			theTrack = NULL;
		}
	}

	return theTrack;
}

extern "C" ComponentResult LoadSubStationAlphaSubtitles(const FSRef *theDirectory, CFStringRef filename, Movie theMovie, Track *firstSubTrack);

static ComponentResult ReadSRTFile(NSString *srt, SampleDescriptionHandle desc, Track theTrack, TimeScale movieTimeScale)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	SubtitleSerializer *serializer = [[SubtitleSerializer alloc] init];
	ComponentResult err = noErr;
	NSScanner *sc = [NSScanner scannerWithString:srt];
	NSString *res=nil;
	SubLine *sl;
	Media theMedia = GetTrackMedia(theTrack);
	Handle sampleHndl;
	int index;
	int h,m,s,ms;
	TimeValue start = 0, end = 0;
	enum {
		INITIAL,
		TIMESTAMP,
		LINES
	} state = INITIAL;
	
	[sc setCharactersToBeSkipped:nil];
	
	do {
		switch (state) {
			case INITIAL:
				if ([sc scanInt:&index] == TRUE && [sc scanUpToString:@"\n" intoString:&res] == FALSE) {
					state = TIMESTAMP;
					[sc scanString:@"\n" intoString:nil];
				} else
					[sc setScanLocation:[sc scanLocation]+1];
				break;
			case TIMESTAMP:
				h = [sc scanInt]; [sc scanString:@":" intoString:nil];
				m = [sc scanInt]; [sc scanString:@":" intoString:nil];				
				s = [sc scanInt]; [sc scanString:@"," intoString:nil];				
				ms = [sc scanInt]; [sc scanString:@" --> " intoString:nil];
				start = ms + s*1000 + m*1000*60 + h*1000*60*60;
				h = [sc scanInt]; [sc scanString:@":" intoString:nil];
				m = [sc scanInt]; [sc scanString:@":" intoString:nil];				
				s = [sc scanInt]; [sc scanString:@"," intoString:nil];				
				ms = [sc scanInt]; [sc scanString:@"\n" intoString:nil];	
				end = ms + s*1000 + m*1000*60 + h*1000*60*60;
				state = LINES;
				break;
			case LINES:
				[sc scanUpToString:@"\n\n" intoString:&res];
				[sc scanString:@"\n\n" intoString:nil];
				
				sl = [[SubLine alloc] initWithLine:res start:start end:end];
				[sl autorelease];
				
				[serializer addLine:sl];
				state = INITIAL;
				break;
		}
	} while (![sc isAtEnd]);
	
	[serializer setFinished:YES];
	
	BeginMediaEdits(theMedia);
	
	while (![serializer isEmpty]) {
		sl = [serializer getSerializedPacket];
		sampleHndl = NewHandle(0);
		TimeRecord movieStartTime = {SInt64ToWide(sl->begin_time), 1000, 0};
		TimeValue sampleTime;
		const char *str = [sl->line UTF8String];
		size_t sampleLen = strlen(str);
		
		PtrToHand(str,&sampleHndl,sampleLen);
		
		err=AddMediaSample(theMedia,sampleHndl,0,sampleLen, sl->end_time - sl->begin_time, desc, 1, 0, &sampleTime);
		if (err != noErr) goto bail;
		
		ConvertTimeScale(&movieStartTime, movieTimeScale);
		
		err = InsertMediaIntoTrack(theTrack, movieStartTime.value.lo, sampleTime, sl->end_time - sl->begin_time, fixed1);
		if (err != noErr) goto bail;
		
		DisposeHandle(sampleHndl);
		sampleHndl = NULL;
	}
	
	EndMediaEdits(theMedia);
	
bail:
	[serializer release];
	[pool release];
	if (sampleHndl) DisposeHandle(sampleHndl);
	return err;
}

ComponentResult LoadSubRipSubtitles(const FSRef *theDirectory, CFStringRef filename, Movie theMovie, Track *firstSubTrack)
{
	ComponentResult err = noErr;
	Handle dataRef = NULL;
	Track theTrack = NULL;
	Media theMedia = NULL;
	Rect movieBox;
	TimeScale movieTimeScale = GetMovieTimeScale(theMovie);
	UInt32 emptyDataRefExtension[2];

	ImageDescriptionHandle textDesc = (ImageDescriptionHandle) NewHandleClear(sizeof(ImageDescription));
	UInt8 *path = (UInt8*)malloc(PATH_MAX);
	NSString *srtfile, *srt;
	
	FSRefMakePath(theDirectory, path, PATH_MAX);
	srtfile = [[NSString stringWithUTF8String:(char*)path] stringByAppendingPathComponent:(NSString*)filename];
	free(path);
	
	srt = [[NSString stringFromUnknownEncodingFile:srtfile] stringByStandardizingNewlines];
	if (!srt) {err = -1; goto bail;}

	dataRef = NewHandleClear(sizeof(Handle) + 1);
	emptyDataRefExtension[0] = EndianU32_NtoB(sizeof(UInt32)*2);
	emptyDataRefExtension[1] = EndianU32_NtoB(kDataRefExtensionInitializationData);
	
	PtrAndHand(&emptyDataRefExtension[0], dataRef, sizeof(emptyDataRefExtension));
	
	GetMovieBox(theMovie, &movieBox);

	theTrack = CreatePlaintextSubTrack(theMovie, textDesc, 1000, dataRef, HandleDataHandlerSubType, kSubFormatUTF8, NULL, movieBox);
	if (theTrack == NULL) {
		err = GetMoviesError();
		goto bail;
	}

	theMedia = GetTrackMedia(theTrack);
	if (theMedia == NULL) {
		err = GetMoviesError();
		goto bail;
	}

	err = ReadSRTFile(srt, (SampleDescriptionHandle)textDesc, theTrack, movieTimeScale);

	if (*firstSubTrack == NULL)
		*firstSubTrack = theTrack;
	else
		SetTrackAlternate(*firstSubTrack, theTrack);

	SetMediaLanguage(theMedia, GetFilenameLanguage(filename));

bail:
	if (err) {
		if (theMedia)
			DisposeTrackMedia(theMedia);

		if (theTrack)
			DisposeMovieTrack(theTrack);
	}

	if (textDesc)
		DisposeHandle((Handle) textDesc);

	DisposeHandle(dataRef);

	return err;
}

ComponentResult LoadVobSubSubtitles(const FSRef *theDirectory, CFStringRef filename, Movie theMovie, Track *firstSubTrack)
{
	ComponentResult err = noErr;
	
	return err;
}

ComponentResult LoadExternalSubtitles(const FSRef *theFile, Movie theMovie)
{
	ComponentResult err = noErr;
	Track firstSubTrack = NULL;
	HFSUniStr255 hfsFilename;
	CFStringRef cfFilename = NULL;
	FSRef parentDir;
	FSIterator dirItr = NULL;
	CFRange baseFilenameRange;
	ItemCount filesFound;
	Boolean containerChanged;

	err = FSGetCatalogInfo(theFile, kFSCatInfoNone, NULL, &hfsFilename, NULL, &parentDir);
	if (err) goto bail;

	// find the location of the extension
	cfFilename = CFStringCreateWithCharacters(NULL, hfsFilename.unicode, hfsFilename.length);
	baseFilenameRange = CFStringFind(cfFilename, CFSTR("."), kCFCompareBackwards);

	// strip the extension
	if (baseFilenameRange.location != kCFNotFound) {
		CFStringRef temp = cfFilename;
		baseFilenameRange.length = baseFilenameRange.location;
		baseFilenameRange.location = 0;
		cfFilename = CFStringCreateWithSubstring(NULL, temp, baseFilenameRange);
		CFRelease(temp);
	}

	err = FSOpenIterator(&parentDir, kFSIterateFlat, &dirItr);
	if (err) goto bail;

	do {
		FSRef foundFileRef;
		HFSUniStr255 hfsFoundFilename;
		CFStringRef cfFoundFilename = NULL;
		CFComparisonResult cmpRes;

		err = FSGetCatalogInfoBulk(dirItr, 1, &filesFound, &containerChanged, kFSCatInfoNone, 
				                   NULL, &foundFileRef, NULL, &hfsFoundFilename);
		if (err) goto bail;

		cfFoundFilename = CFStringCreateWithCharacters(NULL, hfsFoundFilename.unicode, hfsFoundFilename.length);
		cmpRes = CFStringCompareWithOptions(cfFoundFilename, cfFilename, baseFilenameRange, kCFCompareCaseInsensitive);

		// two files share the same base, so now check the extension of the found file
		if (cmpRes == kCFCompareEqualTo) {
			CFRange extRange = { CFStringGetLength(cfFoundFilename) - 4, 4 };
			CFRange actRange;

			// SubRip
			actRange = CFStringFind(cfFoundFilename, CFSTR(".srt"), kCFCompareCaseInsensitive | kCFCompareBackwards);
			if (actRange.location == extRange.location)
				err = LoadSubRipSubtitles(&parentDir, cfFoundFilename, theMovie, &firstSubTrack);
			
			// SubStationAlpha
			actRange = CFStringFind(cfFoundFilename, CFSTR(".ass"), kCFCompareCaseInsensitive | kCFCompareBackwards);
			if (actRange.location == extRange.location)
				err = LoadSubStationAlphaSubtitles(&parentDir, cfFoundFilename, theMovie, &firstSubTrack);
			
			actRange = CFStringFind(cfFoundFilename, CFSTR(".ssa"), kCFCompareCaseInsensitive | kCFCompareBackwards);
			if (actRange.location == extRange.location)
				err = LoadSubStationAlphaSubtitles(&parentDir, cfFoundFilename, theMovie, &firstSubTrack);
			
			// VobSub
			actRange = CFStringFind(cfFoundFilename, CFSTR(".idx"), kCFCompareCaseInsensitive | kCFCompareBackwards);
			if (actRange.location == extRange.location)
				err = LoadVobSubSubtitles(&parentDir, cfFoundFilename, theMovie, &firstSubTrack);
			
			if (err) goto bail;
		}

		CFRelease(cfFoundFilename);
	} while (filesFound && !containerChanged);
	
bail:	
	if (err == errFSNoMoreItems)
		err = noErr;

	if (dirItr)
		FSCloseIterator(dirItr);

	if (cfFilename)
		CFRelease(cfFilename);

	return err;
}

@implementation SubtitleSerializer
-(id)init
{
	if (self = [super init]) {
		lines = [[NSMutableArray alloc] init];
		outpackets = [[NSMutableArray alloc] init];
		finished = NO;
		write_gap = NO;
	}
	
	return self;
}

-(void)dealloc
{
	[outpackets release];
	[lines release];
	[super dealloc];
}

-(void)addLine:(SubLine *)sline
{
	if (sline->begin_time < sline->end_time)
		[lines addObject:sline];
}

static int cmp_line(id a, id b, void* unused)
{			
	SubLine *av = (SubLine*)a, *bv = (SubLine*)b;
	
	if (av->begin_time > bv->begin_time) return NSOrderedDescending;
	if (av->begin_time < bv->begin_time) return NSOrderedAscending;
	return NSOrderedSame;
}

static int cmp_uint(const void *a, const void *b)
{
	unsigned av = *(unsigned*)a, bv = *(unsigned*)b;
	
	if (av > bv) return 1;
	if (av < bv) return -1;
	return 0;
}

static bool isinrange(unsigned base, unsigned test_s, unsigned test_e)
{
	return (base >= test_s) && (base < test_e);
}

-(void)refill
{
	unsigned num = [lines count];
	unsigned min_allowed = finished ? 1 : 2;
	if (num < min_allowed) return;
	unsigned times[num*2], last_last_end = 0;
	SubLine *slines[num], *last=nil;
	bool last_has_invalid_end = false;
	
	[lines sortUsingFunction:cmp_line context:nil];
	[lines getObjects:slines];
#ifdef SS_DEBUG
	NSLog(@"pre - %@",lines);
#endif	
	//leave early if all subtitle lines overlap
	if (!finished) {
		bool all_overlap = true;
		int i;
		
		for (i=0;i < num-1;i++) {
			SubLine *c = slines[i], *n = slines[i+1];
			if (c->end_time <= n->begin_time) {all_overlap = false; break;}
		}
		
		if (all_overlap) return;
		
		for (i=0;i < num-1;i++) {
			if (isinrange(slines[num-1]->begin_time, slines[i]->begin_time, slines[i]->end_time)) {
				num = i + 1; break;
			}
		}
	}
		
	for (int i=0;i < num;i++) {
		times[i*2]   = slines[i]->begin_time;
		times[i*2+1] = slines[i]->end_time;
	}
		
	qsort(times, num*2, sizeof(unsigned), cmp_uint);
	
	for (int i=0;i < num*2; i++) {
		if (i > 0 && times[i-1] == times[i]) continue;
		NSMutableString *accum = nil;
		unsigned start = times[i], last_end = start, next_start=times[num*2-1], end = start;
		bool finishedOutput = false, is_last_line = false;
		
		// Add on packets until we find one that marks it ending (by starting later)
		// ...except if we know this is the last input packet from the stream, then we have to explicitly flush it
		if (finished && (times[i] == slines[num-1]->begin_time || times[i] == slines[num-1]->end_time)) finishedOutput = is_last_line = true;
			
		for (int j=0; j < num; j++) {
			if (isinrange(times[i], slines[j]->begin_time, slines[j]->end_time)) {
				
				// find the next line that starts after this one
				if (j != num-1) {
					unsigned ns = slines[j]->end_time;
					for (int h = j; h < num; h++) if (slines[h]->begin_time != slines[j]->begin_time) {ns = slines[h]->begin_time; break;}
					next_start = MIN(next_start, ns);
				} else next_start = slines[j]->end_time;
					
				last_end = MAX(slines[j]->end_time, last_end);
				if (accum) [accum appendString:slines[j]->line]; else accum = [[slines[j]->line mutableCopy] autorelease];
			} else if (j == num-1) finishedOutput = true;
		}
				
		if (accum && finishedOutput) {
			[accum deleteCharactersInRange:NSMakeRange([accum length] - 1, 1)]; // delete last newline
#ifdef SS_DEBUG
			NSLog(@"%d - %d %d",start,last_end,next_start);	
#endif
			if (last_has_invalid_end) {
				if (last_end < next_start) { 
					int j, set;
					for (j=i; j >= 0; j--) if (times[j] == last->begin_time) break;
					set = times[j+1];
					last->end_time = set;
				} else last->end_time = MIN(last_last_end,start); 
			}
			end = last_end;
			last_has_invalid_end = false;
			if (last_end > next_start && !is_last_line) last_has_invalid_end = true;
			SubLine *event = [[SubLine alloc] initWithLine:accum start:start end:end];
			
			[outpackets addObject:event];
			
			last_last_end = last_end;
			last = event;
		}
	}
	
	if (last_has_invalid_end) {
		last->end_time = times[num*2 - 3]; // end time of line before last
	}
#ifdef SS_DEBUG
	NSLog(@"out - %@",outpackets);
#endif
	
	if (finished) [lines removeAllObjects];
	else {
		num = [lines count];
		for (int i = 0; i < num-1; i++) {
			if (isinrange(slines[num-1]->begin_time, slines[i]->begin_time, slines[i]->end_time)) break;
			[lines removeObject:slines[i]];
		}
	}
#ifdef SS_DEBUG
	NSLog(@"post - %@",lines);
#endif
}

-(SubLine*)getSerializedPacket
{
	int packetcount;
	
	if ([outpackets count] == 0)  {
		[self refill];
		if ([outpackets count] == 0) 
			return nil;
	}
	
	packetcount = [outpackets count];
	if (!finished && packetcount < 2) return nil;
	else if (finished && packetcount == 1) {
		SubLine *sl = [outpackets objectAtIndex:0];
		[outpackets removeObjectAtIndex:0];
		[sl autorelease];
		return sl;
	}
	
	SubLine *bl = [outpackets objectAtIndex:0], *el;
		
	if (write_gap) {
		el = [outpackets objectAtIndex:1];
		write_gap = false;
		[bl autorelease];
		[outpackets removeObjectAtIndex:0];
		if (el->begin_time > bl->end_time) {
			SubLine *ret = [[[SubLine alloc] initWithLine:@"\n" start:bl->end_time end:el->begin_time] autorelease];
			return ret;
		}
		return [self getSerializedPacket]; // this is not so clean
	}
	
	
	write_gap = true;
	return bl;
}

-(void)setFinished:(BOOL)f
{
	finished = f;
}

-(BOOL)isEmpty
{
	return [lines count] == 0 && [outpackets count] == 0;
}

-(NSString*)description
{
	return [NSString stringWithFormat:@"i: %d o: %d finished inputting: %d",[lines count],[outpackets count],finished];
}
@end

@implementation SubLine
-(id)initWithLine:(NSString*)l start:(unsigned)s end:(unsigned)e
{
	if (self = [super init]) {
		if ([l characterAtIndex:[l length]-1] != '\n') l = [l stringByAppendingString:@"\n"];
		line = [l retain];
		begin_time = s;
		end_time = e;
	}
	
	return self;
}

-(void)dealloc
{
	[line release];
	[super dealloc];
}

-(NSString*)description
{
	return [NSString stringWithFormat:@"\"%@\", %d -> %d",line,begin_time,end_time];
}
@end

CXXSubtitleSerializer::CXXSubtitleSerializer()
{
	priv = [[SubtitleSerializer alloc] init];
}

CXXSubtitleSerializer::~CXXSubtitleSerializer()
{
	if (priv) {[(SubtitleSerializer*)priv release]; priv = NULL;}
}

void CXXSubtitleSerializer::pushLine(const char *line, size_t size, unsigned start, unsigned end)
{
	NSString *str = [[NSString alloc] initWithBytes:line length:size encoding:NSUTF8StringEncoding];
	NSString *strn = [str stringByAppendingString:@"\n"];
	[str release];
	
	SubLine *sl = [[SubLine alloc] initWithLine:strn start:start end:end];
	
	[sl autorelease];
	
	[(SubtitleSerializer*)priv addLine:sl];
}


void CXXSubtitleSerializer::setFinished()
{
	[(SubtitleSerializer*)priv setFinished:YES];
}

const char *CXXSubtitleSerializer::popPacket(size_t *size, unsigned *start, unsigned *end)
{
	SubLine *sl = [(SubtitleSerializer*)priv getSerializedPacket];
	if (!sl) return NULL;
	const char *u = [sl->line UTF8String];
	*start = sl->begin_time;
	*end   = sl->end_time;
	
	*size = strlen(u);
	
	return u;
}

void CXXSubtitleSerializer::release()
{
	SubtitleSerializer *s = (SubtitleSerializer*)priv;
	int r = [s retainCount];
	
	if (r == 1) delete this;
	else [s release];
}

void CXXSubtitleSerializer::retain()
{
	[(SubtitleSerializer*)priv retain];
}

bool CXXSubtitleSerializer::empty()
{
	return [(SubtitleSerializer*)priv isEmpty];
}

CXXAutoreleasePool::CXXAutoreleasePool()
{
	pool = [[NSAutoreleasePool alloc] init];
}

CXXAutoreleasePool::~CXXAutoreleasePool()
{
	[(NSAutoreleasePool*)pool release];
}
