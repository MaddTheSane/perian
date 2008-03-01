/*
 *  ssa2html.m
 *  SSARender2
 *
 *  Created by Alexander Strange on 7/28/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#import "SubImport.h"
#import "SubParsing.h"

@interface SubHTMLExporter : SubRenderer
{
	SubContext *sc;
	@public;
	NSMutableString *html;
}

@end

@implementation SubHTMLExporter
-(SubHTMLExporter*)init
{
	if (self = [super init])
	{
		html = [[NSMutableString alloc] init];
		[html appendString:@"<html>\n"];
		[html appendString:@"<head>\n"];
		[html appendString:@"<meta http-equiv=\"Content-type\" content=\"text/html; charset=UTF-8\" />\n"];
	}
	
	return self;
}

-(void)completedHeaderParsing:(SubContext*)sc_
{
	sc = sc_;
	[html appendFormat:@"<title>%@</title>\n",[sc->headers objectForKey:@"Title"]];
	[html appendString:@"<style type=\"text/css\">\n"];
	[html appendFormat:@".screen {width: %dpx; height: %dpx; background-color: gray; position: relative}\n.bottom {bottom: 20px; position: absolute} .top {top: 20px; position: absolute}\n",sc->resX,sc->resY];
}

static const NSString *haligns[] = {@"left", @"center", @"right"};

-(void*)completedStyleParsing:(SubStyle*)s
{
	[html appendFormat:@".%@ {display: inline-block; clear: none;\n",s->name];
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
	[html appendFormat:@"width: %dpx;\n",sc->resX - s->marginL - s->marginR];
	[html appendFormat:@"font-weight: %@; font-style: %@; text-decoration: %@;\n",s->bold ? @"bold" : @"normal", s->italic ? @"italic" : @"normal", s->underline ? @"underline" : (s->strikeout ? @"line-through" : @"none")];
	[html appendFormat:@"text-align: %@;\n", haligns[s->alignH]];
	[html appendString:@"}\n"];
	
	return nil;
}

-(void)endOfHead
{
	[html appendString:@"</style>\n"];
	[html appendString:@"</head>\n"];
	[html appendString:@"<body>\n"];
}

NSString *htmlfilter(NSString *s)
{
	NSMutableString *ms = [s mutableCopy];
	
	[ms replaceOccurrencesOfString:@"\n" withString:@"<br>" options:0 range:NSMakeRange(0, [ms length])];
//	[ms replaceOccurrencesOfString:[NSString stringWithFormat:@"%C",0x00A0] withString:@"&nbsp;" options:0 range:NSMakeRange(0, [ms length])];
	return ms;
}

-(void*)spanExtraFromRenderDiv:(SubRenderDiv*)div
{
	return [NSMutableString string];
}

-(void*)cloneSpanExtra:(SubRenderSpan*)span
{
	return [NSMutableString string];
}

-(void)spanChangedTag:(SSATagType)tag span:(SubRenderSpan*)span div:(SubRenderDiv*)div param:(void*)p
{
	NSMutableString *sty = span->ex;
	int ip;
	NSString *sp;
	float fp;
	
		
#define iv() ip = *(int*)p; 
#define sv() sp = *(NSString**)p;
#define fv() fp = *(float*)p;
#define cv() ip = *(int*)p; ip = EndianU32_BtoL(ip); ip = ip & 0xFFFFFF;
	
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
	int div_count = [divs count], i;
	for (i = 0; i < div_count; i++) {
		SubRenderDiv *div = [divs objectAtIndex:i];
		int j, spancount = [div->spans count], spans = 1, close_div = 0;
		
		if (div->posX > -1) {
			[html appendFormat:@"<div style=\"top: %dpx; left: %dpx; position: absolute\">", div->posY, div->posX];
			close_div = 1;
		}
				
		[html appendFormat:@"<span class=\"%@\">", div->styleLine->name];
		
		for (j = 0; j < spancount; j++) {
			SubRenderSpan *span = [div->spans objectAtIndex:j];
			NSMutableString *ex = span->ex;
			int exl = [ex length];
			
			if (exl) {[html appendFormat:@"<span style=\"%@\">",ex]; spans++;}
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
	NSArray *divs = SubParsePacket(sl->line, sc, self, ubuf);
	free(ubuf);
	NSMutableArray *top = [NSMutableArray array], *bot = [NSMutableArray array], *abs = [NSMutableArray array];
	int div_count = [divs count], i;
	
	[html appendString:@"<div class=\"screen\">\n"];

	for (i = 0; i < div_count; i++) {
		SubRenderDiv *div = [divs objectAtIndex:i];
		
		if (div->posX > -1) [abs addObject:div]; else if (div->alignV == kSubAlignmentTop) [top addObject:div]; else [bot insertObject:div atIndex:0];
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
	[html appendString:@"</body></html>"];
}
@end

int main(int argc, char *argv[])
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	SubContext *sc; SubSerializer *ss;
	SubHTMLExporter *htm = [[SubHTMLExporter alloc] init];
	
	SubLoadSSAFromPath([NSString stringWithUTF8String:argv[1]],&sc,&ss,htm);
	
	[htm endOfHead];
	
	while (![ss isEmpty]) {
		NSAutoreleasePool *pool2 = [[NSAutoreleasePool alloc] init];

		SubLine *sl = [ss getSerializedPacket];
		if ([sl->line length] == 1) continue;
		
		[htm addSub:sl];
		[pool2 release];
	}
	
	[htm endOfFile];
	
	printf([htm->html UTF8String]);
	[pool release];
	return 0;
}
