/*
 * SubUtilities.m
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

#import "SubUtilities.h"
#import "UniversalDetector.h"
#import "Codecprintf.h"
#import "ARCBridge.h"

NSArray *SubSplitStringIgnoringWhitespace(NSString *str, NSString *split)
{
	NSMutableArray *values = [[str componentsSeparatedByString:split] mutableCopy];
	NSCharacterSet *wcs = [NSCharacterSet whitespaceCharacterSet];
	size_t num = [values count], i;
	
	for (i = 0; i < num; i++) values[i] = [values[i] stringByTrimmingCharactersInSet:wcs];
	
#if __has_feature(objc_arc)
	return [values copy];
#else
	NSArray *tmp = [values copy];
	[values release];
	return [tmp autorelease];
#endif
}

NSArray *SubSplitStringWithCount(NSString *str, NSString *split, NSInteger count)
{
	NSMutableArray *ar = [NSMutableArray arrayWithCapacity:count];
	NSScanner *sc = [NSScanner scannerWithString:str];
	NSString *scv=nil;
	[sc setCharactersToBeSkipped:nil];
	[sc setCaseSensitive:TRUE];
	
	while (count != 1) {
		count--;
		[sc scanUpToString:split intoString:&scv];
		[sc scanString:split intoString:nil];
		if (!scv) scv = [NSString string];
		[ar addObject:scv];
		if ([sc isAtEnd]) break;
		scv = nil;
	}
	
	[sc scanUpToString:@"" intoString:&scv];
	if (!scv) scv = [NSString string];
	[ar addObject:scv];
	
	return ar;
}

NSString *SubStandardizeStringNewlines(NSString *str)
{
	if(str == nil)
		return nil;
	str = [str stringByReplacingOccurrencesOfString:@"\r\n" withString:@"\n"];
	str = [str stringByReplacingOccurrencesOfString:@"\r" withString:@"\n"];
	return str;
}

static const short frequencies[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 1026, 29, -1258, 539, -930, -652, -815, -487, -2526, -2161, 146, -956, -914, 1149, -102, 
	293, -2675, -923, -597, 339, 110, 247, 9, 0, 1024, 1239, 0, 0, 0, 0, 0, 
	0, 1980, 1472, 1733, -304, -4086, 273, 582, 333, 2479, 1193, 5014, -1039, 1964, -2025, 1083, 
	-154, -5000, -1725, -4843, -366, -1850, -191, 1356, -2262, 1648, 1475, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, -458, 0, 0, 0, 0, 300, 0, 0, 300, 601, 0, 
	0, 0, -2247, 0, 0, 0, 0, 0, 0, 0, 3667, 0, 0, 3491, 3567, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1993, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 1472, 0, 0, 0, 5000, 0, 601, 0, 1993, 0, 
	0, 1083, 0, 672, -458, 0, 0, -458, 1409, 0, 0, 0, 0, 0, 1645, 425, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 601, -1123, 
	-1912, 4259, 2573, 8866, 55, 0, 0, -2247, -831, -3788, -3043, 0, 0, 3412, 2921, 1251, 
	0, 0, 1377, 520, 1344, 0, -1123, 0, 0, -1213, 2208, -458, -794, 2636, 3824, 0};

BOOL SubDifferentiateLatin12(const unsigned char *data, NSInteger length)
{
	// generated from french/german (latin1) and hungarian/romanian (latin2)
	
	NSInteger frcount = 0;
	
	while (length--) {
		frcount += frequencies[*data++];
	}
	
	return frcount <= 0;
}

static NSString *SubloadDataWithEncoding(NSData *data, NS_RELEASES_ARGUMENT UniversalDetector *ud, NSString *path)
{
	NSStringEncoding enc = [ud encoding];
	float conf = [ud confidence];
	NSString *enc_str = [ud MIMECharset];
	BOOL latin2 = enc == NSWindowsCP1250StringEncoding;
	
	if (latin2) {
		if (SubDifferentiateLatin12([data bytes], [data length])) { // seems to actually be latin1
			enc = NSWindowsCP1252StringEncoding;
			enc_str = @"windows-1252";
		}
	}
	
	if ((conf < .6 || latin2) && enc != NSASCIIStringEncoding) {
		Codecprintf(NULL,"Guessed encoding \"%s\" for \"%s\", but not sure (confidence %f%%).\n",[enc_str UTF8String],[path UTF8String],conf*100.);
	}
	
	NSString *res = AUTORELEASEOBJ([[NSString alloc] initWithData:data encoding:enc]);
	
	if (!res) {
		if (latin2) {
			Codecprintf(NULL,"Encoding %s failed, retrying.\n",[enc_str UTF8String]);
			enc = (enc == NSWindowsCP1252StringEncoding) ? NSWindowsCP1250StringEncoding : NSWindowsCP1252StringEncoding;
			res = AUTORELEASEOBJ([[NSString alloc] initWithData:data encoding:enc]);
			if (!res) Codecprintf(NULL,"Both of latin1/2 failed.\n");
		} else Codecprintf(NULL,"Failed to load file as guessed encoding %s.\n",[enc_str UTF8String]);
	}
	RELEASEOBJ(ud);
	
	return res;
}

NSString *SubLoadDataWithUnknownEncoding(NSData *data)
{
	UniversalDetector *ud = [[UniversalDetector alloc] init];
	
	[ud analyzeData:data];
	
	return SubloadDataWithEncoding(data, ud, @"<Internal Data>");
}

NSString *SubLoadURLWithUnknownEncoding(NSURL *path)
{
	NSData *data = [NSData dataWithContentsOfURL:path options:NSDataReadingMappedIfSafe error:NULL];
	
	if (!data) {
		return nil;
	}
	
	UniversalDetector *ud = [[UniversalDetector alloc] init];
	
	[ud analyzeData:data];

	return SubloadDataWithEncoding(data, ud, [path path]);
}

NSString *SubLoadFileWithUnknownEncoding(NSString *path)
{
	NSData *data = [NSData dataWithContentsOfFile:path];
	if (!data) {
		return nil;
	}
	
	UniversalDetector *ud = [[UniversalDetector alloc] init];
	
	[ud analyzeData:data];
	
	return SubloadDataWithEncoding(data, ud, path);
}

const unichar *SubUnicodeForString(NSString *str, NSData *__strong*datap)
{
	const unichar *p = CFStringGetCharactersPtr((CFStringRef)str);
	
	*datap = nil;
	
	if (!p) {
		NSData *data = RETAINOBJ([str dataUsingEncoding:NSUnicodeStringEncoding]);
		
		p = [data bytes];
		
		//dataUsingEncoding: adds a BOM
		//skip it so the string length will match the input string
		if (*p == 0xfeff)
			p++;
		
		*datap = data;
	}
	
	return p;
}

CFMutableStringRef CopyHomeDirectory()
{	
	@autoreleasepool {
		NSString *home = NSHomeDirectory();
		return CFStringCreateMutableCopy(kCFAllocatorDefault, 0, (CFStringRef)home);
	}
}

CGPathRef CreateSubParseSubShapesWithString(NSString *aStr, const CGAffineTransform * __nullable m)
{
	CGMutablePathRef bPath = CGPathCreateMutable();
	NSScanner *scanner = [NSScanner scannerWithString:aStr];
	NSString *scanned;
	NSString *oldScanned = @"";
	NSCharacterSet *alphaOnly = [NSCharacterSet characterSetWithCharactersInString:@"mnlbspc"];
	while (([scanner scanCharactersFromSet:alphaOnly intoString:&scanned] || oldScanned.length != 0) && !scanner.isAtEnd) {
		if (scanned.length != 0) {
			oldScanned = scanned;
		}
		
		if ([oldScanned isEqualToString:@"m"] || [oldScanned isEqualToString:@"n"]) {
			double x = 0, y = 0;
			if ([scanner scanDouble:&x] && [scanner scanDouble:&y]) {
				CGPathMoveToPoint(bPath, m, x, y);
			}
		} else if ([oldScanned isEqualToString:@"b"]) {
			double startPtX = 0;
			double startPtY = 0;
			double ctrlPt1X = 0;
			double ctrlPt1Y = 0;
			double ctrlPt2X = 0;
			double ctrlPt2Y = 0;
			if ([scanner scanDouble:&ctrlPt1X] && [scanner scanDouble:&ctrlPt1Y] &&
				[scanner scanDouble:&ctrlPt2X] && [scanner scanDouble:&ctrlPt2Y] &&
				[scanner scanDouble:&startPtX] && [scanner scanDouble:&startPtY]) {
				CGPathAddCurveToPoint(bPath, m, ctrlPt1X, ctrlPt1Y, ctrlPt2X, ctrlPt2Y, startPtX, startPtY);
			}
		} else if ([oldScanned isEqualToString:@"c"]) {
			CGPathCloseSubpath(bPath);
		} else if ([oldScanned isEqualToString:@"l"]) {
			double x = 0, y = 0;
			if ([scanner scanDouble:&x] && [scanner scanDouble:&y]) {
				CGPathAddLineToPoint(bPath, m, x, y);
			}
		} else {
			// unhandled switches: p s
			[scanner scanUpToCharactersFromSet:NSCharacterSet.whitespaceCharacterSet intoString:nil];
		}
	}
	
	CGPathCloseSubpath(bPath);
	return bPath;
}
