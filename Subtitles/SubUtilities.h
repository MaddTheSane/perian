/*
 * SubUtilities.h
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

#import <Foundation/Foundation.h>

__BEGIN_DECLS
NS_ASSUME_NONNULL_BEGIN

NSArray<NSString*>  *SubSplitStringIgnoringWhitespace(NSString *str, NSString *split);
NSArray<NSString*>  *SubSplitStringWithCount(NSString *str, NSString *split, NSInteger count);
NSString *_Nullable SubLoadFileWithUnknownEncoding(NSString *path);
NSString *_Nullable SubLoadURLWithUnknownEncoding(NSURL *path);
NSString *_Nullable SubLoadDataWithUnknownEncoding(NSData *data);

NSString *SubStandardizeStringNewlines(NSString *str);

BOOL SubDifferentiateLatin12(const unsigned char *data, NSInteger length);

const unichar *__nullable SubUnicodeForString(NSString *str, NSData * __nonnull __strong* __nullable NS_RETURNS_RETAINED datap);
CGPathRef CreateSubParseSubShapesWithString(NSString *aStr, const CGAffineTransform * __nullable m) CF_RETURNS_RETAINED;

NS_ASSUME_NONNULL_END
__END_DECLS
