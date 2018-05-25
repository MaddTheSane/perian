/*
 * SubRenderer.h
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

#ifndef __PERIAN__SUBRENDERER_H__
#define __PERIAN__SUBRENDERER_H__

#include <ApplicationServices/ApplicationServices.h>

__BEGIN_DECLS

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>

CF_ASSUME_NONNULL_BEGIN

@class SubStyle, SubStyleExtra;
@class SubContext, SubRenderDiv, SubRenderSpan, SubRenderSpanExtra;

typedef NS_ENUM(int, SubSSATagName) {
	tag_b=0, tag_i, tag_u, tag_s, tag_bord, tag_shad, tag_be,
	tag_fn, tag_fs, tag_fscx, tag_fscy, tag_fsp, tag_frx,
	tag_fry, tag_frz, tag_1c, tag_2c, tag_3c, tag_4c, tag_alpha,
	tag_1a, tag_2a, tag_3a, tag_4a, tag_r, tag_p,
	tag_t, tag_pbo, tag_fad
};

@interface SubRenderer : NSObject
- (instancetype)init NS_DESIGNATED_INITIALIZER;

-(void)didCompleteHeaderParsing:(SubContext*)sc;
-(void)didCompleteStyleParsing:(SubStyle*)s;

-(void)didCreateStartingSpan:(SubRenderSpan*)span forDiv:(SubRenderDiv*)div NS_SWIFT_NAME(didCreateStartingSpan(_:for:));

-(void)spanChangedTag:(SubSSATagName)tag span:(SubRenderSpan*)span div:(SubRenderDiv*)div param:(void*)p NS_SWIFT_NAME(spanChanged(tag:span:div:param:));

-(CGFloat)aspectRatio;
@property (readonly) CGFloat aspectRatio;
-(void)renderPacket:(NSString *)packet inContext:(CGContextRef)c width:(CGFloat)cWidth height:(CGFloat)cHeight NS_SWIFT_NAME(render(packet:in:width:height:));
@end

#else
CF_ASSUME_NONNULL_BEGIN
#endif

typedef struct CF_BRIDGED_TYPE(SubRenderer) __SubRendererPtr *SubRendererRef;

// these are actually implemented in SubATSUIRenderer.m
SubRendererRef _Nullable SubRendererCreate(bool isSSA, char *header, size_t headerLen, int width, int height) CF_RETURNS_RETAINED;
void SubRendererPrerollFromHeader(char *header, int headerLen);
void SubRendererRenderPacket(SubRendererRef s, CGContextRef c, CFStringRef str, int cWidth, int cHeight);
void SubRendererDispose(CF_CONSUMED SubRendererRef s);

CF_ASSUME_NONNULL_END
__END_DECLS

#endif
