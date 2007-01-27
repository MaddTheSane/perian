//
//  SSADocument.m
//  SSAView
//
//  Created by Alexander Strange on 1/18/07.
//  Copyright 2007 Perian Project. All rights reserved.
//

#import "SSADocument.h"
#import "Categories.h"
#include "SubImport.h"

@implementation SSAEvent
-(void)dealloc
{
	[line release];
	[super dealloc];
}
@end

@implementation SSADocument

static ATSURGBAlphaColor SSAParseColor(NSString *c)
{
	const char *c_ = [c UTF8String];
	unsigned char r, g, b, a;
	
	if (c_[0] == '&') {
		sscanf(c_ + 2,"%2hhx%2hhx%2hhx%2hhx",&a,&b,&g,&r);
		a = 255-a; // have to reverse it
	} else {
		unsigned int rgb = EndianU32_BtoN(strtoul(c_,NULL,0));
		
		b = rgb & 0xff;
		g = (rgb & 0xff00) >> 8;
		r = rgb >> 16;
		a = 0;
	}
	
	return (ATSURGBAlphaColor){(float)r/255.,(float)g/255.,(float)b/255.,(float)a/255.};
}

-(void)setupHeaders:(NSDictionary*)hDict
{
	NSString *field;
	float rX=-1, rY=-1;
	if (field = [hDict objectForKey:@"PlayResX"]) rX = [field doubleValue];
	if (field = [hDict objectForKey:@"PlayResY"]) rY = [field doubleValue];
	if (rX > 0 && rY == -1) {
		rY = rX * (3./4.);
	} else if (rX == -1 && rY > 0) {
		rX = rY * (4./3.);
	} else if (rX == -1 && rY == -1) {
		rX = 384; //magic numbers are ssa defaults
		rY = 288;
	}
	
	resX = rX; resY = rY;
	
	timescale = (field = [hDict objectForKey:@"Timer"])? [field doubleValue] / 100. : 1;
	collisiontype = Normal;
	if ((field = [hDict objectForKey:@"Collisions"]) && [field isEqualToString:@"Reverse"]) collisiontype = Reverse;
	if (field = [hDict objectForKey:@"ScriptType"]) {
		if ([field isEqualToString:@"v4.00+"]) version = S_ASS;
		else if ([field isEqualToString:@"v3.00"]) version = S_SSA;
		else {version = S_SSA; NSLog(@"unrecognized SSA version %@",field);}
	}
}

int SSA2ASSAlignment(int a)
{
	if (a >= 7 && a <= 9) return a-3;
	if (a >= 4 && a <= 6) return a+3;
	if (a > 9 || a < 1) return 2;
	return a;
}

-(void) makeATSUStylesForSSAStyle:(ssastyleline *)s
{
	{
		ATSUAttributeTag tags[] = {kATSUFontTag, kATSUFontMatrixTag, kATSUStyleRenderingOptionsTag, kATSUSizeTag, kATSUTrackingTag, kATSUQDBoldfaceTag, kATSUQDItalicTag, kATSUQDUnderlineTag, kATSUStyleStrikeThroughTag};
		ByteCount		 sizes[] = {sizeof(ATSUFontID), sizeof(CGAffineTransform), sizeof(ATSStyleRenderingOptions), sizeof(Fixed), sizeof(Fixed), sizeof(Boolean), sizeof(Boolean), sizeof(Boolean), sizeof(Boolean)};
		ATSUFontID	font;
		CGAffineTransform matrix;
		ATSStyleRenderingOptions opt = kATSStyleApplyAntiAliasing;
		Fixed size, tracking;
		
		ATSUAttributeValuePtr vals[] = {&font, &matrix, &opt, &size, &tracking, &s->bold, &s->italic, &s->underline, &s->strikeout};
		char fname[256] = {0};
		ByteCount fl = 256;
		
		font = FMGetFontFromATSFontRef(ATSFontFindFromName((CFStringRef)s->font,kATSOptionFlagsDefault));
		ATSUFindFontName(font,kFontFullName,kFontMacintoshPlatform,kFontNoScriptCode,kFontNoLanguage,fl,(char*)fname,&fl,NULL);
		//kFontUnicodePlatform and MicrosoftPlatform are both pretty broken. asian fonts aren't found sometimes. this is an apple bug.
		
		if (font == kInvalidFont)
			NSLog(@"error finding font named \"%@\"",s->font);
		//else NSLog(@"found font \"%s\" for name \"%@\"",fname,s->font); 
		
		matrix = CGAffineTransformMakeScale(s->scalex/100.,s->scaley/100.);
		
		float scalesize = s->fsize * (72./96.);
		size = FloatToFixed(scalesize); // scale from Windows 96 dpi size
		
		tracking = FloatToFixed(s->tracking); // i am really not sure about this!
		
		ATSUCreateStyle(&s->atsustyle);
		ATSUSetAttributes(s->atsustyle,sizeof(vals) / sizeof(ATSUAttributeValuePtr),tags,sizes,vals);
		
		ATSUFontFeatureType		ftype[] = {kLigaturesType,kLigaturesType};
		ATSUFontFeatureSelector fsel[] = {kCommonLigaturesOnSelector,kRareLigaturesOnSelector};
		
		ATSUSetFontFeatures(s->atsustyle,sizeof(fsel) / sizeof(ATSUFontFeatureSelector),ftype,fsel);
	}
	
	{
		ATSUAttributeTag tags[] = {kATSULineFlushFactorTag, kATSULineRotationTag, kATSULineWidthTag};
		ByteCount		 sizes[] = {sizeof(Fract), sizeof(Fract), sizeof(ATSUTextMeasurement)};
		Fract alignment, rotation = FloatToFract(s->angle);
		ATSUTextMeasurement width;
		ATSUAttributeValuePtr vals[] = {&alignment, &rotation, &width};
		
		switch(s->halign) {
			case S_LeftAlign:
				alignment = FloatToFract(0.);
				break;
			case S_CenterAlign: default:
				alignment = kATSUCenterAlignment;  
				break;
			case S_RightAlign: 
				alignment = FloatToFract(1.);
		}
		
		s->usablewidth = resX - s->marginl - s->marginr;
		width = IntToFixed(s->usablewidth);
		ATSUCreateTextLayout(&s->layout);
		ATSUSetLayoutControls(s->layout,sizeof(vals) / sizeof(ATSUAttributeValuePtr),tags,sizes,vals);
	}
}

#define fv(sn, fn) if (field = [style objectForKey:@"" # sn]) s.fn = [field doubleValue];
#define iv(sn, fn) if (field = [style objectForKey:@"" # sn]) s.fn = [field intValue];
#define sv(sn, fn) if (field = [style objectForKey:@"" # sn]) s.fn = [field retain];
#define cv(sn, fn) if (field = [style objectForKey:@"" # sn]) s.color.fn = SSAParseColor(field);
#define bv(sn, fn) if (field = [style objectForKey:@"" # sn]) s.fn = [field intValue] != 0;

-(void)setupStyles:(NSDictionary*)sDict
{
	if ([sDict count] > 0) {
		NSEnumerator *sEnum = [sDict objectEnumerator];
		NSDictionary *style; int i=0;
		
		stylecount = [sDict count];
		styles = malloc(sizeof(ssastyleline[stylecount]));
		while (style = [sEnum nextObject]) {
			ssastyleline s = {0}; NSString *field;
			
			sv(Name, name)
				sv(Fontname, font)
				fv(Fontsize, fsize)
				cv(PrimaryColour, primary)
				cv(SecondaryColour, secondary)
				cv(OutlineColour, outline)
				cv(BackColour, shadow)
				bv(Bold, bold)
				bv(Italic, italic)
				bv(Underline, underline)
				bv(StrikeOut, strikeout)
				fv(ScaleX, scalex)
				fv(ScaleY, scaley)
				fv(Spacing, tracking)
				fv(Angle, angle)
				iv(BorderStyle, borderstyle)
				fv(Outline, outline)
				fv(Shadow, shadow)
				iv(Alignment, alignment)
				iv(MarginL, marginl)
				iv(MarginR, marginr)
				iv(MarginV, marginv)
				
				if ([s.font length] > 512) s.font = @"Helvetica";
			
			if (version == S_SSA) s.alignment = SSA2ASSAlignment(s.alignment);
			switch (s.alignment) {case 1: case 4: case 7: s.halign = S_LeftAlign; break; case 2: case 5: case 8: default: s.halign = S_CenterAlign; break; case 3: case 6: case 9: s.halign = S_RightAlign;}
			switch (s.alignment) {case 1: case 2: case 3: default: s.valign = S_BottomAlign; break; case 4: case 5: case 6: s.valign = S_MiddleAlign; break; case 7: case 8: case 9: s.valign = S_TopAlign;}
			
			[self makeATSUStylesForSSAStyle:&s];
			
			styles[i] = s;
			i++;
		}
	}
}

-(SSAEvent *)movPacket:(int)i
{
	return [_lines objectAtIndex:i];
}

static unsigned ParseSSATime(NSString *str)
{
	int h,m,s,ms;
	const char *cs = [str UTF8String];
	
	sscanf(cs,"%d:%d:%d.%d",&h,&m,&s,&ms);
	return ms + s * 100 + m * 100 * 60 + h * 100 * 60 * 60;
}

static BOOL isinrange(unsigned base, unsigned test_s, unsigned test_e)
{
	return (base >= test_s) && (base < test_e);
}

static NSString *oneMKVPacket(NSDictionary *s)
{
	return [NSString stringWithFormat:@"%d,%d,%@,%@,%0.4d,%0.4d,%0.4d,%@,%@\n",
		[s objectForKey:@"ReadOrder"],
		[[s objectForKey:@"Layer"] intValue],
		[s objectForKey:@"Style"],
		[s objectForKey:@"Name"],
		[[s objectForKey:@"MarginL"] intValue],
		[[s objectForKey:@"MarginR"] intValue],
		[[s objectForKey:@"MarginV"] intValue],
		[s objectForKey:@"Effect"],
		[s objectForKey:@"Text"]];	
}

static int cmp_uint(const void *a, const void *b)
{
	unsigned av = *(unsigned*)a, bv = *(unsigned*)b;
	
	if (av > bv) return 1;
	if (av < bv) return -1;
	return 0;
}

typedef struct {
	NSString *line;
	unsigned start, end;
} line_range;

static int cmp_line(const void *a, const void *b)
{
	const line_range *av = a, *bv = b;
	
	if (av->start > bv->start) return 1;
	if (av->start < bv->start) return -1;
	return 0;
}

-(NSArray *)serializeSubLines:(NSMutableArray*)linesa
{
	int num = [linesa count];
	unsigned times[num*2];
	line_range lines[num];
	unsigned i, j;
	line_range li;
	NSMutableArray *outa = [[NSMutableArray alloc] init];
	
	for (i = 0; i < num; i++) {
		NSDictionary *l;
		NSString *s,*e;
		l = [linesa objectAtIndex:i];
		
		s = [l objectForKey:@"Start"];
		e = [l objectForKey:@"End"];
		
		li.line = oneMKVPacket(l);
		li.start = ParseSSATime(s);
		li.end = ParseSSATime(e);
		
		times[i*2] = li.start;
		times[i*2+1] = li.end;
		
		lines[i] = li;
	}
	
	qsort(times, num*2, sizeof(unsigned), cmp_uint);
	qsort(lines, num, sizeof(line_range), cmp_line);
	
	for (i = 0; i < num*2; i++) {
		if (i > 0 && times[i-1] == times[i]) continue;
		NSMutableString *accum = [NSMutableString string];
		unsigned start = times[i], end;
		
		for (j = 0; j < num; j++) {
			if (isinrange(times[i], lines[j].start, lines[j].end)) {
				end = lines[j].end;
				[accum appendString:lines[j].line];
			}
		}
		
		if ([accum length] > 0) {
			[accum deleteCharactersInRange:NSMakeRange([accum length] - 1, 1)]; // delete last newline
			SSAEvent *event = [[[SSAEvent alloc] init] autorelease];
			
			if (timescale != 1.) {
				double ds = start, de = end;
				ds *= timescale;
				de *= timescale;
				start = ds;
				end = de;
			}
			event->begin_time = start;
			event->end_time = end;
			event->line = [accum retain];
			
			[outa addObject:event];
		}
	}
	
	return outa;
}

-(void) loadFile:(NSString*)path
{
	NSError *err;
	NSStringEncoding se = NSUTF8StringEncoding;
	NSString *ssa = [[NSString stringWithContentsOfFile:path encoding:se error:&err] stringByStandardizingNewlines];
	if (!ssa) return;
	NSArray *lines = [ssa componentsSeparatedByString:@"\n"];
	NSEnumerator *lenum = [lines objectEnumerator];
	NSString *nextLine, *styleType, *ns;
	NSArray *format;
	NSMutableDictionary *headers, *styleDict;
	NSMutableArray *doclines;
	NSCharacterSet *ws = [NSCharacterSet whitespaceCharacterSet];
	unichar cai;
	int formatc;
	int readorder = 0;
	
	headers = [[NSMutableDictionary alloc] init];
	styleDict = [NSMutableDictionary dictionary];
	doclines = [NSMutableArray array];
	
	if (![(NSString*)[lenum nextObject] isEqualToString:@"[Script Info]"]) return;
	while (1) {
		ns = (NSString*)[lenum nextObject];
		if ([ns length] == 0) continue;
		else {
			cai = [ns characterAtIndex:0];
			
			if (cai == ';') continue;
			else if (cai == '[') {nextLine = ns; break;}
			NSArray *pair = [ns pairSeparatedByString:@": "];
			if ([pair count] == 2) [headers setObject:[(NSString*)[pair objectAtIndex:1] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] forKey:[pair objectAtIndex:0]];
		}
		
	}
	
	[self setupHeaders:headers];

	while (![nextLine isEqualToString:@"[Events]"]) nextLine = [lenum nextObject];
	while ([nextLine length] == 0) nextLine = [lenum nextObject];
	nextLine = [lenum nextObject];
	
	NSArray *pair = [nextLine pairSeparatedByString:@": "];
	NSString *formatstring = (version == S_ASS) ?
		@"Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text"
																 :
		@"Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text";
	
	if ([pair count] == 2 && [(NSString*)[pair objectAtIndex:0] isEqualToString:@"Format"]) {formatstring = (NSString*)[pair objectAtIndex:1]; ns = [lenum nextObject];} else ns = nextLine;
	format = [[formatstring stringByTrimmingCharactersInSet:ws] componentsSeparatedByString:@", "];
	formatc = [format count];
	
	while (1) {
		if (!ns) break; 
		else if ([ns length] != 0) {
			pair = [ns pairSeparatedByString:@": "];
			if ([pair count] == 2 && [(NSString*)[pair objectAtIndex:0] isEqualToString:@"Dialogue"]) {
				NSArray *curl = [(NSString*)[pair objectAtIndex:1] componentsSeparatedByString:@"," count:formatc];
				int count = MIN([format count], [curl count]), i;
				NSMutableDictionary *lDict = [NSMutableDictionary dictionary];
				for (i=0; i < count; i++) [lDict setObject:[curl objectAtIndex:i] forKey:[format objectAtIndex:i]];
				[lDict setObject:[NSString stringWithFormat:@"%d",readorder++] forKey:@"ReadOrder"];
				[doclines addObject:lDict];
			}
		}
		ns = [lenum nextObject];
	}
	
	_lines = [self serializeSubLines:doclines];
	
	header = [ssa substringToIndex:[ssa rangeOfString:@"[Events]" options:NSLiteralSearch].location];
}

-(void) loadHeader:(NSString*)path
{
	NSError *err;
	NSStringEncoding se = NSUTF8StringEncoding;
	NSString *ssa = [[NSString stringWithContentsOfFile:path encoding:se error:&err] stringByStandardizingNewlines];
	if (!ssa) return;
	NSArray *lines = [ssa componentsSeparatedByString:@"\n"];
	NSEnumerator *lenum = [lines objectEnumerator];
	NSString *nextLine, *styleType, *ns;
	NSArray *format;
	NSMutableDictionary *headers, *styleDict;
	NSMutableArray *doclines;
	NSCharacterSet *ws = [NSCharacterSet whitespaceCharacterSet];
	unichar cai;
	int formatc;
	int readorder = 0;
	
	headers = [[NSMutableDictionary alloc] init];
	styleDict = [NSMutableDictionary dictionary];
	doclines = [NSMutableArray array];
	
	if (![(NSString*)[lenum nextObject] isEqualToString:@"[Script Info]"]) return;
	while (1) {
		ns = (NSString*)[lenum nextObject];
		if ([ns length] == 0) continue;
		else {
			cai = [ns characterAtIndex:0];
			
			if (cai == ';') continue;
			else if (cai == '[') {nextLine = ns; break;}
			NSArray *pair = [ns pairSeparatedByString:@": "];
			if ([pair count] == 2) [headers setObject:[(NSString*)[pair objectAtIndex:1] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] forKey:[pair objectAtIndex:0]];
		}
		
	}
	
	[self setupHeaders:headers];
	
	while (!([nextLine isEqualToString:@"[V4 Styles]"] || [nextLine isEqualToString:@"[V4+ Styles]"])) nextLine = [lenum nextObject];
	styleType = nextLine;
	while ([nextLine length] == 0) nextLine = [lenum nextObject];
	nextLine = [lenum nextObject];
	
	NSArray *pair = [nextLine pairSeparatedByString:@": "];
	NSString *formatstring = ([styleType isEqualToString:@"[V4+ Styles]"]) ?
		@"Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding"
																		   :
		@"Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, TertiaryColour, BackColour, Bold, Italic, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, AlphaLevel, Encoding";
	
	if ([pair count] == 2 && [(NSString*)[pair objectAtIndex:0] isEqualToString:@"Format"]) {formatstring = (NSString*)[pair objectAtIndex:1]; ns = [lenum nextObject];} else ns = nextLine;
	
	format = [[formatstring stringByTrimmingCharactersInSet:ws] componentsSeparatedByString:@", "];
	formatc = [format count];
	
	while (1) {
		if ([ns length] != 0) {
			cai = [ns characterAtIndex:0];
			
			if (cai == '[') {nextLine = ns; break;}
			if (cai != ';' && cai != '!') {
				NSArray *pair = [ns pairSeparatedByString:@": "];
				if ([pair count] == 2) {
					NSArray *style = [(NSString*)[pair objectAtIndex:1] componentsSeparatedByString:@"," count:formatc]; // bug in SSA: font names with , break it
					int count = MIN([format count], [style count]), i;
					NSMutableDictionary *styled = [NSMutableDictionary dictionary];
					for (i=0; i < count; i++) [styled setObject:[style objectAtIndex:i] forKey:[format objectAtIndex:i]];
					[styleDict setObject:styled forKey:[styled objectForKey:@"Name"]];
				}
			}
		}
		ns = [lenum nextObject];
	}
	
	[self setupStyles:styleDict];
}

-(NSString*)header
{
	return header;
}

-(unsigned)packetCount
{
	return [_lines count];
}
@end

ComponentResult LoadSubStationAlphaSubtitles(const FSRef *theDirectory, CFStringRef filename, Movie theMovie, Track *firstSubTrack)
{
	ComponentResult err = noErr;
	Handle dataRef = NULL;
	OSType dataRefType = rAliasType;
	HFSUniStr255 hfsFilename;
	CFRange filenameLen;
	Track theTrack = NULL;
	Media theMedia = NULL;
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	SSADocument *ssa = [[SSADocument alloc] init];
	static UInt8 path[PATH_MAX];
	Handle sampleHndl = NULL, headerHndl=NULL, drefHndl = NewHandle(sizeof(Handle)), drefDataH = NewHandle(0);
	ComponentInstance dataHandler = NULL;
	long filePos = 0;
	long fileSize;
	char *data = NULL; const char *header;
	int i, packetCount, sampleLen;
	ImageDescriptionHandle textDesc;
	
	FSRefMakePath(theDirectory, path, PATH_MAX);
	
	[ssa loadFile:[[NSString stringWithUTF8String:(char*)path] stringByAppendingPathComponent:(NSString*)filename]];
	
	packetCount = [ssa packetCount];
	
	textDesc = (ImageDescriptionHandle) NewHandleClear(sizeof(ImageDescription));
	
	header = [[ssa header] UTF8String];
	sampleLen = strlen(header);
	PtrToHand(header, &headerHndl, sampleLen);
	
	*((Handle*)*drefHndl) = drefDataH;
		
	theTrack = CreatePlaintextSubTrack(theMovie, textDesc, 100, drefHndl, HandleDataHandlerSubType, (ssa->version==S_ASS) ? kSubFormatASS : kSubFormatSSA);
	if (theTrack == NULL) {
		err = GetMoviesError();
		goto bail;
	}
	
	AddImageDescriptionExtension(textDesc,headerHndl, (ssa->version==S_ASS) ? kSubFormatASS : kSubFormatSSA);

	theMedia = GetTrackMedia(theTrack);
	if (theMedia == NULL) {
		err = GetMoviesError();
		goto bail;
	}
	
	BeginMediaEdits(theMedia);
	
	for (i = 0; i < packetCount; i++) {
		SSAEvent *p = [ssa movPacket:i];
		const char *str = [p->line UTF8String];
		sampleLen = strlen(str);
		
		PtrToHand(str,&sampleHndl,sampleLen);

		err=AddMediaSample(theMedia,sampleHndl,0,sampleLen, p->end_time - p->begin_time,(SampleDescriptionHandle)textDesc, 1, 0, NULL);
		if (err != noErr) {NSLog(@"a %d",GetMoviesError()); goto bail;}
		//note on -1: we sorted the subtitles already, so we know they can be added to the end of the track.
		InsertMediaIntoTrack(theTrack,-1,p->begin_time,p->end_time - p->begin_time,fixed1);
		if (err != noErr) NSLog(@"i %d",GetMoviesError());

		DisposeHandle(sampleHndl);
	}
	
	EndMediaEdits(theMedia);
		
	if (*firstSubTrack == NULL)
		*firstSubTrack = theTrack;
	else
		SetTrackAlternate(*firstSubTrack, theTrack);
	
	SetMediaLanguage(theMedia, GetFilenameLanguage(filename));
	
bail:
		
	[ssa release];
	[pool release];
	
	if (err) {
		if (theMedia)
			DisposeTrackMedia(theMedia);
		
		if (theTrack)
			DisposeMovieTrack(theTrack);
	}
	
	if (textDesc)
		DisposeHandle((Handle) textDesc);
	
	if (headerHndl) DisposeHandle((Handle)headerHndl);
	//DisposeHandle((Handle)drefDataH);
	DisposeHandle((Handle)drefHndl);
	
	return err;
}
