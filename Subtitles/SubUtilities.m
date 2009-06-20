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

NSArray *STSplitStringIgnoringWhitespace(NSString *str, NSString *split)
{
	NSArray *tmp = [str componentsSeparatedByString:split];
	NSCharacterSet *wcs = [NSCharacterSet whitespaceCharacterSet];
	size_t num = [tmp count], i;
	NSString *values[num];
	
	[tmp getObjects:values];
	for (i = 0; i < num; i++) values[i] = [values[i] stringByTrimmingCharactersInSet:wcs];
	
	return [NSArray arrayWithObjects:values count:num];
}

NSArray *STSplitStringWithCount(NSString *str, NSString *split, size_t count)
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

NSMutableString *STStandardizeStringNewlines(NSString *str)
{
	if(str == nil)
		return nil;
	NSMutableString *ms = [NSMutableString stringWithString:str];
	[ms replaceOccurrencesOfString:@"\r\n" withString:@"\n" options:0 range:NSMakeRange(0,[ms length])];
	[ms replaceOccurrencesOfString:@"\r" withString:@"\n" options:0 range:NSMakeRange(0,[ms length])];
	return ms;
}

void STSortMutableArrayStably(NSMutableArray *array, int (*compare)(const void *, const void *))
{
	int count = [array count];
	id  objs[count];
	
	[array getObjects:objs];
	mergesort(objs, count, sizeof(id), compare);
	[array setArray:[NSArray arrayWithObjects:objs count:count]];
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

BOOL STDifferentiateLatin12(const unsigned char *data, int length)
{
	// generated from french/german (latin1) and hungarian/romanian (latin2)
	
	int frcount = 0;
	
	while (length--) {
		frcount += frequencies[*data++];
	}
	
	return frcount <= 0;
}

NSString *STLoadFileWithUnknownEncoding(NSString *path)
{
	NSData *data = [NSData dataWithContentsOfMappedFile:path];
	UniversalDetector *ud = [[UniversalDetector alloc] init];
	NSString *res = nil;
	NSStringEncoding enc;
	float conf;
	NSString *enc_str;
	BOOL latin2;
	
	[ud analyzeData:data];
	
	enc = [ud encoding];
	conf = [ud confidence];
	enc_str = [ud MIMECharset];
	latin2 = enc == NSWindowsCP1250StringEncoding;
	
	if (latin2) {
		if (STDifferentiateLatin12([data bytes], [data length])) { // seems to actually be latin1
			enc = NSWindowsCP1252StringEncoding;
			enc_str = @"windows-1252";
		}
	}
	
	if (conf < .6 || latin2) {
		Codecprintf(NULL,"Guessed encoding \"%s\" for \"%s\", but not sure (confidence %f%%).\n",[enc_str UTF8String],[path UTF8String],conf*100.);
	}
	
	res = [[[NSString alloc] initWithData:data encoding:enc] autorelease];
	
	if (!res) {
		if (latin2) {
			Codecprintf(NULL,"Encoding %s failed, retrying.\n",[enc_str UTF8String]);
			enc = (enc == NSWindowsCP1252StringEncoding) ? NSWindowsCP1250StringEncoding : NSWindowsCP1252StringEncoding;
			res = [[[NSString alloc] initWithData:data encoding:enc] autorelease];
			if (!res) Codecprintf(NULL,"Both of latin1/2 failed.\n",[enc_str UTF8String]);
		} else Codecprintf(NULL,"Failed to load file as guessed encoding %s.\n",[enc_str UTF8String]);
	}
	[ud release];
	
	return res;
}

const unichar *STUnicodeForString(NSString *str, NSData **datap)
{
	const unichar *p = CFStringGetCharactersPtr((CFStringRef)str);
	
	*datap = nil;
	
	if (!p) {
		NSData *data = [[str dataUsingEncoding:NSUnicodeStringEncoding] retain];
		
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
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *home = NSHomeDirectory();
	CFMutableStringRef mhome = CFStringCreateMutableCopy(NULL, 0, (CFStringRef)home);
	[pool release];
	
	return mhome;
}