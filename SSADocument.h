//
//  SSADocument.h
//  SSAView
//
//  Created by Alexander Strange on 1/18/07.
//  Copyright 2007 Perian Project. All rights reserved.
//

#import <Cocoa/Cocoa.h>

enum {S_LeftAlign = 0, S_CenterAlign, S_RightAlign};
enum {S_BottomAlign = 0, S_MiddleAlign, S_TopAlign};

typedef struct ssacolors {
	ATSURGBAlphaColor primary, secondary, outline, shadow;
} ssacolors;

typedef struct ssastyleline {
	NSString *name, *font;
	float fsize;
	ssacolors color;
	Boolean bold, italic, underline, strikeout;
	float scalex, scaley, tracking, angle;
	int borderstyle;
	float outline, shadow;
	int alignment, halign, valign;
	int marginl, marginr, marginv;
	ATSUTextLayout layout;
	ATSUStyle atsustyle;
	int usablewidth;
} ssastyleline;

@interface SSAEvent : NSObject
{
	@public
	NSString *line;
	unsigned begin_time, end_time;
}
@end

@interface SSADocument : NSObject {
	NSArray *_lines;
	NSString *header;
	
	@public
		
	int stylecount;
	ssastyleline *styles;
	
	float resX, resY;
	double timescale;
	enum {Normal, Reverse} collisiontype;
	enum {S_ASS, S_SSA} version;
}
-(NSString *)header;
-(SSAEvent *)movPacket:(int)i;
-(unsigned)packetCount;
@end

extern int SSA2ASSAlignment(int a);
