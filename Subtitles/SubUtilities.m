//
//  SubUtilities.m
//  SSARender2
//
//  Created by Alexander Strange on 7/28/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

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
	mergesort(objs, count, sizeof(void*), compare);
	[array setArray:[NSArray arrayWithObjects:objs count:count]];
}

static BOOL DifferentiateLatin12(const unsigned char *data, int length)
{
	// generated from french/german (latin1) and hungarian/slovak (latin2)
	
	const short frequencies[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		504, 1024, -192, -403, 433, -1106, -1376, -865, 447, -1894, -1550, 878, -2285, -385, 1616, -1292, 
		27, -2306, -1328, 57, 402, 194, -338, -1509, 0, 944, 1471, 0, 0, 0, 0, 0, 
		0, 1891, 1663, 2461, 226, -4595, -1174, -1672, -1096, 2218, 3418, 4572, -1075, 2244, -2666, 855, 
		279, -4997, -3202, -4819, -627, -1330, 855, -1276, -2190, 3607, 810, 0, 1555, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 618, 0, 0, 0, 357, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4220, 0, -1279, 504, 4677, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2366, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 1747, 0, 0, 0, 5936, 0, 0, 0, 0, 0, 
		-1107, 0, 0, 798, -639, 0, 0, -1107, 874, -751, 0, 0, 0, 0, 1953, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 874, 0, 0, 0, 0, 0, 713, -1567, 
		-3675, 6370, 2120, 10526, -1107, 0, 0, -2306, -598, -3012, -3262, 0, 5571, 6644, 2104, 1595, 
		0, 0, 798, 1070, -1430, 0, -1567, 0, 4280, 1004, 713, -904, -1107, 3288, 4539, 0};
	
	int frcount = 0;
	
	while (length--) {
		frcount += frequencies[*data++];
	}
	
	return frcount <= 0;
}

extern NSString *STLoadFileWithUnknownEncoding(NSString *path)
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
	latin2 = [enc_str isEqualToString:@"windows-1250"];
	
	if (latin2) {
		if (DifferentiateLatin12([data bytes], [data length])) { // seems to actually be latin1
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

CFMutableStringRef GetHomeDirectory()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *home = NSHomeDirectory();
	CFMutableStringRef mhome = CFStringCreateMutableCopy(NULL, 0, (CFStringRef)home);
	[pool release];
	
	return mhome;
}