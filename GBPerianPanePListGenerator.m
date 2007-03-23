/*
 *  GBPerianPanePListGenerator.c
 *  Perian
 *
 *  Created by Graham Booker on 1/6/07.
 *  Copyright 2007 Graham Booker. All rights reserved.
 *
 */

#include "GBPerianPanePListGenerator.h"
#import "CPFPerianPrefPaneController.h"

int main(int argc, char *argv[])
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *filename = [NSString stringWithUTF8String:argv[1]];
	NSString *componentDir = [NSString stringWithUTF8String:argv[2]];
	
	NSMutableDictionary *plist = [[NSDictionary dictionaryWithContentsOfFile:filename] mutableCopy];
	NSMutableArray *components = [NSMutableArray array];
	
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSArray *types = [NSArray arrayWithObjects:@"QuickTime", @"CoreAudio", @"Frameworks", nil];
	NSArray *extensions = [NSArray arrayWithObjects:@"component", @"component", @"framework", nil];
	int i;
	
	for(i=0; i<[types count]; i++)
	{
		NSString *directory = [componentDir stringByAppendingPathComponent:[types objectAtIndex:i]];
		NSString *extension = [extensions objectAtIndex:i];
		NSEnumerator *dirEnum = [[fileManager directoryContentsAtPath:directory] objectEnumerator];
		NSString *candidate = nil;
		
		while((candidate = [dirEnum nextObject]) != nil)
		{
			if(![[candidate pathExtension] isEqualToString:extension])
				continue;
			
			NSString *candidatePath = [directory stringByAppendingPathComponent:candidate];
			NSDictionary *info = [[NSBundle bundleWithPath:candidatePath] infoDictionary];
			if(info == nil)
				continue;
			
			NSDictionary *componentInfo = [NSDictionary dictionaryWithObjectsAndKeys:
				[info objectForKey:BundleVersionKey], BundleVersionKey,
				candidate, ComponentNameKey,
				[[candidate stringByDeletingPathExtension] stringByAppendingPathExtension:@"zip"], ComponentArchiveNameKey,
				[NSNumber numberWithInt:i], ComponentTypeKey,
				nil];
			[components addObject:componentInfo];
		}
	}
	[plist setObject:components forKey:ComponentInfoDictionaryKey];
	[plist writeToFile:[NSString stringWithUTF8String:argv[3]] atomically:YES];
	[pool release];
	
	return 0;
}