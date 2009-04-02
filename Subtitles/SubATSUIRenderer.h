/*
 * SubATSUIRenderer.h
 * Created by Alexander Strange on 7/30/07.
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

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#import "SubRenderer.h"

@interface SubATSUIRenderer : SubRenderer {
	SubContext *context;
	unichar *ubuffer;
	UniCharArrayOffset *breakbuffer;
	ATSUTextLayout layout;
	float screenScaleX, screenScaleY, videoWidth, videoHeight;
	@public;
	CGColorSpaceRef srgbCSpace;
	TextBreakLocatorRef breakLocator;
}
-(SubATSUIRenderer*)initWithVideoWidth:(float)width videoHeight:(float)height;
-(SubATSUIRenderer*)initWithSSAHeader:(NSString*)header videoWidth:(float)width videoHeight:(float)height;
-(void)renderPacket:(NSString *)packet inContext:(CGContextRef)c width:(float)cWidth height:(float)cHeight;
@end

typedef SubATSUIRenderer *SubtitleRendererPtr;

#else
#include <QuickTime/QuickTime.h>

typedef void *SubtitleRendererPtr;

#endif

extern SubtitleRendererPtr SubInitForSSA(char *header, size_t headerLen, int width, int height);
extern SubtitleRendererPtr SubInitNonSSA(int width, int height);
extern CGColorSpaceRef SubGetColorSpace(SubtitleRendererPtr s);
extern void SubRenderPacket(SubtitleRendererPtr s, CGContextRef c, CFStringRef str, int cWidth, int cHeight);
extern void SubPrerollFromHeader(char *header, int headerLen);
extern void SubDisposeRenderer(SubtitleRendererPtr s);

#ifdef __cplusplus
}
#endif