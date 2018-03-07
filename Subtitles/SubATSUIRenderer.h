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

#import <Cocoa/Cocoa.h>
#import "SubRenderer.h"

@class SubRenderer, SubContext;

NS_ASSUME_NONNULL_BEGIN

@interface SubATSUIRenderer : SubRenderer {
	SubContext *context;

	ATSUTextLayout layout;
	CGColorSpaceRef srgbCSpace;
	UniCharArrayOffset *breakBuffer;
	TextBreakLocatorRef breakLocator;
	
	CGFloat screenScaleX, screenScaleY, videoWidth, videoHeight;
	BOOL drawTextBounds;
}
- (instancetype)init UNAVAILABLE_ATTRIBUTE;
-(nullable instancetype)initWithScriptType:(int)type header:(nullable NSString*)header videoWidth:(CGFloat)width videoHeight:(CGFloat)height NS_DESIGNATED_INITIALIZER;
-(void)renderPacket:(NSString *)packet inContext:(CGContextRef)c width:(CGFloat)cWidth height:(CGFloat)cHeight;
@end

NS_ASSUME_NONNULL_END
