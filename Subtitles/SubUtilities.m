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
		513, 1000, -196, -338, 497, -1420, -1356, -850, 452, -1961, -1513, 726, -2247, -367, 1490, -1300, 
		-158, -2306, -1420, 16, 352, 226, -330, -1495, 0, 959, 1308, 0, 0, 0, 0, 0, 
		0, 1845, 1743, 2658, 234, -4533, -1098, -1782, -1138, 2185, 3159, 4390, -1125, 2217, -2643, 647, 
		297, -4997, -3176, -4854, -505, -1176, 744, -1243, -2163, 3706, 763, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 628, 0, 0, 0, 363, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3989, 0, -1279, 513, 4714, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2405, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 1777, 0, 0, 0, 6035, 0, 0, 0, 0, 0, 
		-1107, 0, 0, 811, -639, 0, 0, -1107, 725, -745, 0, 0, 0, 0, 1986, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 888, 0, 0, 0, 0, 0, 725, -1567, 
		-3675, 6477, 2190, 10702, -1107, 0, 0, -2306, -824, -2951, -3262, 0, 5665, 6755, 2178, 1622, 
		0, 0, 811, 1088, -1430, 0, -1567, 0, 4352, 1048, 725, -904, -1107, 3343, 4616, 0};
	
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
	
	if (!res) Codecprintf(NULL,"Failed to load file as guessed encoding %s.\n",[enc_str UTF8String]);
	[ud release];
	
	return res;
}