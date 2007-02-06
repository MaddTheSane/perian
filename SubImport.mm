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
		}
	}

	return theTrack;
}

extern "C" ComponentResult LoadSubStationAlphaSubtitles(const FSRef *theDirectory, CFStringRef filename, Movie theMovie, Track *firstSubTrack);

ComponentResult LoadSubRipSubtitles(const FSRef *theDirectory, CFStringRef filename, Movie theMovie, Track *firstSubTrack)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	SubtitleSerializer *serializer = [[SubtitleSerializer alloc] init];
	ComponentResult err = noErr;
	Handle dataRef = NULL;
	Track theTrack = NULL;
	Media theMedia = NULL;
	Rect movieBox;
	SubLine *sl;
	TimeScale movieTimeScale = GetMovieTimeScale(theMovie);
	UInt32 emptyDataRefExtension[2];

	const char *data = NULL;
	ImageDescriptionHandle textDesc = (ImageDescriptionHandle) NewHandleClear(sizeof(ImageDescription));
	unsigned int subNum = 1;
	UInt8 *path = (UInt8*)malloc(PATH_MAX);
	NSString *srtfile;
	NSError *nserr = nil;
	Handle sampleHndl = NULL;
	
	FSRefMakePath(theDirectory, path, PATH_MAX);
	srtfile = [[NSString stringWithUTF8String:(char*)path] stringByAppendingPathComponent:(NSString*)filename];
	data = [[[NSString stringWithContentsOfFile:srtfile encoding:NSUTF8StringEncoding error:&nserr] stringByStandardizingNewlines] UTF8String];
	free(path);
	
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

	char subNumStr[10];

	snprintf(subNumStr, 10, "%u", subNum);
	char *subOffset = strstr(data, subNumStr);

	while (subOffset != NULL) {
		unsigned int starthour, startmin, startsec, startmsec;
		unsigned int endhour, endmin, endsec, endmsec;

		// skip the line with the subtitle number in it
		char *subTimecodeOffset = strchr(subOffset, '\n');
		if (subTimecodeOffset == NULL)
			break;
		subTimecodeOffset++;

		int ret = sscanf(subTimecodeOffset, "%u:%u:%u,%u --> %u:%u:%u,%u", 
				         &starthour, &startmin, &startsec, &startmsec, 
				         &endhour, &endmin, &endsec, &endmsec);

		// find the next subtitle
		// setting subOffset to point the end of the current subtitle text
		snprintf(subNumStr, 10, "\n%u", ++subNum);
		subOffset = strstr(subTimecodeOffset, subNumStr);

		if (ret == 8) {
			// skip to the beginning of the subtitle text
			char *subTextOffset = strchr(subTimecodeOffset, '\n') + 1;

			TimeValue startTime = startmsec + 1000*startsec + 60*1000*startmin + 60*60*1000*starthour;
			TimeValue endTime = endmsec + 1000*endsec + 60*1000*endmin + 60*60*1000*endhour;

			if (subOffset != NULL) {
				if(subOffset - subTextOffset - 1 > 0 && endTime > startTime) //Make sure subtitle is not empty
				{
					NSString *l = [[NSString alloc] initWithBytes:subTextOffset length:subOffset - subTextOffset encoding:NSUTF8StringEncoding];
					sl = [[SubLine alloc] initWithLine:l start:startTime end:endTime];
					
					[l autorelease];
					[sl autorelease];

					[serializer addLine:sl];
				}

				// with the subtitle offset, we want the number at the beginning
				// so advance past the newline in the stream
				subOffset += 1;
			}
		}
	}
	
	[serializer setFinished:YES];
	
	BeginMediaEdits(theMedia);
	
	while (sl = [serializer getSerializedPacket]) {
		TimeRecord movieStartTime = {SInt64ToWide(sl->begin_time), 1000, 0};
		TimeValue sampleTime;
		const char *str = [sl->line UTF8String];
		size_t sampleLen = strlen(str);
		
		PtrToHand(str,&sampleHndl,sampleLen);
		
		err=AddMediaSample(theMedia,sampleHndl,0,sampleLen, sl->end_time - sl->begin_time,(SampleDescriptionHandle)textDesc, 1, 0, &sampleTime);
		if (err != noErr) goto bail;
		
		ConvertTimeScale(&movieStartTime, movieTimeScale);
		
		err = InsertMediaIntoTrack(theTrack, movieStartTime.value.lo, sampleTime, sl->end_time - sl->begin_time, fixed1);
		if (err != noErr) goto bail;
		
		DisposeHandle(sampleHndl);
	}

	EndMediaEdits(theMedia);
	
	sampleHndl = NULL;

	if (*firstSubTrack == NULL)
		*firstSubTrack = theTrack;
	else
		SetTrackAlternate(*firstSubTrack, theTrack);

	SetMediaLanguage(theMedia, GetFilenameLanguage(filename));

bail:
	[serializer release];
	[pool release];
	if (err) {
		if (theMedia)
			DisposeTrackMedia(theMedia);

		if (theTrack)
			DisposeMovieTrack(theTrack);
	}

	if (textDesc)
		DisposeHandle((Handle) textDesc);

	DisposeHandle(dataRef);
	if (sampleHndl) DisposeHandle(sampleHndl);


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
				LoadSubRipSubtitles(&parentDir, cfFoundFilename, theMovie, &firstSubTrack);
			
			// SubStationAlpha
			actRange = CFStringFind(cfFoundFilename, CFSTR(".ass"), kCFCompareCaseInsensitive | kCFCompareBackwards);
			if (actRange.location == extRange.location)
				LoadSubStationAlphaSubtitles(&parentDir, cfFoundFilename, theMovie, &firstSubTrack);

			actRange = CFStringFind(cfFoundFilename, CFSTR(".ssa"), kCFCompareCaseInsensitive | kCFCompareBackwards);
			if (actRange.location == extRange.location)
				LoadSubStationAlphaSubtitles(&parentDir, cfFoundFilename, theMovie, &firstSubTrack);
			
			// VobSub
			actRange = CFStringFind(cfFoundFilename, CFSTR(".idx"), kCFCompareCaseInsensitive | kCFCompareBackwards);
			if (actRange.location == extRange.location)
				LoadVobSubSubtitles(&foundFileRef, cfFoundFilename, theMovie, &firstSubTrack);
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
	}
	
	return self;
}

-(void)addLine:(SubLine *)sline
{
	[lines addObject:sline];
}

-(void)addLineWithString:(NSString*)string start:(unsigned)start end:(unsigned)end
{
	SubLine *sl = [[[SubLine alloc] init] autorelease];
	
	sl->line = [string retain];
	sl->begin_time = start;
	sl->end_time = end;
	
	[lines addObject:sl];
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

/* FIXME: This algorithm is not nearly good enough.
   It works for trivial overlaps, but not [  [   ]   [   ]  ] (where [] are subtitle lines).
   Many other cases may fail. */

-(void)refill
{
	unsigned num = [lines count];
	unsigned times[num*2+1];
	SubLine *slines[num];
	
	[lines sortUsingFunction:cmp_line context:nil];
	[lines getObjects:slines];
	//NSLog(@"%@",lines);
	for (int i=0;i < num;i++) {
		times[i*2]   = slines[i]->begin_time;
		times[i*2+1] = slines[i]->end_time;
	}
	
	times[num * 2] = times[num * 2 - 1];
	
	qsort(times, num*2, sizeof(unsigned), cmp_uint);
	
	for (int i=0;i < num*2; i++) {
		if (i > 0 && times[i-1] == times[i]) continue;
		NSMutableString *accum = [NSMutableString string];
		unsigned start = times[i], end = start;
		bool startedOutput = false, finishedOutput = false;
		
		// Add on packets until we find one that marks it ending (by starting later)
		// ...except if we know this is the last input packet from the stream, then we have to explicitly flush it
		if (finished && i >= (num*2)-2) finishedOutput = true; 
			
		for (int j=0; j < num; j++) {
			if (isinrange(times[i], slines[j]->begin_time, slines[j]->end_time)) {
				end = slines[j]->end_time;
				[accum appendString:slines[j]->line];
				startedOutput = true;
			} else if (startedOutput) {finishedOutput = true; break;}
		}
		
		
		if (finishedOutput && startedOutput) {
			[accum deleteCharactersInRange:NSMakeRange([accum length] - 1, 1)]; // delete last newline
			SubLine *event = [[[SubLine alloc] initWithLine:accum start:start end:end] autorelease];
			
			[outpackets addObject:event];
		}
	}
	
	//NSLog(@"%@",outpackets);

	[lines removeAllObjects];
	if (!finished) [lines addObject:slines[num-1]]; //keep the last packet in the queue
}

-(SubLine*)getSerializedPacket
{
	if ([outpackets count] == 0) [self refill];
	if ([outpackets count] == 0) return nil;
	
	SubLine *sl = [outpackets objectAtIndex:0];
	[outpackets removeObjectAtIndex:0];
	
	return sl;
}

-(void)setFinished:(BOOL)f
{
	finished = f;
}
@end

@implementation SubLine
-(id)initWithLine:(NSString*)l start:(unsigned)s end:(unsigned)e
{
	if (self = [super init]) {
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