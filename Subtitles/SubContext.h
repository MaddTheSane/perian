//
//  SubContext.h
//  SSARender2
//
//  Created by Alexander Strange on 7/28/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SubRenderer.h"

// approximation, real value must be determined from truetype fields
#define kVSFilterFontScale .9

enum {kSubTypeSSA, kSubTypeASS, kSubTypeSRT};
enum {kSubCollisionsNormal, kSubCollisionsReverse};
enum {kSubLineWrapTopWider = 0, kSubLineWrapSimple, kSubLineWrapNone, kSubLineWrapBottomWider};
enum {kSubAlignmentLeft, kSubAlignmentCenter, kSubAlignmentRight};
enum {kSubAlignmentBottom, kSubAlignmentMiddle, kSubAlignmentTop};

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
	BOOL bold, italic, underline, strikeout;
	UInt8 alignH, alignV, borderStyle;
	void* ex;
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
void ParseASSAlignment(UInt8 a, UInt8 *alignH, UInt8 *alignV);
extern SubRGBAColor ParseSSAColor(unsigned rgb);
