/*
 * SubParsing.h
 * Created by Alexander Strange on 7/25/07.
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
#import "SubContext.h"

__BEGIN_DECLS

@class SubSerializer, SubRenderer;

@interface SubRenderDiv : NSObject {
	@public;
	NSMutableString *text;
	SubStyle *styleLine;
	NSMutableArray *spans;

	float posX, posY;
	unsigned marginL, marginR, marginV, layer;
	UInt8 alignH, alignV, wrapStyle, render_complexity;
	BOOL positioned, shouldResetPens;	
}
-(SubRenderDiv*)nextDivWithDelegate:(SubRenderer*)delegate;
@end

@interface SubRenderSpan : NSObject {
	@public;
	UniCharArrayOffset offset;
	id ex;
	SubRenderer *delegate;
}
+(SubRenderSpan*)startingSpanForDiv:(SubRenderDiv*)div delegate:(SubRenderer*)delegate;
-(SubRenderSpan*)cloneWithDelegate:(SubRenderer*)delegate;
@end

SubRGBAColor SubParseSSAColor(unsigned rgb);
SubRGBAColor SubParseSSAColorString(NSString *c);
UInt8 SubASSFromSSAAlignment(UInt8 a);
void SubParseASSAlignment(UInt8 a, UInt8 *alignH, UInt8 *alignV);
BOOL SubParseFontVerticality(NSString **fontname);
	
void SubParseSSAFile(NSString *ssa, NSDictionary **headers, NSArray **styles, NSArray **subs);
NSArray *SubParsePacket(NSString *packet, SubContext *context, SubRenderer *delegate);

__END_DECLS