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
#import "SubRenderer.h"

NSString * const kSubDefaultFontName = @"Helvetica";

@implementation SubStyle

+(SubStyle*)defaultStyleWithDelegate:(SubRenderer*)delegate
{
	SubStyle *sty = [[[SubStyle alloc] init] autorelease];
	
	sty->name = @"Default";
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
	sty->delegate = delegate;
	
	[delegate didCompleteStyleParsing:sty];
	return sty;
}

@synthesize extra;

-(SubStyle*)initWithDictionary:(NSDictionary *)s scriptVersion:(UInt8)version delegate:(SubRenderer*)delegate_
{
	if (self = [super init]) {
		NSString *tmp;
		NSString *sv;
		
		delegate = delegate_;
		
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
		[delegate didCompleteStyleParsing:self];
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
	[extra release];
	[name release];
	[fontname release];
	[super dealloc];
}
@end

@implementation SubContext

BOOL IsScriptASS(NSDictionary *headers)
{	
	return [[headers objectForKey:@"ScriptType"] caseInsensitiveCompare:@"v4.00+"] == NSOrderedSame;
}

-(void)readSSAHeaders
{
	NSString *sv;
	scriptType = IsScriptASS(headers) ? kSubTypeASS : kSubTypeSSA;
	collisions = [[headers objectForKey:@"Collisions"] isEqualToString:@"Reverse"] ? kSubCollisionsReverse : kSubCollisionsNormal;
	sv = [headers objectForKey:@"WrapStyle"];
	if (sv)
		wrapStyle = [sv intValue];
	
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

-(SubContext*)initWithScriptType:(int)type headers:(NSDictionary *)headers_ styles:(NSArray *)stylesArray delegate:(SubRenderer*)delegate
{
	if (self = [super init]) {
		resX = 640;
		resY = 480;
		scriptType = type;
		wrapStyle = kSubLineWrapBottomWider;
		
		headers = headers_;
		if (headers) {
			[headers retain];
			[self readSSAHeaders];
		}
		
		[delegate didCompleteHeaderParsing:self];
		
		styles = nil;
		if (stylesArray) {
			int i, nstyles = [stylesArray count];
			styles = [[NSMutableDictionary alloc] initWithCapacity:nstyles];
			
			for (i=0; i < nstyles; i++) {
				NSDictionary *style = [stylesArray objectAtIndex:i];
				NSString *name = [style objectForKey:@"Name"];
				SubStyle *sstyle = [[[SubStyle alloc] initWithDictionary:style scriptVersion:scriptType delegate:delegate] autorelease];
				
				// VSFilter bug: styles with * in the name have the first character dropped
				if ([name length] && [name characterAtIndex:0]=='*')
					name = [name substringFromIndex:1];
				
				[(NSMutableDictionary*)styles setObject:sstyle forKey:name];
			}
			
			defaultStyle = [styles objectForKey:@"Default"];
		}
		
		if (!defaultStyle)
			defaultStyle = [SubStyle defaultStyleWithDelegate:delegate];
		
		[defaultStyle retain];
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
-(void)didCompleteStyleParsing:(SubStyle*)s {assert(0);}
-(void)didCompleteHeaderParsing:(SubContext*)sc {assert(0);}
-(void)didCreateStartingSpan:(SubRenderSpan *)span forDiv:(SubRenderDiv *)div {assert(0);}
-(void)spanChangedTag:(SubSSATagName)tag span:(SubRenderSpan*)span div:(SubRenderDiv*)div param:(void*)p {assert(0);}
-(float)aspectRatio {return 4./3.;}
@end