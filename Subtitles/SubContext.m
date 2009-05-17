/*
 * SubContext.m
 * Created by Alexander Strange on 7/28/07.
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

#import "SubContext.h"
#import "SubParsing.h"

@implementation SubStyle

SubRGBAColor ParseSSAColor(unsigned rgb)
{
	unsigned char r, g, b, a;
	
	a = (rgb >> 24) & 0xff;
	b = (rgb >> 16) & 0xff;
	g = (rgb >> 8) & 0xff;
	r = rgb & 0xff;
	
	a = 255-a;
	
	return (SubRGBAColor){r/255.,g/255.,b/255.,a/255.};
}

static SubRGBAColor ParseSSAColorString(NSString *c)
{
	const char *c_ = [c UTF8String];
	unsigned int rgb;
	
	if (c_[0] == '&') {
		rgb = strtoul(&c_[2],NULL,16);
	} else {
		rgb = strtol(c_,NULL,0);
	}
	
	return ParseSSAColor(rgb);
}

UInt8 SSA2ASSAlignment(UInt8 a)
{
    int h = 1, v = 0;
	if (a >= 9 && a <= 11) {v = kSubAlignmentMiddle; h = a-8;}
	if (a >= 5 && a <= 7)  {v = kSubAlignmentTop;    h = a-4;}
	if (a >= 1 && a <= 3)  {v = kSubAlignmentBottom; h = a;}
	return v * 3 + h;
}

void ParseASSAlignment(UInt8 a, UInt8 *alignH, UInt8 *alignV)
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

BOOL ParseFontVerticality(NSString **fontname)
{
	if ([*fontname characterAtIndex:0] == '@') {
		*fontname = [*fontname substringFromIndex:1];
		//return YES; // XXX vertical
	}
	return NO;
}

+(SubStyle*)defaultStyleWithDelegate:(SubRenderer*)delegate
{
	SubStyle *sty = [[[SubStyle alloc] init] autorelease];
	
	sty->name = @"*Default";
	sty->fontname = @"Helvetica";
	sty->platformSizeScale = 1;
	sty->size = 32 * sqrt([delegate aspectRatio] / (4./3.));
	sty->primaryColor = (SubRGBAColor){1,1,1,1};
	sty->outlineColor = (SubRGBAColor){0,0,0,1};
	sty->shadowColor = (SubRGBAColor){0,0,0,1};
	sty->scaleX = sty->scaleY = 100;
	sty->tracking = sty->angle = 0;
	sty->outlineRadius = 1.5;
	sty->shadowDist = 2;
	sty->marginL = sty->marginR = sty->marginV = 20;
	sty->weight = 1;
	sty->italic = sty->underline = sty->strikeout = NO;
	sty->alignH = kSubAlignmentCenter;
	sty->alignV = kSubAlignmentBottom;
	sty->borderStyle = kSubBorderStyleNormal;
	sty->ex = [delegate completedStyleParsing:sty];
	sty->delegate = delegate;
	return sty;
}

-(SubStyle*)initWithDictionary:(NSDictionary *)s scriptVersion:(UInt8)version delegate:(SubRenderer*)delegate_
{
	if (self = [super init]) {
		NSString *tmp;
		delegate = delegate_;
		NSString *sv;
				
#define sv(fn, n) fn = [[s objectForKey: @""#n] retain]
#define fv(fn, n) sv = [s objectForKey:@""#n]; fn = sv ? [sv floatValue] : 0.;
#define iv(fn, n) fn = [[s objectForKey:@""#n] intValue]
#define bv(fn, n) fn = [[s objectForKey:@""#n] intValue] != 0
#define cv(fn, n) tmp = [s objectForKey:@""#n]; if (tmp) fn = ParseSSAColorString(tmp);
		
		sv(name, Name);
		sv(fontname, Fontname);
		fv(size, Fontsize);
		cv(primaryColor, PrimaryColour);
		cv(secondaryColor, SecondaryColour);
		tmp = [s objectForKey:@"BackColour"];
		if (tmp) outlineColor = shadowColor = ParseSSAColorString(tmp);
		cv(outlineColor, OutlineColour);
		cv(shadowColor, ShadowColour);
		fv(weight, Bold);
		bv(italic, Italic);
		bv(underline, Underline);
		bv(strikeout, Strikeout);
		fv(scaleX, ScaleX);
		fv(scaleY, ScaleY);
		fv(tracking, Spacing);
		fv(angle, Angle);
		fv(outlineRadius, Outline);
		fv(shadowDist, Shadow);
		iv(marginL, MarginL);
		iv(marginR, MarginR);
		iv(marginV, MarginV);
		iv(borderStyle, BorderStyle);
		
		if (!scaleX) scaleX = 100;
		if (!scaleY) scaleY = 100;
		if (weight == -1) weight = 1;

		UInt8 align = [[s objectForKey:@"Alignment"] intValue];
		if (version == kSubTypeSSA) align = SSA2ASSAlignment(align);
		
		ParseASSAlignment(align, &alignH, &alignV);
		
		platformSizeScale = 0;
		ex = [delegate completedStyleParsing:self];
	}
	
	return self;
}

static NSString *yes(int i) {return i ? @"yes" : @"no";}

static NSString *ColorString(SubRGBAColor *c)
{
	return [NSString stringWithFormat:@"{R: %d G: %d B: %d A: %d}", (int)(c->red * 255),
					 (int)(c->green * 255), (int)(c->blue * 255), (int)(c->alpha * 255)];
}

-(NSString *)description
{
	static const NSString *alignHStr[] = {@"left", @"center", @"right"};
	static const NSString *alignVStr[] = {@"bottom", @"middle", @"top"};
	static const NSString *bstyle[] = {@"normal", @"box"};
	
	return [NSString stringWithFormat:@""
					 "SubStyle named \"%@\"\n"
					 "-Font: %f pt %@\n"
					 "-Primary color: %@\n"
					 "-Secondary color: %@\n"
					 "-Outline: %f px %@\n"
					 "-Shadow: %f px %@\n"
					 "-Margin: %d px left, %d px right, %d px vertical\n"
					 "-Bold: %@ Italic: %@ Underline: %@ Strikeout: %@\n"
					 "-Alignment: %@ %@\n"
					 "-Border style: %@\n"
					 "-Per-font scale: %f\n",
					 name, size, fontname, ColorString(&primaryColor),
					 ColorString(&secondaryColor), outlineRadius, ColorString(&outlineColor),
					 shadowDist, ColorString(&shadowColor), marginL, marginR, marginV,
					 yes(bold), yes(italic), yes(underline), yes(strikeout), alignHStr[alignH],
					 alignVStr[alignV], bstyle[borderStyle], platformSizeScale];
}

-(void)dealloc
{
	[name release];
	[fontname release];
	[delegate releaseStyleExtra:ex];
	[super dealloc];
}

-(void)finalize
{
	[delegate releaseStyleExtra:ex];
	[super finalize];
}
@end

@implementation SubContext

BOOL IsScriptASS(NSDictionary *headers)
{	
	return [[[headers objectForKey:@"ScriptType"] lowercaseString] isEqualToString:@"v4.00+"];
}

-(void)readHeaders
{
	scriptType = IsScriptASS(headers) ? kSubTypeASS : kSubTypeSSA;
	collisions = [[headers objectForKey:@"Collisions"] isEqualToString:@"Reverse"] ? kSubCollisionsReverse : kSubCollisionsNormal;
	wrapStyle = [[headers objectForKey:@"WrapStyle"] intValue];
	
	NSString *resXS = [headers objectForKey:@"PlayResX"], *resYS = [headers objectForKey:@"PlayResY"];
	
	if (resXS) resX = [resXS floatValue];
	if (resYS) resY = [resYS floatValue];
	
	// obscure res rules copied from VSFilter
	if ((!resXS && !resYS) || (!resX && !resY)) {
		resX = 384; resY = 288;
	} else if (!resYS) {
		resY = (resX == 1280) ? 1024 : (resX / (4./3.));
	} else if (!resXS) {
		resX = (resY == 1024) ? 1280 : (resY * (4./3.));
	}
}

-(SubContext*)initWithHeaders:(NSDictionary *)headers_ styles:(NSArray *)styles_ delegate:(SubRenderer*)delegate
{
	if (self = [super init]) {
		headers = [headers_ retain];
		[self readHeaders];
		
		[delegate completedHeaderParsing:self];
		
		styles = nil;
		if (styles_) {
			int i, nstyles = [styles_ count];
			NSMutableDictionary *sdict = [[NSMutableDictionary alloc] initWithCapacity:nstyles];
			
			for (i=0; i < nstyles; i++) {
				NSDictionary *style = [styles_ objectAtIndex:i];
				[sdict setObject:[[[SubStyle alloc] initWithDictionary:style scriptVersion:scriptType delegate:delegate] autorelease]
												forKey:[style objectForKey:@"Name"]];
			}
			
			styles = sdict;
		}
		
		defaultStyle = [styles objectForKey:@"Default"];
		if (!defaultStyle) defaultStyle = [SubStyle defaultStyleWithDelegate:delegate];
		[defaultStyle retain];
		
	}
	
	return self;
}

-(SubContext*)initWithNonSSAType:(UInt8)type delegate:(SubRenderer*)delegate
{
	if (self = [super init]) {
		resX = 640;
		resY = 480;
		scriptType = type;
		collisions = kSubCollisionsNormal;
		wrapStyle = kSubLineWrapBottomWider;
		styles = headers = nil;
		[delegate completedHeaderParsing:self];

		defaultStyle = [[SubStyle defaultStyleWithDelegate:delegate] retain];		
	}
	
	return self;
}

-(void)dealloc
{
	[styles release];
	[defaultStyle release];
	[headers release];
	[super dealloc];
}

-(SubStyle*)styleForName:(NSString *)name
{
	SubStyle *sty = [styles objectForKey:name];
	return sty ? sty : defaultStyle;
}
@end

@implementation SubRenderer
-(void*)completedStyleParsing:(SubStyle*)s {return nil;}
-(void)completedHeaderParsing:(SubContext*)sc {}
-(void*)spanExtraFromRenderDiv:(SubRenderDiv*)div {return nil;}
-(void*)cloneSpanExtra:(SubRenderSpan*)span {return span->ex;}
-(void)spanChangedTag:(SSATagType)tag span:(SubRenderSpan*)span div:(SubRenderDiv*)div param:(void*)p {}
-(void)releaseStyleExtra:(void*)ex {}
-(void)releaseSpanExtra:(void*)ex {}
-(float)aspectRatio {return 4./3.;}
-(NSString*)describeSpanEx:(void*)ex {return @"";}
@end