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
NS_ASSUME_NONNULL_BEGIN

@class SubSerializer, SubRenderer;
@class SubRenderSpan;

typedef NS_OPTIONS(uint16_t, SubRenderAnimations) {
	SubRenderAnimationNone = 0,
	SubRenderAnimationSimpleFade = 1 << 0,
	SubRenderAnimationCopmlexFade = 1 << 1,
};

typedef struct SubRenderFad {
	int startTime;
	int endTime;
} SubRenderFad;

typedef struct SubRenderFade {
	CGFloat a1;
	CGFloat a2;
	CGFloat a3;
	int startTime;
	int t2;
	int t3;
	int endTime;
} SubRenderFade;

@interface SubRenderDiv : NSObject {
	@public;
	NSString *text;
	SubStyle *styleLine;
	NSArray<SubRenderSpan*>  *spans;

	CGFloat posX, posY;
	int marginL, marginR, marginV, layer;
	SubAlignmentH alignH;
	SubAlignmentV alignV;
	SubLineWrap wrapStyle;
	UInt8 render_complexity;
	BOOL positioned, shouldResetPens;
	CGFloat scale;
	SubRenderAnimations runningAnimations;
	SubRenderFad simpleFade;
	SubRenderFade complexFade;
}
@property (copy, nullable) NSString *text;
@property (strong, nullable) SubStyle *styleLine;
@property (copy, nullable) NSArray<SubRenderSpan*> *spans;
@property CGFloat posX;
@property CGFloat posY;
@property int leftMargin;
@property int rightMargin;
@property int verticalMargin;
@property int layer;
@property SubAlignmentH alignH;
@property SubAlignmentV alignV;
@property SubLineWrap wrapStyle;
@property UInt8 renderComplexity;
@property (getter=isPositioned) BOOL positioned;
@property BOOL shouldResetPens;

@property CGFloat scale;
@property SubRenderAnimations runningAnimations;
@property SubRenderFad simpleFade;
@property SubRenderFade complexFade;

@end

@interface SubRenderSpan : NSObject <NSCopying> {
	id extra;
	@public;
	UniCharArrayOffset offset;
}
@property (retain) id extra;
@property (readonly) UniCharArrayOffset offset;
@end

extern SubRGBAColor SubParseSSAColor(unsigned rgb);
extern SubRGBAColor SubParseSSAColorString(NSString *c);

extern UInt8 SubASSFromSSAAlignment(UInt8 a);
extern void  SubParseASSAlignment(UInt8 a, SubAlignmentH *alignH, SubAlignmentV *alignV) NS_REFINED_FOR_SWIFT;
extern BOOL  SubParseFontVerticality(NSString *_Nonnull* _Nonnull fontname) NS_REFINED_FOR_SWIFT;

extern void     SubParseSSAFile(NSString *ssa, NSDictionary<NSString*,NSString*> *_Nonnull*_Nonnull headers, NSArray<NSDictionary<NSString*,NSString*>*> *_Nonnull*_Nullable styles, NSArray<NSDictionary<NSString*,NSString*>*> *_Nonnull*_Nullable subs) NS_REFINED_FOR_SWIFT;
extern NSArray<SubRenderDiv*> *SubParsePacket(NSString *packet, SubContext *context, SubRenderer *_Nullable delegate);

NS_ASSUME_NONNULL_END
__END_DECLS
