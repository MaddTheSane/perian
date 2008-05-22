//
//  ECQTComponent.m
//  QTManager
//
//  Created by Ken on 12/15/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "ECQTComponent.h"

#define DISABLED_SUFFIX @" (Disabled)"

@implementation ECQTComponent

NSMutableDictionary*	mCache = 0;

#define SET_KEY(key, value) [mParameters setObject:(value) forKey:(key)]
#define SET_KEY_BOOLVAL(key, value) [mParameters setObject:[NSNumber numberWithBool:(value)] forKey:(key)]
#define GET_KEY(key) [mParameters objectForKey:(key)]
#define GET_KEY_BOOLVAL(key) (([mParameters objectForKey:(key)]) ? [[mParameters objectForKey:(key)] boolValue] : NO)

+(id)componentWithPath:(NSString*)path
{
	return [[[ECQTComponent alloc] initWithPath:path] autorelease];
}

-(id)initWithPath:(NSString*)path
{
//	NSLog(@"init with %@", path);
#warning TODO(durin42) There's a lot of caching here - I don't think we really need it to be efficient.
	if (!mCache)
	{
		mCache = [[NSMutableDictionary dictionary] retain];
	}

	NSDictionary* cachedParams = [mCache objectForKey:path];
	
	if (cachedParams)
	{
		mParameters = [cachedParams retain];		
	}
	else
	{
		mParameters = [[NSMutableDictionary dictionary] retain];
		
		SET_KEY(@"Location", path);
		[self setComponentInfo];
	}

	SET_KEY_BOOLVAL(@"Enabled", !([path rangeOfString:DISABLED_SUFFIX].length > 0));

	[self setComponentScope];

	return self;
}


-(void)setComponentInfo
{
	short resFile = 0;

	NSString* filePath = GET_KEY(@"Location");
	
	BOOL isDirectory = NO;
	NSFileManager* fm = [NSFileManager defaultManager];
	NSURL* url = [NSURL fileURLWithPath:filePath];
	if (![fm fileExistsAtPath:filePath isDirectory:&isDirectory])
	{
		return;
	}
			
	CFBundleRef bundleRef = 0;
	if (isDirectory)
	{
		bundleRef = CFBundleCreate(0, (CFURLRef)url);

		if (!bundleRef)
			return;
		
		resFile = CFBundleOpenBundleResourceMap(bundleRef);
		
		NSString* cfvers = (NSString*)CFBundleGetValueForInfoDictionaryKey(bundleRef, kCFBundleVersionKey);
		if (cfvers)
			SET_KEY(@"Version", cfvers);
		
	}
	else
	{
		FSRef ref;
		if (CFURLGetFSRef((CFURLRef)url,&ref))
		{
			resFile = FSOpenResFile(&ref, fsRdPerm);
		}
	}

	NSString* name = [NSString stringWithString:@"Unknown"];
	
	short saveRes = CurResFile();
	if (resFile > 0)
	{
		UseResFile(resFile);
		int x = 0;
		int count = CountResources('thng');
		NSString* firstType = 0;
		BOOL mixed = false;
		
		NSString* singletype = 0;
		NSMutableArray* allTypes = [NSMutableArray array];
		NSMutableArray* allNames = [NSMutableArray array];
		
		for (x = 1; x <= count; x++)
		{
			
			Handle data = Get1IndResource('thng', x);
			if (data)
			{
				ComponentResource* thng = (ComponentResource*)*data;

				ComponentDescription desc;

				memcpy(&desc, thng, sizeof(ComponentDescription));

				Handle nameData = Get1Resource(thng->componentName.resType, thng->componentName.resID);

				if (firstType == 0 && singletype)
					firstType = [singletype copy];
				
				if (desc.componentType == 'vdig')
					singletype = @"Camera Driver";
				else if (desc.componentType == 'sgpn')
					singletype = @"Camera Driver";
				else if (desc.componentType == 'spit')
					singletype = @"Exporter";
				else if (desc.componentType == 'eat ')
					singletype = @"Media Importer";
				else if (desc.componentType == 'imdc')
					singletype = @"Image Codec";
				else if (desc.componentType == 'imco')
					singletype = @"Image Codec";
				else if (desc.componentType == 'sdec')
					singletype = @"Sound Decoder";
				else if (desc.componentType == 'scom')
					singletype = @"Sound Compressor";
				else if (desc.componentType == 'aenc')
					singletype = @"Audio Encoder";
				else if (desc.componentType == 'adec')
					singletype = @"Audio Decoder";
				else if (desc.componentType == 'aunt')
					singletype = @"Audio Unit";
				else if (desc.componentType == 'grip')
					singletype = @"Graphics Importer";
				else if (desc.componentType == 'shlr')
					singletype = @"Sound Handler";
				else if (desc.componentType == 'dhlr')
					singletype = @"Data Handler";
				else if (desc.componentType == 'mhlr')
					singletype = @"Media Handler";
				else if (desc.componentType == 'tsvc')
					singletype = @"Text Service";
				else if (desc.componentType == 'osa ')
					singletype = @"Scripting Component";
				else if (desc.componentType == 'crnc')
					singletype = @"Cruncher";
				else if (desc.componentType == 'ihlr')
					singletype = @"Isochronous Handler";
				else if (desc.componentType == 'acca')
					singletype = @"Audio Context";
				else if (desc.componentType == 'sift')
					singletype = @"Sound Converter";
				else
					singletype = @"Unknown";

				if (firstType && singletype && ![firstType isEqualToString:singletype])
				{
					mixed = true;
				}

				if (![allTypes containsObject:singletype])
					[allTypes addObject:singletype];
				
	//			NSLog(@"%@", singletype);
				if (nameData && *nameData)
				{
					name = [[[NSString alloc] initWithBytes:(*nameData) + 1 length:**nameData encoding:NSUTF8StringEncoding] autorelease];
					if (!name)
						name = [[[NSString alloc] initWithBytes:(*nameData) + 1 length:**nameData encoding:NSUnicodeStringEncoding] autorelease];
				}	

				if (![allNames containsObject:name])
					[allNames addObject:name];
				
				if (GetHandleSize(data) >= (Size)sizeof(ExtComponentResource))
				{

					SET_KEY_BOOLVAL(@"Extended", YES);
					NSMutableArray* arches = [NSMutableArray array];
					ExtComponentResource* extthng = (ExtComponentResource*)*data;
					if (extthng)
					{
						SET_KEY(@"ComponentVersion", [NSNumber numberWithInt:extthng->componentVersion]);
						SET_KEY(@"RegisterFlags", [NSNumber numberWithInt:extthng->componentRegisterFlags]);
						int i= 0; 
						for (i = 0; i < extthng->count; i++)
						{
							ComponentPlatformInfo* info = &(extthng->platformArray[i]);
			
							NSMutableDictionary* pinfo = [NSMutableDictionary dictionary];
							
							[pinfo setObject:[NSNumber numberWithInt:info->platformType] forKey:@"Platform"];
							[pinfo setObject:[NSNumber numberWithInt:info->componentFlags] forKey:@"Flags"];

							[arches addObject:pinfo];
						}
						SET_KEY(@"Arches", arches);
					}
				}
			}

		}

		name = [GET_KEY(@"Location") lastPathComponent];
		if ([[name pathExtension] isEqualToString:@"component"])
			 name = [name stringByDeletingPathExtension];
		
		SET_KEY(@"Name", name);
		SET_KEY(@"Type", singletype);
		SET_KEY(@"Types", allTypes);
		SET_KEY(@"Names", allNames);
	
		[mCache setObject:mParameters forKey:filePath];
	}
	
	UseResFile(saveRes);

	if (isDirectory && bundleRef && resFile >= 0)
	{
		CFBundleCloseBundleResourceMap(bundleRef, resFile);
	}
	else if (!isDirectory && resFile >= 0)
	{
		CloseResFile(resFile);
	}
}



-(void)setComponentScope
{
	NSString* mLocation = GET_KEY(@"Location");

	if ([mLocation hasPrefix:@"/System/"])
		mWhere = componentIsSystem;
	else if ([mLocation hasPrefix:@"/Library/"])
		mWhere = componentIsGlobal;
	else if ([mLocation hasPrefix:@"/Users/"])
		mWhere = componentIsLocal;
}

-(void)dealloc
{
	if (mParameters)
		[mParameters release];

	[super dealloc];
}

-(NSString*)name
{
	return GET_KEY(@"Name");
}

-(NSString*)location
{
	return GET_KEY(@"Location");
}

-(NSString*)version
{
	if (GET_KEY(@"Version"))
		return GET_KEY(@"Version");
	else
		return GET_KEY(@"ComponentVersion");	
}

-(NSString*)type
{
	return GET_KEY(@"Type");
}

-(NSString*)types
{
	return [GET_KEY(@"Types") componentsJoinedByString:@", "];
}

-(NSString*)names
{
	return [GET_KEY(@"Names") componentsJoinedByString:@", "];
}

-(NSString*)whereSort
{
	if (mWhere == componentIsLocal)
		return @"Home";
	else if (mWhere == componentIsGlobal)
		return @"Library";
	else if (mWhere == componentIsSystem)
		return @"System";
	
	return @"";
}

-(NSString*)enabledSort
{
	return [NSString stringWithFormat:@"%d", GET_KEY_BOOLVAL(@"Enabled")];
}

-(componentScope)isWhere
{
	return mWhere;
}

-(BOOL)extended
{
	return GET_KEY_BOOLVAL(@"Extended");
}

-(NSNumber*)compVersion
{
	return GET_KEY(@"ComponentVersion");
}

-(NSNumber*)compFlags
{
	return GET_KEY(@"RegisterFlags");
}

-(NSString*)arches
{
	NSArray* arches = GET_KEY(@"Arches");
	NSMutableString* archesString = [NSMutableString string];
	int x= 0;
	int count = [arches count];
	for (x = 0; x < count; x++)
	{
		NSDictionary* pinfo = [arches objectAtIndex:x];
		int platform = [[pinfo objectForKey:@"Platform"] intValue];
		
		if (platform ==platformPowerPCNativeEntryPoint)
			[archesString appendFormat:@" PowerPC"];
		else if (platform ==platformIA32NativeEntryPoint)
			[archesString appendFormat:@" Intel"];
		else
			[archesString appendFormat:@" %d", x];
	}	
	return archesString;
}



-(NSImage*)where
{
	NSImage* image = 0;
	if (mWhere == componentIsLocal)
		image = [NSImage imageNamed:@"user"];
	else if (mWhere == componentIsGlobal)
		image = [NSImage imageNamed:@"global"];
	else if (mWhere == componentIsSystem)
		image = [NSImage imageNamed:@"system"];
		
	return image;
}

-(NSNumber*)componentEnabled
{
	return GET_KEY(@"Enabled");
}

-(void)setEnabled:(BOOL)enabled
{		
	NSString* mLocation = GET_KEY(@"Location");
	BOOL mEnabled = GET_KEY_BOOLVAL(@"Enabled");
	
	NSString* componentFile = [mLocation lastPathComponent];
	NSString* containingFolderPath = [mLocation stringByDeletingLastPathComponent];
	NSString* containingFolder = [containingFolderPath lastPathComponent];
	NSString* containingFolderFolderPath = [containingFolderPath stringByDeletingLastPathComponent];
	NSString* otherFolder = 0;
	if (mEnabled)
		otherFolder = [containingFolderFolderPath stringByAppendingPathComponent:[NSString stringWithFormat:@"%@%@", containingFolder, DISABLED_SUFFIX]];
	else
	{
		NSRange range = [containingFolder rangeOfString:DISABLED_SUFFIX];
		if (range.length > 0)
			otherFolder = [containingFolderFolderPath stringByAppendingPathComponent:[containingFolder substringToIndex:range.location]];
	}
	
	if (!otherFolder)
		return;
		
	NSFileManager* fm = [NSFileManager defaultManager];
	BOOL successful = YES;
	if (![fm fileExistsAtPath:otherFolder isDirectory:nil])
	{
		successful = [fm createDirectoryAtPath:otherFolder attributes:0];
	}
	NSString* destFile = [otherFolder stringByAppendingPathComponent:componentFile];
	if (successful)
		successful = [fm movePath:mLocation toPath:destFile handler:self];

	SET_KEY(@"Location", destFile);

	SET_KEY_BOOLVAL(@"Enabled", enabled);
}


- (BOOL)fileManager:(NSFileManager *)manager shouldProceedAfterError:(NSDictionary *)errorInfo
{
	NSLog(@"QTComponent Manager: Error: %@", errorInfo);
	return YES;
}

- (void)fileManager:(NSFileManager *)manager willProcessPath:(NSString *)path
{
}

@end
