//
//  Categories.m
//  SSAView
//
//  Created by Alexander Strange on 1/18/07.
//  Copyright 2007 Perian Project. All rights reserved.
//

#import "Categories.h"
#import "UniversalDetector.h"
#import "Codecprintf.h"

@implementation NSCharacterSet(STUtilities)
+ (NSCharacterSet *)newlineCharacterSet
{
	const unichar chars[] = {'\r','\n',0x0085,0x2028,0x2029};
	return [NSCharacterSet characterSetWithCharactersInString:[NSString stringWithCharacters:chars length:5]];
}

+ (NSCharacterSet *)whitespaceAndBomCharacterSet
{
	const unichar bom = 0xfeff;
	NSMutableCharacterSet *cs = [[NSMutableCharacterSet alloc] init]; 

	[cs addCharactersInString:[NSString stringWithCharacters:&bom length:1]];
	
	[cs formUnionWithCharacterSet:[NSCharacterSet whitespaceCharacterSet]];
	
	return [cs autorelease];
}
@end

@implementation NSScanner (STAdditions)
- (int)scanInt
{
	int r;
	[self scanInt:&r];
	return r;
}
@end

@implementation NSString (STAdditions)
- (NSString *)stringByStandardizingNewlines
{
	NSMutableString *ms = [NSMutableString stringWithString:self];
	[ms replaceOccurrencesOfString:@"\r\n" withString:@"\n" options:0 range:NSMakeRange(0,[self length])];
	[ms replaceOccurrencesOfString:@"\r" withString:@"\n" options:0 range:NSMakeRange(0,[ms length])];
	return ms;
}

- (NSArray *)pairSeparatedByString:(NSString *)str
{
	NSMutableArray *ar = [NSMutableArray arrayWithCapacity:2];
	NSRange r = [self rangeOfString:str options:NSLiteralSearch];
	if (r.length == 0) [ar addObject:self];
	else {
		[ar addObject:[self substringToIndex:r.location]];
		[ar addObject:[self substringFromIndex:r.location + r.length]];
	}
	return ar;
}

- (NSArray *)componentsSeparatedByString:(NSString *)str count:(int)count
{
	NSMutableArray *ar = [NSMutableArray arrayWithCapacity:count];
	NSScanner *sc = [NSScanner scannerWithString:self];
	NSString *scv;
	[sc setCharactersToBeSkipped:nil];
	[sc setCaseSensitive:TRUE];
	
	while (count != 1) {
		count--;
		[sc scanUpToString:str intoString:&scv];
		[sc scanString:str intoString:nil];
		if (scv) [ar addObject:scv]; else [ar addObject:[NSString string]];
		if ([sc isAtEnd]) break;
		scv = nil;
	}
	
	[sc scanUpToString:@"" intoString:&scv];
	if (scv) [ar addObject:scv]; else [ar addObject:[NSString string]];

	return ar;
}

+ (NSString *)stringFromUnknownEncodingFile:(NSString *)file
{
	NSData *data = [NSData dataWithContentsOfMappedFile:file];
	UniversalDetector *ud = [[UniversalDetector alloc] init];
	NSString *res;
	CFStringEncoding enc;
	
	[ud analyzeData:data];
	
	enc = [ud encoding];
	
	if ([ud confidence] < .7) 
		Codecprintf(NULL,"Guessed encoding \"%s\" for \"%s\", but not sure (confidence %f%%).\n",[[ud MIMECharset] UTF8String],[file UTF8String],[ud confidence]*100.);
		
	res = [[[NSString alloc] initWithData:data encoding:enc] autorelease];
	
	if (!res) Codecprintf(NULL,"Failed to load file as guessed encoding %s.\n",[[ud MIMECharset] UTF8String]);
	[ud release];

	return res;
}
@end
