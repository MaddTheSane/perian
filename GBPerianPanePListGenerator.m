/*
 * CPFPerianPrefPaneController.h
 * Created by Graham Booker on 1/6/07.
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
		//note: the warning below can't be fixed, the method's replacement isn't in 10.4
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
    [plist release];
	[pool release];
	
	return 0;
}