/*
 * ssa2html
 * Created by Alexander Strange on 7/28/07.
 *
 * A primitive .ssa/.ass to HTML converter.
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

#import "SubImport.h"
#import "SubParsing.h"
#import "SubRenderer.h"

@interface SubHTMLExporter : SubRenderer
{
	SubContext *sc;
	NSMutableString *html;
}
@property (readonly) NSMutableString *html;
@end

@interface SubHTMLSpanExtra : NSObject
{
	@public;
	NSMutableString *str;
}
@end

@implementation SubHTMLSpanExtra
-(SubHTMLSpanExtra*)init
{
	self = [super init];
	str = [[NSMutableString alloc] init];
	return self;
}

-(SubHTMLSpanExtra*)copyWithZone:(NSZone*)zone
{
	return [SubHTMLSpanExtra new];
}

-(void)dealloc
{
	[str release];
	[super dealloc];
}
@end

@implementation SubHTMLExporter
@synthesize html;

-(SubHTMLExporter*)init
{
	if (self = [super init])
	{
		html = [[NSMutableString alloc] init];
		[html appendString:@"<html>\n"];
		[html appendString:@"<head>\n"];
		[html appendString:@"<meta http-equiv=\"Content-type\" content=\"text/html; charset=UTF-8\" />\n"];
		[html appendString:@"<meta name=\"generator\" content=\"ssa2html\" />\n"];
	}
	
	return self;
}

-(void)dealloc
{
	[sc release];
	[html release];
	[super dealloc];
}

-(void)didCompleteHeaderParsing:(SubContext*)sc_
{
	sc = [sc_ retain];
	[html appendFormat:@"<title>%@</title>\n",[sc->headers objectForKey:@"Title"]];
	[html appendString:@"<style type=\"text/css\">\n"];
	[html appendFormat:@".screen {width: %fpx; height: %fpx; background-color: gray; position: relative; display: table}\n.bottom {bottom: 20px; position: absolute} .top {top: 20px; position: absolute}\n",sc->resX,sc->resY];
}

static const NSString *haligns[] = {@"left", @"center", @"right"};
static const NSString *valigns[] = {@"bottom", @"middle", @"top"};

// font-weight actually seems to be enumerated 100|200|...|900
// but, like, whatever
static NSString *FontWeightStringForWeight(Float32 weight)
{
	if (weight == 0)
		return @"normal";
	else if (weight == 1)
		return @"bold";
	else
		return [NSString stringWithFormat:@"%d", (int)weight];
}

static NSString *EscapeCSSIdentifier(NSString *s)
{
	if (![s rangeOfString:@" "].length) return s;
	return [s stringByReplacingOccurrencesOfString:@" " withString:@"_"];
}

-(void)didCompleteStyleParsing:(SubStyle*)s
{
	[html appendFormat:@".%@ {display: table-cell; clear: none;\n",EscapeCSSIdentifier(s->name)];
	[html appendFormat:@"font-family: \"%@\"; ",s->fontname];
	[html appendFormat:@"font-size: %fpt;\n",s->size * (72./96.)];
	[html appendFormat:@"color: #%X%X%X;\n",(int)(s->primaryColor.red*255.),(int)(s->primaryColor.green*255.),(int)(s->primaryColor.blue*255.)];
	[html appendFormat:@"-webkit-text-stroke-color: #%X%X%X;\n",(int)(s->outlineColor.red*255.),(int)(s->outlineColor.green*255.),(int)(s->outlineColor.blue*255.)];
	[html appendFormat:@"letter-spacing: %fpx;\n",s->tracking];
//	[html appendFormat:@"-webkit-text-stroke-width: %fpx;\n",s->outlineRadius];
	[html appendFormat:@"text-shadow: #%X%X%X %fpx %fpx 0;\n",(int)(s->shadowColor.red*255.),(int)(s->shadowColor.green*255.),(int)(s->shadowColor.blue*255.),
					  s->shadowDist*2., s->shadowDist*2.];
	[html appendFormat:@"text-outline: #%X%X%X %fpx 0;\n",(int)(s->shadowColor.red*255.),(int)(s->shadowColor.green*255.),(int)(s->shadowColor.blue*255.),
		s->outlineRadius];
	[html appendFormat:@"width: %fpx;\n",sc->resX - s->marginL - s->marginR];
	[html appendFormat:@"font-weight: %@; font-style: %@; text-decoration: %@;\n",FontWeightStringForWeight(s->weight), s->italic ? @"italic" : @"normal", s->underline ? @"underline" : (s->strikeout ? @"line-through" : @"none")];
	[html appendFormat:@"text-align: %@;\n", haligns[s->alignH]];
	[html appendFormat:@"vertical-align: %@;\n", valigns[s->alignV]];
	[html appendString:@"}\n"];
}

-(void)endOfHead
{
	[html appendString:@"</style>\n"];
	[html appendString:@"</head>\n"];
	[html appendString:@"<body>\n"];
}

NSString *htmlfilter(NSString *s)
{
	NSMutableString *ms = [[s mutableCopy] autorelease];
	
	[ms replaceOccurrencesOfString:@"\n" withString:@"<br>\n" options:0 range:NSMakeRange(0, [ms length])];
//	[ms replaceOccurrencesOfString:[NSString stringWithFormat:@"%C",0x00A0] withString:@"&nbsp;" options:0 range:NSMakeRange(0, [ms length])];
	return ms;
}

-(void)didCreateStartingSpan:(SubRenderSpan *)span forDiv:(SubRenderDiv *)div
{
	span.extra = [[SubHTMLSpanExtra new] autorelease];
}

-(void)spanChangedTag:(SubSSATagName)tag span:(SubRenderSpan*)span div:(SubRenderDiv*)div param:(void*)p
{
	SubHTMLSpanExtra *ex = span.extra;
	NSMutableString *sty = ex->str;
	int ip;
	NSString *sp;
	float fp;
	
#define iv() ip = *(int*)p; 
#define sv() sp = *(NSString**)p;
#define fv() fp = *(float*)p;
#define cv() ip = *(int*)p; ip = EndianU32_NtoL(ip) & 0xFFFFFF;
	
	switch (tag) {
		case tag_b:
			iv();
			[sty appendFormat:@"font-weight: %@; ", ip? @"bold" : @"normal"];
			break;
		case tag_i:
			iv();
			[sty appendFormat:@"font-style: %@; ", ip? @"italic" : @"normal"];
			break;
		case tag_u:
			iv();
			[sty appendFormat:@"text-decoration: %@; ", ip? @"underline" : @"none"];
			break;
		case tag_s:
			iv();
			[sty appendFormat:@"text-decoration: %@; ", ip? @"line-through" : @"none"];
			break;
		case tag_fn:
			sv();
			[sty appendFormat:@"font-family: %@; ", sp];
			break;
		case tag_fs:
			fv();
			//this is wrong, see GetWinFontSizeScale()
			[sty appendFormat:@"font-size: %fpt; ", fp * (72./96.)];
			break;
		case tag_1c:
			cv();
			[sty appendFormat:@"color: #%0.9X; ", ip]; 
			break;
		case tag_4c:
			cv();
			[sty appendFormat:@"text-shadow: #%0.9X %fpx %fpx 0; ", ip, div->styleLine->shadowDist*2., div->styleLine->shadowDist*2.];
			break;
		default:
			NSLog(@"unimplemented tag type %d",tag);
	}
}

-(void)htmlifyDivArray:(NSArray*)divs
{
	for (SubRenderDiv *div in divs) {
		int j, spancount = [div->spans count], spans = 1, close_div = 0;
		
		if (div->positioned) {
			[html appendFormat:@"<div style=\"top: %fpx; left: %fpx; position: absolute\">", div->posY, div->posX];
			close_div = 1;
		}
				
		[html appendFormat:@"<span class=\"%@\">", EscapeCSSIdentifier(div->styleLine->name)];
		
		for (j = 0; j < spancount; j++) {
			SubRenderSpan *span = [div->spans objectAtIndex:j];
			SubHTMLSpanExtra *ex = span.extra;
			NSMutableString *str = ex->str;
			
			if ([str length]) {[html appendFormat:@"<span style=\"%@\">",str]; spans++;}
			[html appendString:htmlfilter([div->text substringWithRange:NSMakeRange(span->offset, ((j == (spancount-1)) ? [div->text length] : ((SubRenderSpan*)[div->spans objectAtIndex:j+1])->offset) - span->offset)])];
		}
		
		while (spans--) [html appendString:@"</span>"]; 
		if (close_div) [html appendString:@"</div>"];
		[html appendString:@"\n"]; 
	}
}

-(void)addSub:(SubLine*)sl
{	
	unichar *ubuf = malloc(sizeof(unichar) * [sl->line length]);
	NSArray *divs = SubParsePacket(sl->line, sc, self);
	free(ubuf);
	NSMutableArray *top = [NSMutableArray array], *bot = [NSMutableArray array], *abs = [NSMutableArray array];
	
	[html appendString:@"<div class=\"screen\">\n"];

	for (SubRenderDiv *div in divs) {
		
		if (div->positioned)
			[abs addObject:div];
		else if (div->alignV == kSubAlignmentTop)
			[top addObject:div];
		else
			[bot insertObject:div atIndex:0];
	}
	
	if ([top count]) {
		[html appendString:@"<div class=\"top\">\n"];
		[self htmlifyDivArray:top];
		[html appendString:@"</div>\n"];
	}
	
	if ([bot count]) {
		[html appendString:@"<div class=\"bottom\">\n"];
		[self htmlifyDivArray:bot];
		[html appendString:@"</div>\n"];
	}
	
	[self htmlifyDivArray:abs];
	
	[html appendString:@"</div>\n"];

	[html appendString:@"<br>\n"];
}

-(void)endOfFile
{
	[html appendString:@"</body></html>\n"];
}
@end

int main(int argc, char *argv[])
{	
	if (argc != 2)
		return 1;

	NSAutoreleasePool *outer_pool = [[NSAutoreleasePool alloc] init];
	SubContext *sc; SubSerializer *ss = [[SubSerializer alloc] init];
	SubHTMLExporter *htm = [[SubHTMLExporter alloc] init];

	//start of lameness
	//it should only have to call subparsessafile here, or something
	NSString *header = SubLoadSSAFromPath([NSString stringWithUTF8String:argv[1]], ss);
	ss.finished = YES;

	NSDictionary *headers;
	NSArray *styles;
	SubParseSSAFile(header, &headers, &styles, NULL);
	sc = [[SubContext alloc] initWithScriptType:kSubTypeSSA headers:headers styles:styles delegate:htm];
	//end(?) of lameness
	
	[htm endOfHead];
	[sc release];
	
	while (![ss isEmpty]) {
		SubLine *sl = [ss getSerializedPacket];
		if ([sl->line length] == 1) continue;
		
		[htm addSub:sl];
	}
	
	[htm endOfFile];
	
	puts([htm.html UTF8String]);
	[ss release];
	[htm release];
	[outer_pool release];

	return 0;
}
