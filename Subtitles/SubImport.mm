//
//  SubImport.m
//  SSARender2
//
//  Created by Alexander Strange on 7/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#include <QuickTime/QuickTime.h>
#include "CommonUtils.h"
#include "Codecprintf.h"
#import "SubImport.h"
#import "SubParsing.h"
#import "SubUtilities.h"

//#define SS_DEBUG

#pragma mark C

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

static NSString *MatroskaPacketizeLine(NSDictionary *sub, int n)
{
	NSString *name = [sub objectForKey:@"Name"];
	if (!name) name = [sub objectForKey:@"Actor"];
	
	return [NSString stringWithFormat:@"%d,%d,%@,%@,%@,%@,%@,%@,%@\n",
		n+1,
		[[sub objectForKey:@"Layer"] intValue],
		[sub objectForKey:@"Style"],
		[sub objectForKey:@"Name"],
		[sub objectForKey:@"MarginL"],
		[sub objectForKey:@"MarginR"],
		[sub objectForKey:@"MarginV"],
		[sub objectForKey:@"Effect"],
		[sub objectForKey:@"Text"]];
}

static unsigned ParseSSATime(NSString *time)
{
	unsigned hour, minute, second, millisecond;
	
	sscanf([time UTF8String],"%u:%u:%u.%u",&hour,&minute,&second,&millisecond);
	
	return hour * 100 * 60 * 60 + minute * 100 * 60 + second * 100 + millisecond;
}

static NSString *LoadSSAFromPath(NSString *path, SubSerializer *ss)
{
	NSString *nssSub = STLoadFileWithUnknownEncoding(path);
	
	if (!nssSub) return nil;
	
	size_t slen = [nssSub length], flen = sizeof(unichar) * (slen+1);
	unichar *subdata = (unichar*)malloc(flen);
	[nssSub getCharacters:subdata];
	
	if (subdata[slen-1] != '\n') subdata[slen++] = '\n'; // append newline if missing
	
	NSDictionary *headers;
	NSArray *subs;
	
	SubParseSSAFile(subdata, slen, &headers, NULL, &subs);
	free(subdata);
	
	int i, numlines = [subs count];
	
	for (i = 0; i < numlines; i++) {
		NSDictionary *sub = [subs objectAtIndex:i];
		SubLine *sl = [[SubLine alloc] initWithLine:MatroskaPacketizeLine(sub, i) 
											  start:ParseSSATime([sub objectForKey:@"Start"]) end:ParseSSATime([sub objectForKey:@"End"])];
		
		[ss addLine:sl];
		[sl autorelease];
	}
		
	return [nssSub substringToIndex:[nssSub rangeOfString:@"[Events]" options:NSLiteralSearch].location];
}

static void LoadSRTFromPath(NSString *path, SubSerializer *ss)
{
	NSMutableString *srt = STStandardizeStringNewlines(STLoadFileWithUnknownEncoding(path));
	if (!srt) return;
		
	if ([srt characterAtIndex:0] == 0xFEFF) [srt deleteCharactersInRange:NSMakeRange(0,1)];
	if ([srt characterAtIndex:[srt length]-1] != '\n') [srt appendFormat:@"%c",'\n'];
	
	NSScanner *sc = [NSScanner scannerWithString:srt];
	NSString *res=nil;
	[sc setCharactersToBeSkipped:nil];
	
	int h, m, s, ms;
	unsigned startTime=0, endTime=0;
	
	enum {
		INITIAL,
		TIMESTAMP,
		LINES
	} state = INITIAL;
	
	do {
		switch (state) {
			case INITIAL:
				if ([sc scanInt:NULL] == TRUE && [sc scanUpToString:@"\n" intoString:&res] == FALSE) {
					state = TIMESTAMP;
					[sc scanString:@"\n" intoString:nil];
				} else
					[sc setScanLocation:[sc scanLocation]+1];
				break;
			case TIMESTAMP:
				[sc scanInt:&h];  [sc scanString:@":" intoString:nil];
				[sc scanInt:&m];  [sc scanString:@":" intoString:nil];				
				[sc scanInt:&s];  [sc scanString:@"," intoString:nil];				
				[sc scanInt:&ms]; [sc scanString:@" --> " intoString:nil];
				startTime = ms + s*1000 + m*1000*60 + h*1000*60*60;
				[sc scanInt:&h];  [sc scanString:@":" intoString:nil];
				[sc scanInt:&m];  [sc scanString:@":" intoString:nil];				
				[sc scanInt:&s];  [sc scanString:@"," intoString:nil];				
				[sc scanInt:&ms]; [sc scanString:@"\n" intoString:nil];	
				endTime = ms + s*1000 + m*1000*60 + h*1000*60*60;
				state = LINES;
				break;
			case LINES:
				[sc scanUpToString:@"\n\n" intoString:&res];
				[sc scanString:@"\n\n" intoString:nil];
				SubLine *sl = [[SubLine alloc] initWithLine:res start:startTime end:endTime];
				[ss addLine:[sl autorelease]];
				state = INITIAL;
				break;
		};
	} while (![sc isAtEnd]);
}

ComponentResult LoadSingleTextSubtitle(const FSRef *theDirectory, CFStringRef filename, Movie theMovie, Track *firstSubTrack, int subtitleType)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	UInt8 path[PATH_MAX];
	
	FSRefMakePath(theDirectory, path, PATH_MAX);
	NSString *nsPath = [[NSString stringWithUTF8String:(char*)path] stringByAppendingPathComponent:(NSString*)filename];
	
	SubSerializer *ss = [[SubSerializer alloc] init];
	
	FourCharCode subCodec;
	Handle header = NULL;
	TimeScale timeBase;
	Fixed movieRate = fixed1;
	
	switch (subtitleType) {
		case kSubTypeASS:
		case kSubTypeSSA:
		default:
		{
			timeBase = 100;
			subCodec = 'SSA ';
			const char *cheader = [LoadSSAFromPath(nsPath, ss) UTF8String];
			int headerLen = strlen(cheader);
			PtrToHand(cheader, &header, headerLen);
		}
			break;
		case kSubTypeSRT:
			timeBase = 1000;
			subCodec = 'SRT ';
			LoadSRTFromPath(nsPath, ss);
			break;
	}
	
	[ss setFinished:YES];

	Handle dataRefHndl = NewHandleClear(sizeof(Handle) + 1);
	UInt32 emptyDataRefExt[2] = {EndianU32_NtoB(sizeof(UInt32)*2), EndianU32_NtoB(kDataRefExtensionInitializationData)};
	PtrAndHand(emptyDataRefExt, dataRefHndl, sizeof(emptyDataRefExt));
	
	Rect movieBox;
	GetMovieBox(theMovie, &movieBox);
	
	ImageDescriptionHandle textDesc = (ImageDescriptionHandle) NewHandleClear(sizeof(ImageDescription));

	Track theTrack = CreatePlaintextSubTrack(theMovie, textDesc, timeBase, dataRefHndl, HandleDataHandlerSubType, subCodec, header, movieBox);
	Media theMedia = NULL;
	ComponentResult err=noErr;
	TimeScale movieTimeScale = GetMovieTimeScale(theMovie);
	
	if (theTrack == NULL) {
		err = GetMoviesError();
		goto bail;
	}
	
	theMedia = GetTrackMedia(theTrack);
	if (theMedia == NULL) {
		err = GetMoviesError();
		goto bail;
	}
	
	BeginMediaEdits(theMedia);
	
	while (![ss isEmpty]) {
		SubLine *sl = [ss getSerializedPacket];
		TimeRecord startTime = {SInt64ToWide(sl->begin_time), timeBase, 0};
		TimeValue sampleTime;
		const char *str = [sl->line UTF8String];
		unsigned sampleLen = strlen(str);
		Handle sampleHndl;
		
		PtrToHand(str, &sampleHndl, sampleLen);
		err = AddMediaSample(theMedia, sampleHndl, 0, sampleLen, sl->end_time - sl->begin_time, (SampleDescriptionHandle)textDesc, 1, 0, &sampleTime);
		
		if (err) {
			err = GetMoviesError();
			Codecprintf(stderr,"error %d adding line from %d to %d in external subtitles",err,sl->begin_time,sl->end_time);
			goto inLoopError;
		}
		
		ConvertTimeScale(&startTime, movieTimeScale);
		
		InsertMediaIntoTrack(theTrack, startTime.value.lo, sampleTime, sl->end_time - sl->begin_time, movieRate);
		
inLoopError:
		err = noErr;
		DisposeHandle(sampleHndl);
	}
	
	EndMediaEdits(theMedia);
	
	if (*firstSubTrack == NULL)
		*firstSubTrack = theTrack;
	else
		SetTrackAlternate(*firstSubTrack, theTrack);
	
	SetMediaLanguage(theMedia, GetFilenameLanguage(filename));
	
bail:
	[ss release];
	[pool release];
	
	if (err) {
		if (theMedia)
			DisposeTrackMedia(theMedia);
		
		if (theTrack)
			DisposeMovieTrack(theTrack);
	}
	
	if (textDesc)    DisposeHandle((Handle)textDesc);
	if (header)      DisposeHandle((Handle)header);
	if (dataRefHndl) DisposeHandle((Handle)dataRefHndl);
	
	return err;
}

static ComponentResult LoadVobSubSubtitles(const FSRef *theDirectory, CFStringRef filename, Movie theMovie, Track *firstSubTrack)
{
	ComponentResult err = noErr;
	
	return err;
}

static Boolean ShouldLoadExternalSubtitles()
{
	Boolean isSet, value;
	
	value = CFPreferencesGetAppBooleanValue(CFSTR("LoadExternalSubtitles"),CFSTR("org.perian.Perian"),&isSet);
	
	return isSet ? value : YES;
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
	
	if (!ShouldLoadExternalSubtitles()) return noErr;
	
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
			int subType = -1;
			
			// SubRip
			actRange = CFStringFind(cfFoundFilename, CFSTR(".srt"), kCFCompareCaseInsensitive | kCFCompareBackwards);
			if (actRange.length && actRange.location == extRange.location)
				subType = kSubTypeSRT;
			else {
				// SubStationAlpha
				actRange = CFStringFind(cfFoundFilename, CFSTR(".ass"), kCFCompareCaseInsensitive | kCFCompareBackwards);
				if (actRange.length && actRange.location == extRange.location)
					subType = kSubTypeASS;
				else {
					actRange = CFStringFind(cfFoundFilename, CFSTR(".ssa"), kCFCompareCaseInsensitive | kCFCompareBackwards);
					if (actRange.length && actRange.location == extRange.location)
						subType = kSubTypeSSA;
					else {
						// VobSub
						actRange = CFStringFind(cfFoundFilename, CFSTR(".idx"), kCFCompareCaseInsensitive | kCFCompareBackwards);
						if (actRange.length && actRange.location == extRange.location)
							err = LoadVobSubSubtitles(&parentDir, cfFoundFilename, theMovie, &firstSubTrack);
					}
				}
			}
			
			if (subType != -1) err = LoadSingleTextSubtitle(&parentDir, cfFoundFilename, theMovie, &firstSubTrack, subType);
			
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

ComponentResult LoadExternalSubtitlesFromFileDataRef(Handle dataRef, OSType dataRefType, Movie theMovie)
{
	if (dataRefType != AliasDataHandlerSubType) return noErr;
	
	CFStringRef cfPath;
	FSRef ref;
	uint8_t path[PATH_MAX];
	
	QTGetDataReferenceFullPathCFString(dataRef, dataRefType, kQTPOSIXPathStyle, &cfPath);
	CFStringGetCString(cfPath, (char*)path, PATH_MAX, kCFStringEncodingUTF8);
	FSPathMakeRef(path, &ref, NULL);
	CFRelease(cfPath);
	
	return LoadExternalSubtitles(&ref, theMovie);
}

#pragma mark Obj-C Classes

@implementation SubSerializer
-(id)init
{
	if (self = [super init]) {
		lines = [[NSMutableArray alloc] init];
		outpackets = [[NSMutableArray alloc] init];
		finished = NO;
		write_gap = NO;
		toReturn = nil;
		last_time = 0;
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

-(SubLine*)_getSerializedPacket
{
	if ([outpackets count] == 0)  {
		[self refill];
		if ([outpackets count] == 0) 
			return nil;
	}
	
	SubLine *sl = [outpackets objectAtIndex:0];
	[outpackets removeObjectAtIndex:0];
	
	[sl autorelease];
	return sl;
}

-(SubLine*)getSerializedPacket
{
	if (!last_time) {
		SubLine *ret = [self _getSerializedPacket];
		if (ret) {
			last_time = ret->end_time;
			write_gap = YES;
		}
		return ret;
	}
	
restart:
	
	if (write_gap) {
		SubLine *next = [self _getSerializedPacket];
		SubLine *sl;

		if (!next) return nil;

		toReturn = [next retain];
		
		write_gap = NO;
		
		if (toReturn->begin_time > last_time) sl = [[SubLine alloc] initWithLine:@"\n" start:last_time end:toReturn->begin_time];
		else goto restart;
		
		return [sl autorelease];
	} else {
		SubLine *ret = toReturn;
		last_time = ret->end_time;
		write_gap = YES;
		
		toReturn = nil;
		return [ret autorelease];
	}
}

-(void)setFinished:(BOOL)f
{
	finished = f;
}

-(BOOL)isEmpty
{
	return [lines count] == 0 && [outpackets count] == 0 && !toReturn;
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
	return [NSString stringWithFormat:@"\"%@\", from %d s to %d s",line,begin_time,end_time];
}
@end

#pragma mark C++ Wrappers

CXXSubSerializer::CXXSubSerializer()
{
	priv = [[SubSerializer alloc] init];
}

CXXSubSerializer::~CXXSubSerializer()
{
	if (priv) {[(SubSerializer*)priv release]; priv = NULL;}
}

void CXXSubSerializer::pushLine(const char *line, size_t size, unsigned start, unsigned end)
{
	NSString *str = [[NSString alloc] initWithBytes:line length:size encoding:NSUTF8StringEncoding];
	NSString *strn = [str stringByAppendingString:@"\n"];
	[str release];
	
	SubLine *sl = [[SubLine alloc] initWithLine:strn start:start end:end];
	
	[sl autorelease];
	
	[(SubSerializer*)priv addLine:sl];
}

void CXXSubSerializer::setFinished()
{
	[(SubSerializer*)priv setFinished:YES];
}

const char *CXXSubSerializer::popPacket(size_t *size, unsigned *start, unsigned *end)
{
	SubLine *sl = [(SubSerializer*)priv getSerializedPacket];
	if (!sl) return NULL;
	const char *u = [sl->line UTF8String];
	*start = sl->begin_time;
	*end   = sl->end_time;
	
	*size = strlen(u);
	
	return u;
}

void CXXSubSerializer::release()
{
	SubSerializer *s = (SubSerializer*)priv;
	int r = [s retainCount];
	
	if (r == 1) delete this;
	else [s release];
}

void CXXSubSerializer::retain()
{
	[(SubSerializer*)priv retain];
}

bool CXXSubSerializer::empty()
{
	return [(SubSerializer*)priv isEmpty];
}

CXXAutoreleasePool::CXXAutoreleasePool()
{
	pool = [[NSAutoreleasePool alloc] init];
}

CXXAutoreleasePool::~CXXAutoreleasePool()
{
	[(NSAutoreleasePool*)pool release];
}