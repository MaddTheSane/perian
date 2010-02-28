/*
 * SubImport.mm
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

#include <QuickTime/QuickTime.h>
#include "CommonUtils.h"
#include "Codecprintf.h"
#include "CodecIDs.h"
#import "SubImport.h"
#import "SubParsing.h"
#import "SubUtilities.h"

//#define SS_DEBUG

extern "C" {
	int ExtractVobSubPacket(UInt8 *dest, const UInt8 *framedSrc, int srcSize, int *usedSrcBytes, int index);
	void set_track_colorspace_ext(ImageDescriptionHandle imgDescHandle, Fixed displayW, Fixed displayH);
}

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
	langStr = CFStringCreateWithSubstring(NULL, baseName, findResult);
	
	// check for 3 char language code
	if (findResult.length == 3)
		lang = ISO639_2ToQTLangCode([(NSString*)langStr UTF8String]);
	else if (findResult.length == 2) // and for a 2 char language code
		lang = ISO639_1ToQTLangCode([(NSString*)langStr UTF8String]);

	CFRelease(langStr);
	CFRelease(baseName);
	return lang;
}

static void AppendSRGBProfile(ImageDescriptionHandle imgDesc)
{
	if (!(*CGColorSpaceCopyICCProfile))
		return; //10.4 weak symbol check
	
	CGColorSpaceRef cSpace = GetSRGBColorSpace();
	CFDataRef cSpaceICC = CGColorSpaceCopyICCProfile(cSpace);
	
	if (!cSpaceICC)
		return;
	
	ICMImageDescriptionSetProperty(imgDesc, kQTPropertyClass_ImageDescription,
								   kICMImageDescriptionPropertyID_ICCProfile, sizeof(CFDataRef), &cSpaceICC);
	CFRelease(cSpaceICC);
}

void SetSubtitleMediaHandlerTransparent(MediaHandler mh)
{
	if (IsTransparentSubtitleHackEnabled()) {
		RGBColor blendColor = {0x0000, 0x0000, 0x0000};
		MediaSetGraphicsMode(mh, transparent, &blendColor);
	}
	else
		MediaSetGraphicsMode(mh, graphicsModePreBlackAlpha, NULL);
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
	
	if (!trackWidth || !trackHeight) {trackWidth = IntToFixed(640); trackHeight = IntToFixed(480);}
	
	if (imageExtension) AddImageDescriptionExtension(imgDesc, imageExtension, subType);
	AppendSRGBProfile(imgDesc);
	
	theTrack = NewMovieTrack(theMovie, trackWidth, trackHeight, kNoVolume);
	if (theTrack != NULL) {
		theMedia = NewTrackMedia(theTrack, VideoMediaType, timescale, dataRef, dataRefType);
		
		if (theMedia != NULL) {
			// finally, say that we're transparent
			MediaHandler mh = GetMediaHandler(theMedia);
			
			SetSubtitleMediaHandlerTransparent(mh);
			
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

static unsigned ParseSubTime(const char *time, unsigned secondScale, BOOL hasSign)
{
	unsigned hour, minute, second, subsecond, timeval;
	char separator;
	int sign = 1;
	
	if (hasSign && *time == '-') {
		sign = -1;
		time++;
	}
	
	if (sscanf(time,"%u:%u:%u%[,.:]%u",&hour,&minute,&second,&separator,&subsecond) < 5)
		return 0;
	
	timeval = hour * 60 * 60 + minute * 60 + second;
	timeval = secondScale * timeval + subsecond;
	
	return timeval * sign;
}

NSString *LoadSSAFromPath(NSString *path, SubSerializer *ss)
{
	NSString *ssa = STLoadFileWithUnknownEncoding(path);
	
	if (!ssa) return nil;
	
	NSDictionary *headers;
	NSArray *subs;
	
	SubParseSSAFile(ssa, &headers, NULL, &subs);
	
	int i, numlines = [subs count];
	
	for (i = 0; i < numlines; i++) {
		NSDictionary *sub = [subs objectAtIndex:i];
		SubLine *sl = [[SubLine alloc] initWithLine:MatroskaPacketizeLine(sub, i) 
											  start:ParseSubTime([[sub objectForKey:@"Start"] UTF8String],100,NO)
												end:ParseSubTime([[sub objectForKey:@"End"] UTF8String],100,NO)];
		
		[ss addLine:sl];
		[sl autorelease];
	}
		
	return [ssa substringToIndex:[ssa rangeOfString:@"[Events]" options:NSLiteralSearch].location];
}

#pragma mark SAMI Parsing

static void LoadSRTFromPath(NSString *path, SubSerializer *ss)
{
	NSMutableString *srt = STStandardizeStringNewlines(STLoadFileWithUnknownEncoding(path));
	if (![srt length]) return;
		
	if ([srt characterAtIndex:0] == 0xFEFF) [srt deleteCharactersInRange:NSMakeRange(0,1)];
	if ([srt characterAtIndex:[srt length]-1] != '\n') [srt appendFormat:@"%c",'\n'];
	
	NSScanner *sc = [NSScanner scannerWithString:srt];
	NSString *res=nil;
	[sc setCharactersToBeSkipped:nil];
	
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
				[sc scanUpToString:@" --> " intoString:&res];
				[sc scanString:@" --> " intoString:nil];
				startTime = ParseSubTime([res UTF8String], 1000, NO);
				
				[sc scanUpToString:@"\n" intoString:&res];
				[sc scanString:@"\n" intoString:nil];
				endTime = ParseSubTime([res UTF8String], 1000, NO);
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

static int parse_SYNC(NSString *str)
{
	NSScanner *sc = [NSScanner scannerWithString:str];

	int res;

	if ([sc scanString:@"START=" intoString:nil])
		[sc scanInt:&res];

	return res;
}

static NSArray *parse_STYLE(NSString *str)
{
	NSScanner *sc = [NSScanner scannerWithString:str];

	NSString *firstRes;
	NSString *secondRes;
	NSArray *subArray;
	int secondLoc;

	[sc scanUpToString:@"<P CLASS=" intoString:nil];
	if ([sc scanString:@"<P CLASS=" intoString:nil])
		[sc scanUpToString:@">" intoString:&firstRes];
	else
		firstRes = @"noClass";

	secondLoc = [str length] * .9;
	[sc setScanLocation:secondLoc];

	[sc scanUpToString:@"<P CLASS=" intoString:nil];
	if ([sc scanString:@"<P CLASS=" intoString:nil])
		[sc scanUpToString:@">" intoString:&secondRes];
	else
		secondRes = @"noClass";

	if ([firstRes isEqualToString:secondRes])
		secondRes = @"noClass";

	subArray = [NSArray arrayWithObjects:firstRes, secondRes, nil];

	return subArray;
}

static int parse_P(NSString *str, NSArray *subArray)
{
	NSScanner *sc = [NSScanner scannerWithString:str];

	NSString *res;
	int subLang;

	if ([sc scanString:@"CLASS=" intoString:nil])
		[sc scanUpToString:@">" intoString:&res];
	else
		res = @"noClass";

	if ([res isEqualToString:[subArray objectAtIndex:0]])
		subLang = 1;
	else if ([res isEqualToString:[subArray objectAtIndex:1]])
		subLang = 2;
	else
		subLang = 3;

	return subLang;
}

static NSString *parse_COLOR(NSString *str)
{
	NSString *cvalue;
	NSMutableString *cname = [NSMutableString stringWithString:str];

	if (![str length]) return str;
	
	if ([cname characterAtIndex:0] == '#' && [cname lengthOfBytesUsingEncoding:NSASCIIStringEncoding] == 7)
		cvalue = [NSString stringWithFormat:@"{\\1c&H%@%@%@&}", [cname substringWithRange:NSMakeRange(5,2)], [cname substringWithRange:NSMakeRange(3,2)], [cname substringWithRange:NSMakeRange(1,2)]];
	else {
		[cname replaceOccurrencesOfString:@"Aqua" withString:@"00FFFF" options:1 range:NSMakeRange(0,[cname length])];
		[cname replaceOccurrencesOfString:@"Black" withString:@"000000" options:1 range:NSMakeRange(0,[cname length])];
		[cname replaceOccurrencesOfString:@"Blue" withString:@"0000FF" options:1 range:NSMakeRange(0,[cname length])];
		[cname replaceOccurrencesOfString:@"Fuchsia" withString:@"FF00FF" options:1 range:NSMakeRange(0,[cname length])];
		[cname replaceOccurrencesOfString:@"Gray" withString:@"808080" options:1 range:NSMakeRange(0,[cname length])];
		[cname replaceOccurrencesOfString:@"Green" withString:@"008000" options:1 range:NSMakeRange(0,[cname length])];
		[cname replaceOccurrencesOfString:@"Lime" withString:@"00FF00" options:1 range:NSMakeRange(0,[cname length])];
		[cname replaceOccurrencesOfString:@"Maroon" withString:@"800000" options:1 range:NSMakeRange(0,[cname length])];
		[cname replaceOccurrencesOfString:@"Navy" withString:@"000080" options:1 range:NSMakeRange(0,[cname length])];
		[cname replaceOccurrencesOfString:@"Olive" withString:@"808000" options:1 range:NSMakeRange(0,[cname length])];
		[cname replaceOccurrencesOfString:@"Purple" withString:@"800080" options:1 range:NSMakeRange(0,[cname length])];
		[cname replaceOccurrencesOfString:@"Red" withString:@"FF0000" options:1 range:NSMakeRange(0,[cname length])];
		[cname replaceOccurrencesOfString:@"Silver" withString:@"C0C0C0" options:1 range:NSMakeRange(0,[cname length])];
		[cname replaceOccurrencesOfString:@"Teal" withString:@"008080" options:1 range:NSMakeRange(0,[cname length])];
		[cname replaceOccurrencesOfString:@"White" withString:@"FFFFFF" options:1 range:NSMakeRange(0,[cname length])];
		[cname replaceOccurrencesOfString:@"Yellow" withString:@"FFFF00" options:1 range:NSMakeRange(0,[cname length])];

		if ([cname lengthOfBytesUsingEncoding:NSASCIIStringEncoding] == 6)
			cvalue = [NSString stringWithFormat:@"{\\1c&H%@%@%@&}", [cname substringWithRange:NSMakeRange(4,2)], [cname substringWithRange:NSMakeRange(2,2)], [cname substringWithRange:NSMakeRange(0,2)]];
		else
			cvalue = @"{\\1c&HFFFFFF&}";
	}

	return cvalue;
}

static NSString *parse_FONT(NSString *str)
{
	NSScanner *sc = [NSScanner scannerWithString:str];

	NSString *res;
	NSString *color;

	if ([sc scanString:@"COLOR=" intoString:nil]) {
		[sc scanUpToString:@">" intoString:&res];
		color = parse_COLOR(res);
	}
	else
		color = @"{\\1c&HFFFFFF&}";

	return color;
}

static NSMutableString *StandardizeSMIWhitespace(NSString *str)
{
	if (!str) return nil;
	NSMutableString *ms = [NSMutableString stringWithString:str];
	[ms replaceOccurrencesOfString:@"\r" withString:@"" options:0 range:NSMakeRange(0,[ms length])];
	[ms replaceOccurrencesOfString:@"\n" withString:@"" options:0 range:NSMakeRange(0,[ms length])];
	[ms replaceOccurrencesOfString:@"&nbsp;" withString:@" " options:0 range:NSMakeRange(0,[ms length])];
	return ms;
}

static void LoadSMIFromPath(NSString *path, SubSerializer *ss, int subCount)
{
	NSMutableString *smi = StandardizeSMIWhitespace(STLoadFileWithUnknownEncoding(path));
	if (!smi) return;
		
	NSScanner *sc = [NSScanner scannerWithString:smi];
	NSString *res = nil;
	[sc setCharactersToBeSkipped:nil];
	[sc setCaseSensitive:NO];
	
	NSMutableString *cmt = [NSMutableString string];
	NSArray *subLanguage = parse_STYLE(smi);

	int startTime=-1, endTime=-1, syncTime=-1;
	int cc=1;
	
	enum {
		TAG_INIT,
		TAG_SYNC,
		TAG_P,
		TAG_BR_OPEN,
		TAG_BR_CLOSE,
		TAG_B_OPEN,
		TAG_B_CLOSE,
		TAG_I_OPEN,
		TAG_I_CLOSE,
		TAG_FONT_OPEN,
		TAG_FONT_CLOSE,
		TAG_COMMENT
	} state = TAG_INIT;
	
	do {
		switch (state) {
			case TAG_INIT:
				[sc scanUpToString:@"<SYNC" intoString:nil];
				if ([sc scanString:@"<SYNC" intoString:nil])
					state = TAG_SYNC;
				break;
			case TAG_SYNC:
				[sc scanUpToString:@">" intoString:&res];
				syncTime = parse_SYNC(res);
				if (startTime > -1) {
					endTime = syncTime;
					if (subCount == 2 && cc == 2)
						[cmt insertString:@"{\\an8}" atIndex:0];
					if (subCount == 1 && cc == 1 || subCount == 2 && cc == 2) {
						SubLine *sl = [[SubLine alloc] initWithLine:cmt start:startTime end:endTime];
						[ss addLine:[sl autorelease]];
					}
				}
				startTime = syncTime;
				[cmt setString:@""];
				state = TAG_COMMENT;
				break;
			case TAG_P:
				[sc scanUpToString:@">" intoString:&res];
				cc = parse_P(res, subLanguage);
				[cmt setString:@""];
				state = TAG_COMMENT;
				break;
			case TAG_BR_OPEN:
				[sc scanUpToString:@">" intoString:nil];
				[cmt appendString:@"\\n"];
				state = TAG_COMMENT;
				break;
			case TAG_BR_CLOSE:
				[sc scanUpToString:@">" intoString:nil];
				[cmt appendString:@"\\n"];
				state = TAG_COMMENT;
				break;
			case TAG_B_OPEN:
				[sc scanUpToString:@">" intoString:&res];
				[cmt appendString:@"{\\b1}"];
				state = TAG_COMMENT;
				break;
			case TAG_B_CLOSE:
				[sc scanUpToString:@">" intoString:nil];
				[cmt appendString:@"{\\b0}"];
				state = TAG_COMMENT;
				break;
			case TAG_I_OPEN:
				[sc scanUpToString:@">" intoString:&res];
				[cmt appendString:@"{\\i1}"];
				state = TAG_COMMENT;
				break;
			case TAG_I_CLOSE:
				[sc scanUpToString:@">" intoString:nil];
				[cmt appendString:@"{\\i0}"];
				state = TAG_COMMENT;
				break;
			case TAG_FONT_OPEN:
				[sc scanUpToString:@">" intoString:&res];
				[cmt appendString:parse_FONT(res)];
				state = TAG_COMMENT;
				break;
			case TAG_FONT_CLOSE:
				[sc scanUpToString:@">" intoString:nil];
				[cmt appendString:@"{\\1c&HFFFFFF&}"];
				state = TAG_COMMENT;
				break;
			case TAG_COMMENT:
				[sc scanString:@">" intoString:nil];
				if ([sc scanUpToString:@"<" intoString:&res])
					[cmt appendString:res];
				else
					[cmt appendString:@"<>"];
				if ([sc scanString:@"<" intoString:nil]) {
					if ([sc scanString:@"SYNC" intoString:nil]) {
						state = TAG_SYNC;
						break;
					}
					else if ([sc scanString:@"P" intoString:nil]) {
						state = TAG_P;
						break;
					}
					else if ([sc scanString:@"BR" intoString:nil]) {
						state = TAG_BR_OPEN;
						break;
					}
					else if ([sc scanString:@"/BR" intoString:nil]) {
						state = TAG_BR_CLOSE;
						break;
					}
					else if ([sc scanString:@"B" intoString:nil]) {
						state = TAG_B_OPEN;
						break;
					}
					else if ([sc scanString:@"/B" intoString:nil]) {
						state = TAG_B_CLOSE;
						break;
					}
					else if ([sc scanString:@"I" intoString:nil]) {
						state = TAG_I_OPEN;
						break;
					}
					else if ([sc scanString:@"/I" intoString:nil]) {
						state = TAG_I_CLOSE;
						break;
					}
					else if ([sc scanString:@"FONT" intoString:nil]) {
						state = TAG_FONT_OPEN;
						break;
					}
					else if ([sc scanString:@"/FONT" intoString:nil]) {
						state = TAG_FONT_CLOSE;
						break;
					}
					else {
						[cmt appendString:@"<"];
						state = TAG_COMMENT;
						break;
					}
				}
		}
	} while (![sc isAtEnd]);
}

static ComponentResult LoadSingleTextSubtitle(CFURLRef theDirectory, CFStringRef filename, Movie theMovie, Track *firstSubTrack, int subtitleType, int whichTrack)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];	
	NSString *nsPath = [[(NSURL*)theDirectory path] stringByAppendingPathComponent:(NSString*)filename];
	
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
			subCodec = kSubFormatSSA;
			const char *cheader = [LoadSSAFromPath(nsPath, ss) UTF8String];
			int headerLen = strlen(cheader);
			PtrToHand(cheader, &header, headerLen);
		}
			break;
		case kSubTypeSRT:
			timeBase = 1000;
			subCodec = kSubFormatUTF8;
			LoadSRTFromPath(nsPath, ss);
			break;
		case kSubTypeSMI:
			timeBase = 1000;
			subCodec = kSubFormatUTF8;
			LoadSMIFromPath(nsPath, ss, whichTrack);
			break;
	}
	
	[ss setFinished:YES];

	SubPrerollFromHeader(header ? *header : NULL, header ? GetHandleSize(header) : 0);
	
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
		} else {
			ConvertTimeScale(&startTime, movieTimeScale);
			InsertMediaIntoTrack(theTrack, startTime.value.lo, sampleTime, sl->end_time - sl->begin_time, movieRate);
		}
		
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

#pragma mark IDX Parsing

static NSString *getNextVobSubLine(NSEnumerator *lineEnum)
{
	NSString *line;
	while ((line = [lineEnum nextObject]) != nil) {
		//Reject empty lines which may contain whitespace
		if([line length] < 3)
			continue;
		
		if([line characterAtIndex:0] == '#')
			continue;
		
		break;
	}
	return line;
}

static Media createVobSubMedia(Movie theMovie, Rect movieBox, ImageDescriptionHandle *imgDescHand, Handle dataRef, OSType dataRefType, VobSubTrack *track, int imageWidth, int imageHeight)
{
	ImageDescriptionHandle imgDesc = (ImageDescriptionHandle) NewHandleClear(sizeof(ImageDescription));
	*imgDescHand = imgDesc;
	(*imgDesc)->idSize = sizeof(ImageDescription);
	(*imgDesc)->cType = kSubFormatVobSub;
	(*imgDesc)->frameCount = 1;
	(*imgDesc)->depth = 32;
	(*imgDesc)->clutID = -1;
	Fixed trackWidth = IntToFixed(movieBox.right - movieBox.left);
	Fixed trackHeight = IntToFixed(movieBox.bottom - movieBox.top);
	(*imgDesc)->width = FixedToInt(trackWidth);
	(*imgDesc)->height = FixedToInt(trackHeight);
	set_track_colorspace_ext(imgDesc, (*imgDesc)->width, (*imgDesc)->height);
	Track theTrack = NewMovieTrack(theMovie, trackWidth, trackHeight, kNoVolume);
	if(theTrack == NULL)
		return NULL;
	
	Media theMedia = NewTrackMedia(theTrack, VideoMediaType, 1000, dataRef, dataRefType);
	if(theMedia == NULL)
	{
		DisposeMovieTrack(theTrack);
		return NULL;
	}
	MediaHandler mh = GetMediaHandler(theMedia);	
	SetSubtitleMediaHandlerTransparent(mh);
	SetTrackLayer(theTrack, -1);
	
	if(imageWidth != 0)
	{
		(*imgDesc)->width = imageWidth;
		(*imgDesc)->height = imageHeight;
	}	
	
	Handle imgDescExt = NewHandle([track->privateData length]);
	memcpy(*imgDescExt, [track->privateData bytes], [track->privateData length]);
	
	AddImageDescriptionExtension(imgDesc, imgDescExt, kVobSubIdxExtension);
	DisposeHandle(imgDescExt);
	
	return theMedia;
}

static Boolean ReadPacketTimes(uint8_t *packet, uint32_t length, uint16_t *startTime, uint16_t *endTime, uint8_t *forced) {
	// to set whether the key sequences 0x01 - 0x02 have been seen
	Boolean loop = TRUE;
	*startTime = *endTime = 0;
	*forced = 0;

	int controlOffset = (packet[2] << 8) + packet[3];
	while(loop)
	{	
		if(controlOffset > length)
			return NO;
		uint8_t *controlSeq = packet + controlOffset;
		int32_t i = 4;
		int32_t end = length - controlOffset;
		uint16_t timestamp = (controlSeq[0] << 8) | controlSeq[1];
		uint16_t nextOffset = (controlSeq[2] << 8) + controlSeq[3];
		while (i < end) {
			switch (controlSeq[i]) {
				case 0x00:
					*forced = 1;
					i++;
					break;
					
				case 0x01:
					*startTime = (timestamp << 10) / 90;
					i++;
					break;
				
				case 0x02:
					*endTime = (timestamp << 10) / 90;
					i++;
					loop = false;
					break;
					
				case 0x03:
					// palette info, we don't care
					i += 3;
					break;
					
				case 0x04:
					// alpha info, we don't care
					i += 3;
					break;
					
				case 0x05:
					// coordinates of image, ffmpeg takes care of this
					i += 7;
					break;
					
				case 0x06:
					// offset of the first graphic line, and second, ffmpeg takes care of this
					i += 5;
					break;
					
				case 0xff:
					// end of control sequence
					if(controlOffset == nextOffset)
						loop = false;
					controlOffset = nextOffset;
					i = INT_MAX;
					break;
					
				default:
					Codecprintf(NULL, " !! Unknown control sequence 0x%02x  aborting (offset %x)\n", controlSeq[i], i);
					return NO;
					break;
			}
		}
		if(i != INT_MAX)
		{
			//End of packet
			loop = false;
		}
	}
	return YES;
}

typedef struct {
	Movie theMovie;
	OSType dataRefType;
	Handle dataRef;
	int imageWidth;
	int imageHeight;
	Rect movieBox;
	NSData *subFileData;
} VobSubInfo;

static OSErr loadTrackIntoMovie(VobSubTrack *track, VobSubInfo info, uint8_t onlyForced, Track *theTrack, uint8_t *hasForcedSubtitles)
{
	int sampleCount = [track->samples count];
	if(sampleCount == 0)
		return noErr;
	
	ImageDescriptionHandle imgDesc;
	Media trackMedia = createVobSubMedia(info.theMovie, info.movieBox, &imgDesc, info.dataRef, info.dataRefType, track, info.imageWidth, info.imageHeight);
	
	int totalSamples = 0;
	SampleReference64Ptr samples = (SampleReference64Ptr)calloc(sampleCount*2, sizeof(SampleReference64Record));
	SampleReference64Ptr sample = samples;
	int i;
	uint32_t lastTime = 0;
	VobSubSample *firstSample = nil;
	for(i=0; i<sampleCount; i++)
	{
		VobSubSample *currentSample = [track->samples objectAtIndex:i];
		int offset = currentSample->fileOffset;
		int nextOffset;
		if(i == sampleCount - 1)
			nextOffset = [info.subFileData length];
		else
			nextOffset = ((VobSubSample *)[track->samples objectAtIndex:i+1])->fileOffset;
		int size = nextOffset - offset;
		if(size < 0)
			//Skip samples for which we cannot determine size
			continue;
		
		NSData *subData = [info.subFileData subdataWithRange:NSMakeRange(offset, size)];
		uint8_t *extracted = (uint8_t *)malloc(size);
		//The index here likely should really be track->index, but I'm not sure we can really trust it.
		int extractedSize = ExtractVobSubPacket(extracted, (const UInt8 *)[subData bytes], size, &size, -1);
		
		uint16_t startTimestamp, endTimestamp;
		uint8_t forced;
		if(!ReadPacketTimes(extracted, extractedSize, &startTimestamp, &endTimestamp, &forced))
			continue;
		if(onlyForced && !forced)
			continue;
		if(forced)
			*hasForcedSubtitles = forced;
		free(extracted);
		uint32_t startTime = currentSample->timeStamp + startTimestamp;
		uint32_t endTime = currentSample->timeStamp + endTimestamp;
		int duration = endTimestamp - startTimestamp;
		if(duration <= 0)
			//Skip samples which are broken
			continue;
		if(firstSample == nil)
		{
			currentSample->timeStamp = startTime;
			firstSample = currentSample;
		}
		else if(lastTime != startTime)
		{
			//insert a sample with no real data, to clear the subs
			memset(sample, 0, sizeof(SampleReference64Record));
			sample->durationPerSample = startTime - lastTime;
			sample->numberOfSamples = 1;
			sample->dataSize = 1;
			totalSamples++;
			sample++;
		}
		
		sample->dataOffset.hi = 0;
		sample->dataOffset.lo = offset;
		sample->dataSize = size;
		sample->sampleFlags = 0;
		sample->durationPerSample = duration;
		sample->numberOfSamples = 1;
		lastTime = endTime;
		totalSamples++;
		sample++;
	}
	AddMediaSampleReferences64(trackMedia, (SampleDescriptionHandle)imgDesc, totalSamples, samples, NULL);
	free(samples);
	NSString *langStr = track->language;
	int lang = langUnspecified;
	if([langStr length] == 3)
		lang = ISO639_2ToQTLangCode([langStr UTF8String]);
	else if([langStr length] == 2)
		lang = ISO639_1ToQTLangCode([langStr UTF8String]);		
	SetMediaLanguage(trackMedia, lang);
	
	TimeValue mediaDuration = GetMediaDuration(trackMedia);
	TimeValue movieTimeScale = GetMovieTimeScale(info.theMovie);
	*theTrack = GetMediaTrack(trackMedia);
	if(firstSample == nil)
		firstSample = [track->samples objectAtIndex:0];
	return InsertMediaIntoTrack(*theTrack, (firstSample->timeStamp * movieTimeScale)/1000, 0, mediaDuration, fixed1);
}

typedef enum {
	VOB_SUB_STATE_READING_PRIVATE,
	VOB_SUB_STATE_READING_TRACK_HEADER,
	VOB_SUB_STATE_READING_DELAY,
	VOB_SUB_STATE_READING_TRACK_DATA
} VobSubState;

static ComponentResult LoadVobSubSubtitles(CFURLRef theDirectory, CFStringRef filename, Movie theMovie, Track *firstSubTrack)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *nsPath = [[(NSURL*)theDirectory path] stringByAppendingPathComponent:(NSString *)filename];
	NSString *idxContent = STLoadFileWithUnknownEncoding(nsPath);
	NSData *privateData = nil;
	ComponentResult err = noErr;
	
	VobSubState state = VOB_SUB_STATE_READING_PRIVATE;
	VobSubTrack *currentTrack = nil;
	int imageWidth = 0, imageHeight = 0;
	long delay=0;

	NSString *subFileName = [[nsPath stringByDeletingPathExtension] stringByAppendingPathExtension:@"sub"];

	if([[NSFileManager defaultManager] fileExistsAtPath:subFileName] && [idxContent length]) {
	int subFileSize = [[[[NSFileManager defaultManager] fileAttributesAtPath:subFileName traverseLink:NO] objectForKey:NSFileSize] intValue];
	
	NSArray *lines = [idxContent componentsSeparatedByString:@"\n"];
	NSMutableArray *privateLines = [NSMutableArray array];
	NSEnumerator *lineEnum = [lines objectEnumerator];
	NSString *line;
	Rect movieBox;
	GetMovieBox(theMovie, &movieBox);
	
	NSMutableArray *tracks = [NSMutableArray array];
	
	while((line = getNextVobSubLine(lineEnum)) != NULL)
	{
		if([line hasPrefix:@"timestamp: "])
			state = VOB_SUB_STATE_READING_TRACK_DATA;
		else if([line hasPrefix:@"id: "])
		{
			if(privateData == nil)
			{
				NSString *allLines = [privateLines componentsJoinedByString:@"\n"];
				privateData = [allLines dataUsingEncoding:NSUTF8StringEncoding];
			}
			state = VOB_SUB_STATE_READING_TRACK_HEADER;
		}
		else if([line hasPrefix:@"delay: "])
			state = VOB_SUB_STATE_READING_DELAY;
		else if(state != VOB_SUB_STATE_READING_PRIVATE)
			state = VOB_SUB_STATE_READING_TRACK_HEADER;
		
		switch(state)
		{
			case VOB_SUB_STATE_READING_PRIVATE:
				[privateLines addObject:line];
				if([line hasPrefix:@"size: "])
				{
					sscanf([line UTF8String], "size: %dx%d", &imageWidth, &imageHeight);
				}
				break;
			case VOB_SUB_STATE_READING_TRACK_HEADER:
				if([line hasPrefix:@"id: "])
				{
					char *langStr = (char *)malloc([line length]);
					int index;
					sscanf([line UTF8String], "id: %s index: %d", langStr, &index);
					int langLength = strlen(langStr);
					if(langLength > 0 && langStr[langLength-1] == ',')
						langStr[langLength-1] = 0;
					NSString *language = [NSString stringWithUTF8String:langStr];
					
					currentTrack = [[VobSubTrack alloc] initWithPrivateData:privateData language:language andIndex:index];
					[tracks addObject:currentTrack];
					[currentTrack release];
				}
				break;
			case VOB_SUB_STATE_READING_DELAY:
				delay = ParseSubTime([[line substringFromIndex:7] UTF8String], 1000, YES);
				break;
			case VOB_SUB_STATE_READING_TRACK_DATA:
			{
				char *timeStr = (char *)malloc([line length]);
				unsigned int position;
				sscanf([line UTF8String], "timestamp: %s filepos: %x", timeStr, &position);
				long time = ParseSubTime(timeStr, 1000, YES);
				free(timeStr);
				if(position > subFileSize)
					position = subFileSize;
				[currentTrack addSampleTime:time + delay offset:position];
			}
				break;
		}
	}
		
	if([tracks count])
	{
		OSType dataRefType;
		Handle dataRef = NULL;
		
		NSData *subFileData = [NSData dataWithContentsOfMappedFile:subFileName];
		FSRef subFile;
		FSPathMakeRef((const UInt8*)[subFileName fileSystemRepresentation], &subFile, NULL);
		
		if((err = QTNewDataReferenceFromFSRef(&subFile, 0, &dataRef, &dataRefType)) != noErr)
			goto bail;
		
		NSEnumerator *trackEnum = [tracks objectEnumerator];
		VobSubTrack *track = nil;
		while((track = [trackEnum nextObject]) != nil)
		{
			Track theTrack = NULL;
			VobSubInfo info = {theMovie, dataRefType, dataRef, imageWidth, imageHeight, movieBox, subFileData};
			uint8_t hasForced = 0;
			err = loadTrackIntoMovie(track, info, 0, &theTrack, &hasForced);
			if(theTrack && hasForced)
			{
				Track forcedTrack;
				err = loadTrackIntoMovie(track, info, 1, &forcedTrack, &hasForced);
				if(*firstSubTrack == NULL)
					*firstSubTrack = forcedTrack;
				else
					SetTrackAlternate(*firstSubTrack, forcedTrack);
			}
			
			if (*firstSubTrack == NULL)
				*firstSubTrack = theTrack;
			else if(theTrack)
				SetTrackAlternate(*firstSubTrack, theTrack);
		}
	}
	}
bail:
	[pool release];
	
	return err;
}

static Boolean ShouldLoadExternalSubtitles()
{
	Boolean isSet, value;
	
	value = CFPreferencesGetAppBooleanValue(CFSTR("LoadExternalSubtitles"),CFSTR("org.perian.Perian"),&isSet);
	
	return isSet ? value : YES;
}

static ComponentResult LoadExternalSubtitles(CFURLRef theFileURL, Movie theMovie)
{
	ComponentResult err = noErr;
	Track firstSubTrack = NULL;
	CFStringRef cfFilename = NULL;
	FSRef parentDir;
	FSIterator dirItr = NULL;
	CFRange baseFilenameRange;
	ItemCount filesFound;
	Boolean containerChanged;
	CFURLRef parentURL;
		
	// find the location of the extension
	cfFilename = CFURLCopyLastPathComponent(theFileURL);
	baseFilenameRange = CFStringFind(cfFilename, CFSTR("."), kCFCompareBackwards);
	
	// strip the extension
	if (baseFilenameRange.location != kCFNotFound) {
		CFStringRef temp = cfFilename;
		baseFilenameRange.length = baseFilenameRange.location;
		baseFilenameRange.location = 0;
		cfFilename = CFStringCreateWithSubstring(NULL, temp, baseFilenameRange);
		CFRelease(temp);
	}
	
	parentURL = CFURLCreateCopyDeletingLastPathComponent(NULL, theFileURL);
	CFURLGetFSRef(parentURL, &parentDir);
	
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
		
		cfFoundFilename = CFStringCreateWithCharactersNoCopy(NULL, hfsFoundFilename.unicode, hfsFoundFilename.length, kCFAllocatorNull);
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
							err = LoadVobSubSubtitles(parentURL, cfFoundFilename, theMovie, &firstSubTrack);
						else {
							// SAMI
							actRange = CFStringFind(cfFoundFilename, CFSTR(".smi"), kCFCompareCaseInsensitive | kCFCompareBackwards);
							if (actRange.length && actRange.location == extRange.location)
								subType = kSubTypeSMI;
						}
					}
				}
			}
			
			if (subType == kSubTypeSMI) {
				err = LoadSingleTextSubtitle(parentURL, cfFoundFilename, theMovie, &firstSubTrack, kSubTypeSMI, 1);
				if (!err) err = LoadSingleTextSubtitle(parentURL, cfFoundFilename, theMovie, &firstSubTrack, kSubTypeSMI, 2);
			}
			else if (subType != -1)
				err = LoadSingleTextSubtitle(parentURL, cfFoundFilename, theMovie, &firstSubTrack, subType, 0);

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
	
	CFRelease(parentURL);
	
	return err;
}

ComponentResult LoadExternalSubtitlesFromFileDataRef(Handle dataRef, OSType dataRefType, Movie theMovie)
{
	if (dataRefType != AliasDataHandlerSubType) return noErr;
	if (!ShouldLoadExternalSubtitles()) return noErr;

	CFStringRef cfPath = NULL;
	
	OSErr err = QTGetDataReferenceFullPathCFString(dataRef, dataRefType, kQTPOSIXPathStyle, &cfPath);
	if(err != noErr) return err;

	CFURLRef cfURL = CFURLCreateWithFileSystemPath(NULL, cfPath, kCFURLPOSIXPathStyle, FALSE);
	CFRelease(cfPath);
	
	if (!cfURL) return noErr;
	
	err = LoadExternalSubtitles(cfURL, theMovie);
	
	CFRelease(cfURL);
	
	return err;
}

#pragma mark Obj-C Classes

@implementation SubSerializer
-(id)init
{
	if (self = [super init]) {
		lines = [[NSMutableArray alloc] init];
		finished = NO;
		last_begin_time = last_end_time = 0;
		linesInput = 0;
	}
	
	return self;
}

-(void)dealloc
{
	[lines release];
	[super dealloc];
}

static CFComparisonResult CompareLinesByBeginTime(const void *a, const void *b, void *unused)
{
	SubLine *al = (SubLine*)a, *bl = (SubLine*)b;
	
	if (al->begin_time > bl->begin_time) return kCFCompareGreaterThan;
	if (al->begin_time < bl->begin_time) return kCFCompareLessThan;
	
	if (al->no > bl->no) return kCFCompareGreaterThan;
	if (al->no < bl->no) return kCFCompareLessThan;
	return kCFCompareEqualTo;
}

static int cmp_uint(const void *a, const void *b)
{
	unsigned av = *(unsigned*)a, bv = *(unsigned*)b;
	
	if (av > bv) return 1;
	if (av < bv) return -1;
	return 0;
}

-(void)addLine:(SubLine *)line
{
	if (line->begin_time >= line->end_time) {
		if (line->begin_time)
			Codecprintf(NULL, "Invalid times (%d and %d) for line \"%s\"", line->begin_time, line->end_time, [line->line UTF8String]);
		return;
	}
	
	line->no = linesInput++;
	
	int nlines = [lines count];
	
	if (!nlines || line->begin_time > ((SubLine*)[lines objectAtIndex:nlines-1])->begin_time) {
		[lines addObject:line];
	} else {
		CFIndex i = CFArrayBSearchValues((CFArrayRef)lines, CFRangeMake(0, nlines), line, CompareLinesByBeginTime, NULL);
		
		if (i >= nlines)
			[lines addObject:line];
		else
			[lines insertObject:line atIndex:i];
	}
	
}

-(SubLine*)getNextRealSerializedPacket
{
	int nlines = [lines count];
	SubLine *first = [lines objectAtIndex:0];
	int i;

	if (!finished) {
		if (nlines > 1) {
			unsigned maxEndTime = first->end_time;
			
			for (i = 1; i < nlines; i++) {
				SubLine *l = [lines objectAtIndex:i];
				
				if (l->begin_time >= maxEndTime) {
					goto canOutput;
				}
				
				maxEndTime = MAX(maxEndTime, l->end_time);
			}
		}
		
		return nil;
	}
	
canOutput:
	NSMutableString *str = [NSMutableString stringWithString:first->line];
	unsigned begin_time = last_end_time, end_time = first->end_time;
	int deleted = 0;
		
	for (i = 1; i < nlines; i++) {
		SubLine *l = [lines objectAtIndex:i];
		if (l->begin_time >= end_time) break;
		
		//shorten packet end time if another shorter time (begin or end) is found
		//as long as it isn't the begin time
		end_time = MIN(end_time, l->end_time);
		if (l->begin_time > begin_time)
			end_time = MIN(end_time, l->begin_time);
		
		if (l->begin_time <= begin_time)
			[str appendString:l->line];
	}
	
	for (i = 0; i < nlines; i++) {
		SubLine *l = [lines objectAtIndex:i - deleted];
		
		if (l->end_time == end_time) {
			[lines removeObjectAtIndex:i - deleted];
			deleted++;
		}
	}
	
	return [[SubLine alloc] initWithLine:str start:begin_time end:end_time];
}

-(SubLine*)getSerializedPacket
{
	int nlines = [lines count];

	if (!nlines) return nil;
	
	SubLine *nextline = [lines objectAtIndex:0], *ret;
	
	if (nextline->begin_time > last_end_time) {
		ret = [[SubLine alloc] initWithLine:@"\n" start:last_end_time end:nextline->begin_time];
	} else {
		ret = [self getNextRealSerializedPacket];
	}
	
	if (!ret) return nil;
	
	last_begin_time = ret->begin_time;
	last_end_time   = ret->end_time;
		
	return [ret autorelease];
}

-(void)setFinished:(BOOL)_finished
{
	finished = _finished;
}

-(BOOL)isEmpty
{
	return [lines count] == 0;
}

-(NSString*)description
{
	return [NSString stringWithFormat:@"lines left: %d finished inputting: %d",[lines count],finished];
}
@end

@implementation SubLine
-(id)initWithLine:(NSString*)l start:(unsigned)s end:(unsigned)e
{
	if (self = [super init]) {
		int length = [l length];
		if (!length || [l characterAtIndex:length-1] != '\n') l = [l stringByAppendingString:@"\n"];
		line = [l retain];
		begin_time = s;
		end_time = e;
		no = 0;
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
	return [NSString stringWithFormat:@"\"%@\", from %d s to %d s",[line substringToIndex:[line length]-1],begin_time,end_time];
}
@end

@implementation VobSubSample

- (id)initWithTime:(long)time offset:(long)offset
{
	self = [super init];
	if(!self)
		return self;
	
	timeStamp = time;
	fileOffset = offset;
	
	return self;
}

@end

@implementation VobSubTrack

- (id)initWithPrivateData:(NSData *)idxPrivateData language:(NSString *)lang andIndex:(int)trackIndex
{
	self = [super init];
	if(!self)
		return self;
	
	privateData = [idxPrivateData retain];
	language = [lang retain];
	index = trackIndex;
	samples = [[NSMutableArray alloc] init];
	
	return self;
}

- (void)dealloc
{
	[privateData release];
	[language release];
	[samples release];
	[super dealloc];
}

- (void)addSample:(VobSubSample *)sample
{
	[samples addObject:sample];
}

- (void)addSampleTime:(long)time offset:(long)offset
{
	VobSubSample *sample = [[VobSubSample alloc] initWithTime:time offset:offset];
	[self addSample:sample];
	[sample release];
}

@end

#pragma mark C++ Wrappers

CXXSubSerializer::CXXSubSerializer()
{
	priv = [[SubSerializer alloc] init];
    CFRetain(priv);
	retainCount = 1;
}

CXXSubSerializer::~CXXSubSerializer()
{
	if (priv) {CFRelease(priv); [(SubSerializer*)priv release]; priv = NULL;}
}

void CXXSubSerializer::pushLine(const char *line, size_t size, unsigned start, unsigned end)
{
	NSMutableString *str = [[NSMutableString alloc] initWithBytes:line length:size encoding:NSUTF8StringEncoding];
	[str appendString:@"\n"];
	
	SubLine *sl = [[SubLine alloc] initWithLine:str start:start end:end];
	
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
	retainCount--;
	
	if (!retainCount)
		delete this;
}

void CXXSubSerializer::retain()
{
	retainCount++;
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
