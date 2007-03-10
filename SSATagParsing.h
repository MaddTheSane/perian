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
#import "SSADocument.h"

@interface SSAStyleSpan : NSObject
{
	@public
	float outline, shadow;
	ATSUStyle astyle;
	NSRange range;
	BOOL outlineblur;
	ssacolors color;
}
@end

@interface SSARenderEntity : NSObject
{
	@public
	ATSUTextLayout layout;
	int layer;
	ssastyleline *style;
	int marginl, marginr, marginv, usablewidth;
	int posx, posy, halign, valign;
	NSString *nstext;
	unichar *text;
	SSAStyleSpan **styles;
	size_t style_count;
	BOOL disposelayout, multipart_drawing;
	int is_shape;
}
@end

extern NSArray *ParseSubPacket(NSString *str, SSADocument *ssa, Boolean plaintext);
extern void SetATSUStyleFlag(ATSUStyle style, ATSUAttributeTag t, Boolean v);
extern void SetATSUStyleOther(ATSUStyle style, ATSUAttributeTag t, ByteCount s, const ATSUAttributeValuePtr v);
extern void SetATSULayoutOther(ATSUTextLayout l, ATSUAttributeTag t, ByteCount s, const ATSUAttributeValuePtr v);
