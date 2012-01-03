/*
 * SubParsing.m.rl
 * Created by Alexander Strange on 7/25/07.
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

/*
 * Parsing of SSA/ASS subtitle files using Ragel.
 * At the moment, all subtitle formats supported by Perian
 * are converted to SSA before reaching here.
 * Feel free to implement new file formats as Ragel parsers here
 * if it ends up cleaner than doing it by hand.
 *
 * SSA specification (as it exists):
 * http://google.com/codesearch/p?hl=en#_g4u1OIsR_M/trunk/src/subtitles/STS.cpp&q=package:vsfilter%20%22v4%22&sa=N&cd=7&ct=rc&l=1395
 * http://moodub.free.fr/video/ass-specs.doc 
 *
 * FIXME:
 * - Files which can't be parsed have no clear error messages.
 * - Line and section names are case-insensitive in VSFilter, but we
 *   assume they are capitalized as in Aegisub.
 * - SSA v4.00++ is not supported.
 */

#import "SubParsing.h"
#import "SubRenderer.h"
#import "SubUtilities.h"
#import "SubContext.h"
#import "Codecprintf.h"

%%machine SSAfile;
%%write data;

SubRGBAColor SubParseSSAColor(unsigned rgb)
{
	unsigned char r, g, b, a;
	
	a = (rgb >> 24) & 0xff;
	b = (rgb >> 16) & 0xff;
	g = (rgb >> 8) & 0xff;
	r = rgb & 0xff;
	
	a = 255-a;
	
	return (SubRGBAColor){r/255.,g/255.,b/255.,a/255.};
}

SubRGBAColor SubParseSSAColorString(NSString *c)
{
	const char *c_ = [c UTF8String];
	unsigned int rgb;
	
	if (c_[0] == '&') {
		rgb = strtoul(&c_[2],NULL,16);
	} else {
		rgb = strtol(c_,NULL,0);
	}
	
	return SubParseSSAColor(rgb);
}

UInt8 SubASSFromSSAAlignment(UInt8 a)
{
    int h = 1, v = 0;
	if (a >= 9 && a <= 11) {v = kSubAlignmentMiddle; h = a-8;}
	if (a >= 5 && a <= 7)  {v = kSubAlignmentTop;    h = a-4;}
	if (a >= 1 && a <= 3)  {v = kSubAlignmentBottom; h = a;}
	return v * 3 + h;
}

void SubParseASSAlignment(UInt8 a, UInt8 *alignH, UInt8 *alignV)
{
	switch (a) {
		default: case 1 ... 3: *alignV = kSubAlignmentBottom; break;
		case 4 ... 6: *alignV = kSubAlignmentMiddle; break;
		case 7 ... 9: *alignV = kSubAlignmentTop; break;
	}
	
	switch (a) {
		case 1: case 4: case 7: *alignH = kSubAlignmentLeft; break;
		default: case 2: case 5: case 8: *alignH = kSubAlignmentCenter; break;
		case 3: case 6: case 9: *alignH = kSubAlignmentRight; break;
	}
}

BOOL SubParseFontVerticality(NSString **fontname)
{
	if ([*fontname length] && [*fontname characterAtIndex:0] == '@') {
		*fontname = [*fontname substringFromIndex:1];
		return YES;
	}
	return NO;
}

@implementation SubRenderSpan
-(SubRenderSpan*)copyWithZone:(NSZone*)zone
{
	SubRenderSpan *span = [[SubRenderSpan alloc] init];
	span->offset = offset;
	span.extra   = [[extra copy] autorelease];
	return span;
}

@synthesize extra;

-(void)dealloc
{
	[extra release];
	[super dealloc];
}

-(NSString*)description
{
	return [NSString stringWithFormat:@"Span at %d: %@", offset, extra];
}
@end

@implementation SubRenderDiv
-(NSString*)description
{
	int i, sc = [spans count];
	NSMutableString *tmp = [NSMutableString stringWithFormat:@"div \"%@\" with %d spans:", text, sc];
	for (i = 0; i < sc; i++) {[tmp appendFormat:@" %d",((SubRenderSpan*)[spans objectAtIndex:i])->offset];}
	[tmp appendFormat:@" %d", [text length]];
	return tmp;
}

-(SubRenderDiv*)init
{
	if (self = [super init]) {
		text      = nil;
		styleLine = nil;
		marginL   = marginR = marginV = layer = 0;
		spans     = nil;
		
		posX   = posY = 0;
		alignH = kSubAlignmentMiddle; alignV = kSubAlignmentBottom;
		
		positioned = NO;
		render_complexity = 0;
	}
	
	return self;
}

-(void)dealloc
{
	[text release];
	[styleLine release];
	[spans release];
	[super dealloc];
}
@end

extern BOOL IsScriptASS(NSDictionary *headers);

static NSArray *SplitByFormat(NSString *format, NSArray *lines)
{
	NSArray *formarray = SubSplitStringIgnoringWhitespace(format,@",");
	int i, numlines = [lines count], numfields = [formarray count];
	NSMutableArray *ar = [NSMutableArray arrayWithCapacity:numlines];
	
	for (i = 0; i < numlines; i++) {
		NSString *s = [lines objectAtIndex:i];
		NSArray *splitline = SubSplitStringWithCount(s, @",", numfields);
		
		if ([splitline count] != numfields) continue;
		[ar addObject:[NSDictionary dictionaryWithObjects:splitline
												  forKeys:formarray]];
	}
	
	return ar;
}

void SubParseSSAFile(NSString *ssastr, NSDictionary **headers, NSArray **styles, NSArray **subs)
{
	int len = [ssastr length];
	NSData *ssaData;
	const unichar *ssa = SubUnicodeForString(ssastr, &ssaData);
	NSMutableDictionary *headerdict = [NSMutableDictionary dictionary];
	NSMutableArray *stylearr = [NSMutableArray array], *eventarr = [NSMutableArray array], *cur_array=NULL;
	NSCharacterSet *wcs = [NSCharacterSet whitespaceCharacterSet];
	NSString *str=NULL, *styleformat=NULL, *eventformat=NULL;
	BOOL is_ass = NO;
	
	const unichar *p = ssa, *pe = ssa + len, *strbegin = p;
	int cs=0;
	
#define send() [[[NSString alloc] initWithCharactersNoCopy:(unichar*)strbegin length:p-strbegin freeWhenDone:NO] autorelease]
	
	%%{
		alphtype unsigned short;
		
		action sstart {strbegin = p;}
		action setheaderval {[headerdict setObject:send() forKey:str];}
		action savestr {str = send();}
		action csvlineend {[cur_array addObject:[send() stringByTrimmingCharactersInSet:wcs]];}
		action setupstyles {
			cur_array=stylearr;
			is_ass = IsScriptASS(headerdict);
			styleformat = is_ass ?
				@"Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding"
			   :@"Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, TertiaryColour, BackColour, Bold, Italic, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, AlphaLevel, Encoding";
		}
		action setupevents {
			cur_array=eventarr;
			eventformat = is_ass ?
				@"Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text"
			   :@"Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text";
		}
				
		nl = ("\n" | "\r" | "\r\n");
		str = any*;
		comment = ";" :> str;
		ws = space | 0xa0;
		bom = 0xfeff;
		
		keyvalueline = str >sstart %savestr :> (":" ws* %sstart str %setheaderval);
		headerline = (keyvalueline | comment | str) :> nl;
		header = "[" [Ss] "cript " [Ii] "nfo]" ws* nl headerline*;
		
		styleline = (("Style:" ws* %sstart str %csvlineend) | str) :> nl;
		styles = ("[" [Vv] "4" "+"? " " [Ss] "tyles]") %setupstyles ws* nl styleline*;
		
		event_txt = (("Dialogue:" ws* %sstart str %csvlineend %/csvlineend) | str);
		event = event_txt :> nl;
			
		lines = "[" [Ee] "vents]" %setupevents ws* nl event*;
		
		main := bom? header styles lines?;
	}%%
		
	%%write init;
	%%write exec;
	%%write eof;

	[ssaData release];

	*headers = headerdict;
	if (styles) *styles = SplitByFormat(styleformat, stylearr);
	if (subs) *subs = SplitByFormat(eventformat, eventarr);
}

%%machine SSAtag;
%%write data;

NSArray *SubParsePacket(NSString *packet, SubContext *context, SubRenderer *delegate)
{
	packet = SubStandardizeStringNewlines(packet);
	NSArray *lines = (context->scriptType == kSubTypeSRT) ? [NSArray arrayWithObject:[packet substringToIndex:[packet length]-1]] : [packet componentsSeparatedByString:@"\n"];
	size_t line_count = [lines count];
	NSMutableArray *divs = [NSMutableArray arrayWithCapacity:line_count];
	int i;
	
	for (i = 0; i < line_count; i++) {
		NSString *inputText = [lines objectAtIndex:(context->collisions == kSubCollisionsReverse) ? (line_count - i - 1) : i];
		SubRenderDiv *div = [[[SubRenderDiv alloc] init] autorelease];
		NSMutableString *text = [[NSMutableString alloc] init];
		NSMutableArray *spans = [[NSMutableArray alloc] init];
		
		div->text  = text;
		div->spans = spans;
		
		if (context->scriptType == kSubTypeSRT) {
			div->styleLine = [context->defaultStyle retain];
			div->marginL = div->styleLine->marginL;
			div->marginR = div->styleLine->marginR;
			div->marginV = div->styleLine->marginV;
			div->layer = 0;
			div->wrapStyle = kSubLineWrapTopWider;
		} else {
			NSArray *fields = SubSplitStringWithCount(inputText, @",", 9);
			if ([fields count] < 9) continue;
			div->layer = [[fields objectAtIndex:1] intValue];
			div->styleLine = [[context styleForName:[fields objectAtIndex:2]] retain];
			div->marginL = [[fields objectAtIndex:4] intValue];
			div->marginR = [[fields objectAtIndex:5] intValue];
			div->marginV = [[fields objectAtIndex:6] intValue];
			inputText = [fields objectAtIndex:8];
			if ([inputText length] == 0) continue;
			
			if (div->marginL == 0) div->marginL = div->styleLine->marginL;
			if (div->marginR == 0) div->marginR = div->styleLine->marginR;
			if (div->marginV == 0) div->marginV = div->styleLine->marginV;
			
			div->wrapStyle = context->wrapStyle;
		}
		
		div->alignH = div->styleLine->alignH;
		div->alignV = div->styleLine->alignV;
		
#undef send
#define send()  [[[NSString alloc] initWithCharactersNoCopy:(unichar*)outputbegin length:p-outputbegin freeWhenDone:NO] autorelease]
#define psend() [[[NSString alloc] initWithCharactersNoCopy:(unichar*)parambegin length:p-parambegin freeWhenDone:NO] autorelease]
#define tag(tagt, p) [delegate spanChangedTag:tag_##tagt span:current_span div:div param:&(p)]
				
		{
			size_t linelen = [inputText length];
			NSData *linebufData;
			const unichar *linebuf = SubUnicodeForString(inputText, &linebufData);
			const unichar *p = linebuf, *pe = linebuf + linelen, *outputbegin = p, *parambegin=p, *last_tag_start=p;
			const unichar *pb = p;
			int cs = 0;
			SubRenderSpan *current_span = [[SubRenderSpan new] autorelease];
			int chars_deleted = 0; float floatnum = 0;
			NSString *strval=NULL;
			float curX, curY;
			int intnum = 0;
			BOOL reachedEnd = NO, setWrapStyle = NO, setPosition = NO, setAlignForDiv = NO, dropThisSpan = NO;
			
			[delegate didCreateStartingSpan:current_span forDiv:div];
			
			%%{
				action bold {tag(b, intnum);}
				action italic {tag(i, intnum);}
				action underline {tag(u, intnum);}
				action strikeout {tag(s, intnum);}
				action outlinesize {tag(bord, floatnum);}
				action shadowdist {tag(shad, floatnum);}
				action bluredge {tag(be, intnum);}
				action fontname {tag(fn, strval);}
				action fontsize {tag(fs, floatnum);}
				action scalex {tag(fscx, floatnum);}
				action scaley {tag(fscy, floatnum);}
				action tracking {tag(fsp, floatnum);}
				action frz {tag(frz, floatnum);}
				action frx {tag(frx, floatnum);}
				action fry {tag(fry, floatnum);}
				action primaryc {tag(1c, intnum);}
				action secondaryc {tag(2c, intnum);}
				action outlinec {tag(3c, intnum);}
				action shadowc {tag(4c, intnum);}
				action alpha {tag(alpha, intnum);}
				action primarya {tag(1a, intnum);}
				action secondarya {tag(2a, intnum);}
				action outlinea {tag(3a, intnum);}
				action shadowa {tag(4a, intnum);}
				action stylerevert {tag(r, strval);}
				action drawingmode {tag(p, floatnum); dropThisSpan = floatnum > 0;}

				action paramset {parambegin=p;}
				action setintnum {intnum = [psend() intValue];}
				action sethexnum {intnum = strtoul([psend() UTF8String], NULL, 16);}
				action setfloatnum {floatnum = [psend() floatValue];}
				action setstringval {strval = psend();}
				action nullstring {strval = @"";}
				action setpos {curX=curY=0; sscanf([psend() UTF8String], "(%f,%f", &curX, &curY);}

				action ssaalign {
					if (!setAlignForDiv) {
						setAlignForDiv = YES;
						
						SubParseASSAlignment(SubASSFromSSAAlignment(intnum), &div->alignH, &div->alignV);
					}
				}
				
				action align {
					if (!setAlignForDiv) {
						setAlignForDiv = YES;
						
						SubParseASSAlignment(intnum, &div->alignH, &div->alignV);
					}
				}
				
				action wrapstyle {
					if (!setWrapStyle) {
						setWrapStyle = YES;
						
						div->wrapStyle = intnum;

					}
				}
				
				action position {
					if (!setPosition) {
						setPosition = YES;
						
						div->posX = curX;
						div->posY = curY;
						div->positioned = YES;
					}
				}
				
				action origin {
					div->shouldResetPens = YES;
				}

				intnum = ("-"? [0-9]+) >paramset %setintnum;
				flag = [01] >paramset %setintnum;
				floatn = ("-"? ([0-9]+ ("." [0-9]*)?) | ([0-9]* "." [0-9]+));
				floatnum = floatn >paramset %setfloatnum;
				string = (([^\\}]+) >paramset %setstringval | "" %nullstring );
				color = ("H"|"&"){,2} (xdigit+) >paramset %sethexnum "&"?;
				parens = "(" [^)]* ")";
				pos = ("(" floatn "," floatn ")") >paramset %setpos;
				move = ("(" (floatn ","){3,5} floatn ")") >paramset %setpos;
				
				cmd = "\\" (
							"b" intnum %bold
							|"i" flag %italic
							|"u" flag %underline
							|"s" flag %strikeout
							|"bord" floatnum %outlinesize
							|"shad" floatnum %shadowdist
							|"be" floatnum
							|"blur" floatnum
							|"fax" floatnum
							|"fay" floatnum
							|"fn" string %fontname
							|"fs" floatnum %fontsize
							|"fscx" floatnum %scalex
							|"fscy" floatnum %scaley
							|"fsp" floatnum %tracking
							|"fr" "z"? floatnum %frz
							|"frx" floatnum
							|"fry" floatnum
							|"fe" intnum
							|"1"? "c" color %primaryc
							|"2c" color %secondaryc
							|"3c" color %outlinec
							|"4c" color %shadowc
							|"alpha" color %alpha
							|"1a" color %primarya
							|"2a" color %secondarya
							|"3a" color %outlinea
							|"4a" color %shadowa
							|"a" intnum %ssaalign
							|"an" intnum %align
							|[kK] [fo]? intnum
							|"q" intnum %wrapstyle
							|"r" string %stylerevert
							|"pos" pos %position
							|"move" move %position
							|"t" parens
							|"org" parens %origin
							|"fad" "e"? parens
							|"i"? "clip" parens
							|"p" floatnum %drawingmode
							|"pbo" floatnum
							|"xbord" floatnum
							|"ybord" floatnum
							|"xshad" floatnum
							|"yshad" floatnum
					   );
				
				tag = "{" (cmd* | any*) :> "}";

				action backslash_handler {
					p--;
					[text appendString:send()];
					unichar c = *(p+1), o=c;
					
					if (c) {
						switch (c) {
							case 'N': case 'n':
								o = '\n';
								break;
							case 'h':
								o = 0xA0; //non-breaking space
								break;
						}
						
						[text appendFormat:@"%C",o];
					}
					
					chars_deleted++;
					
					p++;
					outputbegin = p+1;
				}
				
				action enter_tag {
					if (dropThisSpan) chars_deleted += p - outputbegin;
					else if (p > outputbegin) [text appendString:send()];
					if (p == pe) reachedEnd = YES;
					
					if (p != pb) {
						[spans addObject:current_span];
						
						if (!reachedEnd) current_span = [[current_span copy] autorelease];
					}
					
					last_tag_start = p;
				}
				
				action exit_tag {			
					p++;
					chars_deleted += (p - last_tag_start);
					
					current_span->offset = (p - pb) - chars_deleted;
					outputbegin = p;
					
					p--;
				}
								
				special = ("\\" :> any) @backslash_handler | tag >enter_tag @exit_tag;
				sub_text_char = [^\\{];
				sub_text = sub_text_char+;
				
				main := ((sub_text | special)* "\\"?) %/enter_tag;
			}%%
				
			%%write init;
			%%write exec;
			%%write eof;

			if (!reachedEnd) Codecprintf(NULL, "parse error: %s\n", [inputText UTF8String]);
			[linebufData release];
			[divs addObject:div];
		}
		
	}

	[divs sortWithOptions:NSSortStable|NSSortConcurrent usingComparator:^NSComparisonResult(id a, id b){
		SubRenderDiv *divA = a, *divB = b;

		if (divA->layer < divB->layer) return NSOrderedAscending;
		else if (divA->layer > divB->layer) return NSOrderedDescending;
		return NSOrderedSame;
	}];
	return divs;
}
