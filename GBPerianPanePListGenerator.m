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
	@autoreleasepool {
		NSString *filename = @(argv[1]);
		NSString *componentDir = @(argv[2]);
		
		NSMutableDictionary *plist = [[NSDictionary dictionaryWithContentsOfFile:filename] mutableCopy];
		NSMutableArray *components = [NSMutableArray array];
		
		NSFileManager *fileManager = [NSFileManager defaultManager];
		NSArray *types = @[@"QuickTime", @"CoreAudio", @"Frameworks"];
		NSArray *extensions = @[@"component", @"component", @"framework"];
		int i;
		
		for(i=0; i<[types count]; i++)
		{
			NSString *directory = [componentDir stringByAppendingPathComponent:[types objectAtIndex:i]];
			NSString *extension = [extensions objectAtIndex:i];
			NSArray *dirContents = [fileManager contentsOfDirectoryAtPath:directory error:NULL];
			
			for (NSString *candidate in dirContents) {
				if(![[candidate pathExtension] isEqualToString:extension])
					continue;
				
				NSString *candidatePath = [directory stringByAppendingPathComponent:candidate];
				NSDictionary *info = [[NSBundle bundleWithPath:candidatePath] infoDictionary];
				if(info == nil)
					continue;
				
				NSDictionary *componentInfo = @{BundleVersionKey: info[BundleVersionKey],
											   ComponentNameKey: candidate,
											   ComponentArchiveNameKey: [[candidate stringByDeletingPathExtension] stringByAppendingPathExtension:@"zip"],
											   ComponentTypeKey: @(i)};
				[components addObject:componentInfo];
			}
		}
		plist[ComponentInfoDictionaryKey] = components;
		[plist writeToFile:@(argv[3]) atomically:YES];
		
		return EXIT_SUCCESS;
	}
}