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

#import <Cocoa/Cocoa.h>

#ifdef __cplusplus
extern "C"
{
#endif
	
extern NSArray *STSplitStringIgnoringWhitespace(NSString *str, NSString *split);
extern NSArray *STSplitStringWithCount(NSString *str, NSString *split, size_t count);
extern NSMutableString *STStandardizeStringNewlines(NSString *str);
extern NSString *STLoadFileWithUnknownEncoding(NSString *path);
extern void STSortMutableArrayStably(NSMutableArray *array, int (*compare)(const void *, const void *));
extern BOOL STDifferentiateLatin12(const unsigned char *data, int length);
#ifdef __cplusplus
}
#endif