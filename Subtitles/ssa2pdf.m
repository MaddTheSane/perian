/*
 * ssa2pdf
 * Created by Alexander Strange on 8/11/09.
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

#import <ApplicationServices/ApplicationServices.h>
#import "SubImport.h"
#import "SubParsing.h"

// vv These are Apple private API! Don't use them just because this does!
// I have a good reason for it, I promise!
extern CGContextRef CGPSContextCreateWithURL(CFURLRef url, const CGRect *mediaBox, CFDictionaryRef auxiliaryInfo);
extern void CGPSContextClose(CGContextRef c);

int main(int argc, char *argv[])
{	
	if (argc != 3)
		return 1;

	NSAutoreleasePool *outer_pool = [[NSAutoreleasePool alloc] init];
	SubContext *sc; SubSerializer *ss = [[SubSerializer alloc] init];
	NSString *inFile = [NSString stringWithUTF8String:argv[1]], *outDir = [NSString stringWithUTF8String:argv[2]];
	int i = 0;

	//loading copied from ssa2html, still duplicated
	NSString *header = SubLoadSSAFromPath(inFile, ss);
	[ss setFinished:YES];
	
	NSDictionary *headers;
	NSArray *styles;
	SubParseSSAFile(header, &headers, &styles, NULL);
	sc = [[SubContext alloc] initWithHeaders:headers styles:styles delegate:NULL];
	int width = sc->resX, height = sc->resY;
	CGRect rect = CGRectMake(0, 0, width, height);
	SubRendererPtr s = SubRendererCreateWithSSA((char*)[header UTF8String], [header length], width, height);

	while (![ss isEmpty]) {
		NSAutoreleasePool *inner_pool = [[NSAutoreleasePool alloc] init];
		SubLine *sl = [ss getSerializedPacket];
		if ([sl->line length] > 1) {
			NSString *pdf = [outDir stringByAppendingPathComponent:[NSString stringWithFormat:@"%d.pdf", i]];
			CGContextRef pdfC = CGPDFContextCreateWithURL((CFURLRef)[NSURL fileURLWithPath:pdf], &rect, NULL);
			CGContextBeginPage(pdfC, NULL);
			SubRendererRenderPacket(s, pdfC, (CFStringRef)sl->line, width, height);
			CGContextEndPage(pdfC);
			CGContextRelease(pdfC);
			
			NSString *ps = [outDir stringByAppendingPathComponent:[NSString stringWithFormat:@"%d.eps", i]];
			CGContextRef psC = CGPSContextCreateWithURL((CFURLRef)[NSURL fileURLWithPath:ps], &rect, NULL);
			CGContextBeginPage(psC, NULL);
			SubRendererRenderPacket(s, psC, (CFStringRef)sl->line, width, height);
			CGContextEndPage(psC);
			CGContextRelease(psC);

			i++;
		}
		[inner_pool release];
	}
	
	[ss release];
	[outer_pool release];

	return 0;
}
