/*
 * SubContext.h
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

#import <Cocoa/Cocoa.h>
#import "SubRenderer.h"

enum {kSubTypeSSA, kSubTypeASS, kSubTypeSRT, kSubTypeSMI};
enum {kSubCollisionsNormal, kSubCollisionsReverse};
enum {kSubLineWrapTopWider = 0, kSubLineWrapSimple, kSubLineWrapNone, kSubLineWrapBottomWider};
enum {kSubAlignmentLeft, kSubAlignmentCenter, kSubAlignmentRight};
enum {kSubAlignmentBottom, kSubAlignmentMiddle, kSubAlignmentTop};
enum {kSubBorderStyleNormal = 1, kSubBorderStyleBox = 3};
enum {kSubPositionNone = INT_MAX};

typedef ATSURGBAlphaColor SubRGBAColor;

@interface SubStyle : NSObject {	
	@public;
	NSString *name;
	NSString *fontname;
	Float32 size;
	SubRGBAColor primaryColor, secondaryColor, outlineColor, shadowColor;
	Float32 scaleX, scaleY, tracking, angle;
	Float32 outlineRadius, shadowDist;
	unsigned marginL, marginR, marginV;
	Float32 weight; // 0/1 = not bold/bold, > 1 is a font weight
	BOOL italic, underline, strikeout;
	UInt8 alignH, alignV, borderStyle;
	__strong void* ex;
	Float32 platformSizeScale;
	SubRenderer *delegate;
}
+(SubStyle*)defaultStyleWithDelegate:(SubRenderer*)delegate;
-(SubStyle*)initWithDictionary:(NSDictionary *)s scriptVersion:(UInt8)version delegate:(SubRenderer *)renderer;
@end

@interface SubContext : NSObject {
	@public;
	
	float resX, resY;
	UInt8 scriptType, collisions, wrapStyle;
	NSDictionary *styles; SubStyle *defaultStyle;
	NSDictionary *headers;
	NSString *headertext;
}

-(SubContext*)initWithHeaders:(NSDictionary *)headers styles:(NSArray *)styles extraData:(NSString *)ed delegate:(SubRenderer*)delegate;
-(SubContext*)initWithNonSSAType:(UInt8)type delegate:(SubRenderer*)delegate;
-(SubStyle*)styleForName:(NSString *)name;
@end

extern UInt8 SSA2ASSAlignment(UInt8 a);
extern void ParseASSAlignment(UInt8 a, UInt8 *alignH, UInt8 *alignV);
extern SubRGBAColor ParseSSAColor(unsigned rgb);
extern BOOL ParseFontVerticality(NSString **fontname);