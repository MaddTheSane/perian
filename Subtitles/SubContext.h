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

__BEGIN_DECLS
NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(int, SubType) {
	kSubTypeSSA,
	kSubTypeASS,
	kSubTypeSRT,
	kSubTypeSMI
};
typedef NS_ENUM(UInt8, SubCollisions) {
	kSubCollisionsNormal,
	kSubCollisionsReverse
};
typedef NS_ENUM(UInt8, SubLineWrap) {
	kSubLineWrapTopWider = 0,
	kSubLineWrapSimple,
	kSubLineWrapNone,
	kSubLineWrapBottomWider
};
typedef NS_ENUM(UInt8, SubAlignmentH)  {
	kSubAlignmentLeft,
	kSubAlignmentCenter,
	kSubAlignmentRight
};
typedef NS_ENUM(UInt8, SubAlignmentV)  {
	kSubAlignmentBottom,
	kSubAlignmentMiddle,
	kSubAlignmentTop
};
typedef NS_ENUM(UInt8, SubBorderStyle) {
	kSubBorderStyleNormal = 1,
	kSubBorderStyleBox = 3
};
enum {kSubPositionNone = INT_MAX};

typedef ATSURGBAlphaColor SubRGBAColor;

extern NSString * const kSubDefaultFontName;

@class SubRenderer;

@interface SubStyle : NSObject {
	id extra;

	@public;
	NSString *name;
	NSString *fontname;
	__unsafe_unretained SubRenderer *delegate;
	
	Float32 size;
	SubRGBAColor primaryColor, secondaryColor, outlineColor, shadowColor;
	Float32 scaleX, scaleY, tracking, angle;
	Float32 outlineRadius, shadowDist;
	Float32 weight; // 0/1 = not bold/bold, > 1 is a font weight
	BOOL italic, underline, strikeout, vertical;
	int marginL, marginR, marginV;
	SubAlignmentH alignH;
	SubAlignmentV alignV;
	SubBorderStyle borderStyle;
	Float32 platformSizeScale;
}

@property (retain) id extra;
@property (copy) NSString *name;
@property (copy) NSString *fontname;
@property (assign) SubRenderer *delegate;
	
@property Float32 size;
@property SubRGBAColor primaryColor, secondaryColor, outlineColor, shadowColor;
@property Float32 scaleX, scaleY, tracking, angle;
@property Float32 outlineRadius, shadowDist;
@property Float32 weight;
@property BOOL italic, underline, strikeout, vertical;
@property int marginL, marginR, marginV;
@property SubAlignmentH alignH;
@property SubAlignmentV alignV;
@property SubBorderStyle borderStyle;
@property Float32 platformSizeScale;

+ (instancetype)defaultStyleWithDelegate:(SubRenderer*)delegate;
- (instancetype)initWithDictionary:(NSDictionary<NSString*,id> *)ssaDict scriptVersion:(UInt8)version delegate:(SubRenderer *)renderer;
@end

@interface SubContext : NSObject {
	@public;
	NSDictionary<NSString*,id> *headers;
	NSDictionary<NSString*,SubStyle*> *styles;
	SubStyle *defaultStyle;

	UInt8 scriptType, collisions, wrapStyle;
	
	CGFloat resX, resY;
}

@property CGFloat resX;
@property CGFloat resY;

- (instancetype)initWithScriptType:(SubType)type headers:(nullable NSDictionary<NSString*,NSString*> *)headers styles:(nullable NSArray<NSDictionary<NSString*,NSString*>*> *)styles delegate:(nullable SubRenderer*)delegate;
-(SubStyle*)styleForName:(NSString *)name;
@property (readonly, copy) NSDictionary<NSString*,SubStyle*> *styles;
@end

NS_ASSUME_NONNULL_END
__END_DECLS
