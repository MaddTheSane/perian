/*
 * detect-encoding
 * Created by Alexander Strange on 5/16/09.
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
#import "SubUtilities.h"
#import "UniversalDetector.h"

int main(int argc, char *argv[])
{
	if (argc != 2)
		return 1;
	
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	UniversalDetector *ud = [[UniversalDetector alloc] init];
	NSData *data = [NSData dataWithContentsOfMappedFile:[NSString stringWithUTF8String:argv[1]]];
	
	[ud analyzeData:data];
	NSStringEncoding enc = [ud encoding];
	
	printf("%s:\n", argv[1]);
	printf("\tUniversalDetector: \"%s\" (%#x) with %f%% confidence\n", [[ud MIMECharset] UTF8String], enc, [ud confidence]*100.f);
	
	if (enc == NSWindowsCP1250StringEncoding || enc == NSWindowsCP1252StringEncoding) {
		enc = STDifferentiateLatin12([data bytes], [data length]) ? NSWindowsCP1252StringEncoding : NSWindowsCP1250StringEncoding;
		NSString *encName = (NSString*)CFStringConvertEncodingToIANACharSetName(CFStringConvertNSStringEncodingToEncoding(enc));
		printf("\tLatin12: \"%s\" (%#x)\n\n", [encName UTF8String], enc);
	}
	
	[ud debugDump];
	[pool release];
	
	return 0;
}