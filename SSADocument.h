/*
 *  SSARenderCodec.m
 *  Copyright (c) 2007 Perian Project
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; 
 *  version 2.1 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

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
	ssastyleline *defaultstyle;
	
	float resX, resY;
	double timescale;
	enum {Normal, Reverse} collisiontype;
	enum {S_ASS, S_SSA} version;
	Boolean disposedefaultstyle;
}
-(NSString *)header;
-(SSAEvent *)movPacket:(int)i;
-(void) loadHeader:(NSString*)path width:(float)width height:(float)height;
-(unsigned)packetCount;
-(void)loadDefaultsWithWidth:(float)width height:(float)height;
@end

extern int SSA2ASSAlignment(int a);
