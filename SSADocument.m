/*
 *  SSARenderCodec.m
 *  Copyright (c) 2007 Perian Project
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; 
 *  version 2.1 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#import "SSADocument.h"
#import "Categories.h"
#include "SubImport.h"
#include "Codecprintf.h"

@implementation SSADocument

// This line contains the default style, for SRT files.

static ssastyleline SSA_DefaultStyle = (ssastyleline){
	@"Default",@"Helvetica",
	32 * (96./72.),
	{{1,1,1,1},{1,1,1,1},{0,0,0,1},{0,0,0,1}}, // white on black
	1,0,0,0,
	100,100,0,0,
	0,1.5,1.5,
	2,1,0,
	10,10,10,
	NULL, NULL, 0
};

-(void)dealloc
{
	int i;
	if (disposedefaultstyle) free(defaultstyle);
	for (i=0; i < stylecount; i++) {
		ATSUDisposeStyle(styles[i].atsustyle);
		ATSUDisposeTextLayout(styles[i].layout);
		[styles[i].name release];
		[styles[i].font release];
	}
	free(styles);
	
	[_lines release];
	[header release];
	[super dealloc];
}

static ATSURGBAlphaColor SSAParseColor(NSString *c)
{
	const char *c_ = [c UTF8String];
	unsigned char r, g, b, a;
	unsigned int rgb;
	
	if (c_[0] == '&') {
		rgb = strtoul(&c_[2],NULL,16);
	} else {
		rgb = strtol(c_,NULL,0);
	}
	
	a = (rgb >> 24) & 0xff;
	b = (rgb >> 16) & 0xff;
	g = (rgb >> 8) & 0xff;
	r = rgb & 0xff;

	a = 255-a;
	
	return (ATSURGBAlphaColor){(float)r/255.,(float)g/255.,(float)b/255.,(float)a/255.};
}

-(void)setupHeaders:(NSDictionary*)hDict width:(float)width height:(float)height
{
	NSString *field;
	float rX=-1, rY=-1, aspect = width / height;
	if (field = [hDict objectForKey:@"PlayResX"]) rX = [field doubleValue];
	if (field = [hDict objectForKey:@"PlayResY"]) rY = [field doubleValue];
	if (rX > 0 && rY == -1) {
		rY = rX / aspect;
	} else if (rX == -1 && rY > 0) {
		rX = rY * aspect;
	} else if (rX == -1 && rY == -1) {
		rX = 384; //magic numbers are ssa defaults
		rY = 288;
	}
	
	resX = rX; resY = rY;
	
	timescale = (field = [hDict objectForKey:@"Timer"])? [field doubleValue] / 100. : 1.;
	collisiontype = NormalCollisions;
	if ((field = [hDict objectForKey:@"Collisions"]) && [field isEqualToString:@"ReverseCollisions"]) collisiontype = ReverseCollisions;
	if (field = [hDict objectForKey:@"ScriptType"]) {
		if ([field isEqualToString:@"v4.00+"]) version = S_ASS;
		else version = S_SSA;
	}
}

int SSA2ASSAlignment(int a)
{
    int h = 1, v = 0;
	if (a >= 9 && a <= 11) {v = S_MiddleAlign; h = a-8;}
	if (a >= 5 && a <= 7)  {v = S_TopAlign;    h = a-4;}
	if (a >= 1 && a <= 3)  {v = S_BottomAlign; h = a;}
	return v * 3 + h;
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
//		char fname[256] = {0};
//		ByteCount fl = 256;
		
		font = FMGetFontFromATSFontRef(ATSFontFindFromName((CFStringRef)s->font,kATSOptionFlagsDefault));
		//ATSUFindFontName(font,kFontFullName,kFontMacintoshPlatform,kFontNoScriptCode,kFontNoLanguage,fl,(char*)fname,&fl,NULL);
		//kFontUnicodePlatform and MicrosoftPlatform are both pretty broken. asian fonts aren't found sometimes. this is an apple bug.
		
		if (font == kInvalidFont)
			font = FMGetFontFromATSFontRef(ATSFontFindFromName((CFStringRef)@"Helvetica",kATSOptionFlagsDefault));
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
		ATSUAttributeTag tags[] = {kATSULineFlushFactorTag, kATSULineWidthTag};
		ByteCount		 sizes[] = {sizeof(Fract), sizeof(ATSUTextMeasurement)};
		Fract alignment;
		ATSUTextMeasurement width;
		ATSUAttributeValuePtr vals[] = {&alignment, &width};
		
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
	defaultstyle = NULL;

	if ([sDict count] > 0) {
		NSEnumerator *sEnum = [sDict objectEnumerator];
		NSDictionary *style; int i=0;
		
		stylecount = [sDict count];
		styles = malloc(sizeof(ssastyleline[stylecount]));
		while (style = [sEnum nextObject]) {
			ssastyleline s = {0}; NSString *field;
			
			s.scalex = s.scaley = 100;
			
			sv(Name, name)
				sv(Fontname, font)
				fv(Fontsize, fsize)
				cv(PrimaryColour, primary)
				cv(SecondaryColour, secondary)
				cv(OutlineColour, outline)
			//	cv(TertiaryColour, outline)
				if (version == S_SSA) cv(BackColour, outline)
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
			if (!defaultstyle && [styles[i].name isEqualToString:@"Default"]) {defaultstyle = &styles[i]; disposedefaultstyle = NO;}
			i++;
		}
	}
	
	if (!defaultstyle) {
		disposedefaultstyle = YES;
		defaultstyle = malloc(sizeof(ssastyleline));
		*defaultstyle = SSA_DefaultStyle;
		[self makeATSUStylesForSSAStyle:defaultstyle];
	}
}

-(SubLine *)movPacket:(int)i
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
		[[s objectForKey:@"ReadOrder"] intValue],
		[[s objectForKey:@"Layer"] intValue],
		[s objectForKey:@"Style"],
		[s objectForKey:@"Name"],
		[[s objectForKey:@"MarginL"] intValue],
		[[s objectForKey:@"MarginR"] intValue],
		[[s objectForKey:@"MarginV"] intValue],
		[s objectForKey:@"Effect"],
		[s objectForKey:@"Text"]];	
}

-(NSArray *)serializeSubLines:(NSMutableArray*)linesa
{
	int num = [linesa count], i;
	NSMutableArray *outa = [[NSMutableArray alloc] init];
	SubtitleSerializer *serializer = [[SubtitleSerializer alloc] init];
	SubLine *sl;
	
	for (i = 0; i < num; i++) {
		NSDictionary *l = [linesa objectAtIndex:i];;
		NSString *s,*e;
		SubLine *li;
		
		s = [l objectForKey:@"Start"];
		e = [l objectForKey:@"End"];
		
		li = [[[SubLine alloc] initWithLine:oneMKVPacket(l) start:ParseSSATime(s) end:ParseSSATime(e)] autorelease];
		
		if (timescale != 1.) {
			li->begin_time *= timescale;
			li->end_time *= timescale;
		}

		[serializer addLine:li];
	}
	
	[serializer setFinished:YES];
	
	while (sl = [serializer getSerializedPacket]) {
		[outa addObject:sl];
	}
	
	[serializer release];
	return outa;
}

-(ComponentResult) loadFile:(NSString*)path
{
	NSString *ssa = [[NSString stringFromUnknownEncodingFile:path] stringByStandardizingNewlines];
	if (!ssa) return -1;
	NSArray *lines = [ssa componentsSeparatedByString:@"\n"];
	NSEnumerator *lenum = [lines objectEnumerator];
	NSString *nextLine, *ns;
	NSArray *format;
	NSMutableDictionary *headers, *styleDict;
	NSMutableArray *doclines;
	NSCharacterSet *ws = [NSCharacterSet whitespaceAndBomCharacterSet];
	unichar cai;
	int formatc;
	int readorder = 0;
	
	headers = [[NSMutableDictionary alloc] init];
	styleDict = [NSMutableDictionary dictionary];
	doclines = [NSMutableArray array];
	
	ns = [(NSString*)[lenum nextObject] stringByTrimmingCharactersInSet:ws];
	if (![ns isEqualToString:@"[Script Info]"]) return -1;
	while (1) {
		ns = (NSString*)[lenum nextObject];
		if (!ns || [ns length] == 0) continue;
		else {
			cai = [ns characterAtIndex:0];
			
			if (cai == ';') continue;
			else if (cai == '[') {nextLine = ns; break;}
			NSArray *pair = [ns pairSeparatedByString:@": "];
			if ([pair count] == 2) [headers setObject:[(NSString*)[pair objectAtIndex:1] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] forKey:[pair objectAtIndex:0]];
		}
		
	}
	
	[self setupHeaders:headers width:640 height:480];

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
	
	header = [[ssa substringToIndex:[ssa rangeOfString:@"[Events]" options:NSLiteralSearch].location] retain];
	
	return noErr;
}

-(void) loadHeader:(NSString*)ssa width:(float)width height:(float)height
{
	if (!ssa) return;
	NSArray *lines = [[ssa stringByStandardizingNewlines] componentsSeparatedByString:@"\n"];
	NSEnumerator *lenum = [lines objectEnumerator];
	NSString *nextLine, *styleType, *ns;
	NSArray *format;
	NSMutableDictionary *headers, *styleDict;
	NSMutableArray *doclines;
	NSCharacterSet *ws = [NSCharacterSet whitespaceAndBomCharacterSet];
	unichar cai;
	int formatc;
	
	headers = [NSMutableDictionary dictionary];
	styleDict = [NSMutableDictionary dictionary];
	doclines = [NSMutableArray array];
	
	nextLine = [[lenum nextObject] stringByTrimmingCharactersInSet:ws];
	
	if (![nextLine isEqualToString:@"[Script Info]"]) {
		Codecprintf(stderr,"The line \"%s\" isn't a valid SSA header\n",[nextLine UTF8String]);
		return;
	}
	while (1) {
		ns = (NSString*)[lenum nextObject];
		if (!ns || [ns length] == 0) continue;
		else {
			cai = [ns characterAtIndex:0];
			
			if (cai == ';') continue;
			else if (cai == '[') {nextLine = ns; break;}
			NSArray *pair = [ns pairSeparatedByString:@": "];
			if ([pair count] == 2) [headers setObject:[(NSString*)[pair objectAtIndex:1] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] forKey:[pair objectAtIndex:0]];
		}
		
	}
	
	[self setupHeaders:headers width:width height:height];
	
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
		if (!ns) break;
		else if ([ns length] != 0) {
			cai = [ns characterAtIndex:0];
			
			if (cai == '[') {nextLine = ns; break;}
			if (cai != ';' && cai != '!') {
				NSArray *pair = [ns pairSeparatedByString:@": "];
				if ([pair count] == 2) {
					NSArray *style = [(NSString*)[pair objectAtIndex:1] componentsSeparatedByString:@"," count:formatc]; // bug in SSA: font names with , break it
					int count = MIN([format count], [style count]), i;
					NSMutableDictionary *styled = [NSMutableDictionary dictionary];
					for (i=0; i < count; i++) [styled setObject:[[style objectAtIndex:i] stringByTrimmingCharactersInSet:ws] forKey:[format objectAtIndex:i]];
					[styleDict setObject:styled forKey:[styled objectForKey:@"Name"]];
				}
			}
		}
		ns = [lenum nextObject];
	}
	
	[self setupStyles:styleDict];
}

-(void)loadDefaultsWithWidth:(float)width height:(float)height
{
	stylecount = 0;
	styles = malloc(0);
	defaultstyle = malloc(sizeof(ssastyleline));
	*defaultstyle = SSA_DefaultStyle;
	disposedefaultstyle = YES;
	resX = (width / height) * 480.; resY = 480;
	timescale = 1;
	collisiontype = NormalCollisions;
	version = S_ASS;
	
	[self makeATSUStylesForSSAStyle:defaultstyle];
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
	Track theTrack = NULL;
	Media theMedia = NULL;
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	SSADocument *ssa = [[SSADocument alloc] init];
	Handle sampleHndl = NULL, headerHndl=NULL, drefHndl = NULL;
	const char *header;
	int i, packetCount, sampleLen;
	ImageDescriptionHandle textDesc = NULL;
	Rect movieBox;
	UInt32 emptyDataRefExtension[2];
	TimeScale movieTimeScale = GetMovieTimeScale(theMovie);
	UInt8 *path = malloc(PATH_MAX);
	NSString *nspath;

	FSRefMakePath(theDirectory, path, PATH_MAX);
	nspath = [[NSString stringWithUTF8String:(char*)path] stringByAppendingPathComponent:(NSString*)filename];
	free(path);
	
	if ([ssa loadFile:nspath]) {
		Codecprintf(NULL,"Unable to load SSA file \"%s\"\n",[nspath UTF8String]);
		err = -1;
		goto bail;
	}
	
	packetCount = [ssa packetCount];
	
	textDesc = (ImageDescriptionHandle) NewHandleClear(sizeof(ImageDescription));
	
	header = [[ssa header] UTF8String];
	sampleLen = strlen(header);
	PtrToHand(header, &headerHndl, sampleLen);
	
	drefHndl = NewHandleClear(sizeof(Handle) + 1);
	
	/* Explanation of next three lines:
	 * The magic 'data' atom, added to an in-memory data handler, allows its data to be saved by value to a reference movie.
	 * It does this by saving the whole thing in the MOV header.
	 * This is imperfectly documented.
	 */
	emptyDataRefExtension[0] = EndianU32_NtoB(sizeof(UInt32)*2);
	emptyDataRefExtension[1] = EndianU32_NtoB(kDataRefExtensionInitializationData);
	
	PtrAndHand(&emptyDataRefExtension[0], drefHndl, sizeof(emptyDataRefExtension));
		
	GetMovieBox(theMovie,&movieBox);
	theTrack = CreatePlaintextSubTrack(theMovie, textDesc, 100, drefHndl, HandleDataHandlerSubType, 'SSA ', headerHndl, movieBox);
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
	
	for (i = 0; i < packetCount; i++) {
		SubLine *p = [ssa movPacket:i];
		TimeRecord movieStartTime = {SInt64ToWide(p->begin_time), 100, 0};
		TimeValue sampleTime;
		const char *str = [p->line UTF8String];
		sampleLen = strlen(str);
		
		PtrToHand(str,&sampleHndl,sampleLen);

		err=AddMediaSample(theMedia,sampleHndl,0,sampleLen, p->end_time - p->begin_time,(SampleDescriptionHandle)textDesc, 1, 0, &sampleTime);
		if (err != noErr) {err = GetMoviesError(); Codecprintf(stderr,"external SSA: error %d adding %d-%d line\n",err, p->begin_time, p->end_time); goto loopend;}
		
		ConvertTimeScale(&movieStartTime, movieTimeScale);

		err = InsertMediaIntoTrack(theTrack, movieStartTime.value.lo, sampleTime, p->end_time - p->begin_time, fixed1);
		if (err != noErr) {goto loopend;}

loopend:
		err = noErr;
		DisposeHandle(sampleHndl);
	}
	
	sampleHndl = NULL;
	
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
	if (sampleHndl) DisposeHandle(sampleHndl);
	if (drefHndl)   DisposeHandle((Handle)drefHndl);
	
	return err;
}
