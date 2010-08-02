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

NSString * const kSubDefaultFontName = @"Helvetica";

@implementation SubStyle


+(SubStyle*)defaultStyleWithDelegate:(SubRenderer*)delegate
{
	SubStyle *sty = [[[SubStyle alloc] init] autorelease];
	
	sty->name = @"*Default";
	sty->fontname = kSubDefaultFontName;
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
#define cv(fn, n) tmp = [s objectForKey:@""#n]; if (tmp) fn = SubParseSSAColorString(tmp);
		
		sv(name, Name);
		sv(fontname, Fontname);
		fv(size, Fontsize);
		cv(primaryColor, PrimaryColour);
		cv(secondaryColor, SecondaryColour);
		tmp = [s objectForKey:@"BackColour"];
		if (tmp) outlineColor = shadowColor = SubParseSSAColorString(tmp);
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
		if (version == kSubTypeSSA) align = SubASSFromSSAAlignment(align);
		
		SubParseASSAlignment(align, &alignH, &alignV);
		
		tmp = fontname;
		vertical = SubParseFontVerticality(&tmp);
		if (vertical)
			fontname = [tmp retain];
		
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
					 alignVStr[alignV], (borderStyle == 3) ? @"box" : @"normal", platformSizeScale];
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
	return [[headers objectForKey:@"ScriptType"] caseInsensitiveCompare:@"v4.00+"] == NSOrderedSame;
}

-(void)readHeaders
{
	scriptType = IsScriptASS(headers) ? kSubTypeASS : kSubTypeSSA;
	collisions = [[headers objectForKey:@"Collisions"] isEqualToString:@"Reverse"] ? kSubCollisionsReverse : kSubCollisionsNormal;
	wrapStyle = [[headers objectForKey:@"WrapStyle"] intValue];
	
	NSString *resXS = [headers objectForKey:@"PlayResX"], *resYS = [headers objectForKey:@"PlayResY"];
	
	if (resXS) resX = [resXS floatValue];
	if (resYS) resY = [resYS floatValue];
	
	// XXX: obscure res rules copied from VSFilter
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
		SubStyle *firstStyle = nil;
		headers = [headers_ retain];
		[self readHeaders];
		
		[delegate completedHeaderParsing:self];
		
		styles = nil;
		if (styles_) {
			int i, nstyles = [styles_ count];
			NSMutableDictionary *sdict = [[NSMutableDictionary alloc] initWithCapacity:nstyles];
			
			for (i=0; i < nstyles; i++) {
				NSDictionary *style = [styles_ objectAtIndex:i];
				SubStyle *sstyle = [[[SubStyle alloc] initWithDictionary:style scriptVersion:scriptType delegate:delegate] autorelease];
				
				if (!i) firstStyle = sstyle;
				
				[sdict setObject:sstyle forKey:[style objectForKey:@"Name"]];
			}
			
			styles = sdict;
		}
		
		defaultStyle = [styles objectForKey:@"Default"];
		if (!defaultStyle) defaultStyle = firstStyle;
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
-(void)spanChangedTag:(SubSSATagName)tag span:(SubRenderSpan*)span div:(SubRenderDiv*)div param:(void*)p {}
-(void)releaseStyleExtra:(void*)ex {}
-(void)releaseSpanExtra:(void*)ex {}
-(float)aspectRatio {return 4./3.;}
-(NSString*)describeSpanEx:(void*)ex {return @"";}
@end