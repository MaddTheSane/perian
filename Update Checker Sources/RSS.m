/*

BSD License

Copyright (c) 2002, Brent Simmons
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

*	Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.
*	Redistributions in binary form must reproduce the above copyright notice,
	this list of conditions and the following disclaimer in the documentation
	and/or other materials provided with the distribution.
*	Neither the name of ranchero.com or Brent Simmons nor the names of its
	contributors may be used to endorse or promote products derived
	from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


*/

/*
	RSS.m
	A class for reading RSS feeds.

	Created by Brent Simmons on Wed Apr 17 2002.
	Copyright (c) 2002 Brent Simmons. All rights reserved.
*/


#import "RSS.h"

@interface RSS ()
/*Private*/
//TODO: Use NSXML functions, the CoreFoundation XMLs are deprecated as of 10.8
- (void) createheaderdictionary: (CFXMLTreeRef) tree;
- (void) createitemsarray: (CFXMLTreeRef) tree;
- (void) setversionstring: (CFXMLTreeRef) tree;
- (void) flattenimagechildren: (CFXMLTreeRef) tree into: (NSMutableDictionary *) dictionary;
- (void) flattensourceattributes: (CFXMLNodeRef) node into: (NSMutableDictionary *) dictionary;
- (CFXMLTreeRef) getchanneltree: (CFXMLTreeRef) tree;
- (CFXMLTreeRef) getnamedtree: (CFXMLTreeRef) currentTree name: (NSString *) name;
- (void) normalizeRSSItem: (NSMutableDictionary *) rssItem;
- (NSString *) getelementvalue: (CFXMLTreeRef) tree;

@property (readwrite, copy) NSDictionary *headerItems;
@property (readwrite, strong) NSMutableArray *newsItems;
@property (readwrite, copy) NSString *version;

@end

@implementation RSS

@synthesize headerItems;
@synthesize newsItems;
@synthesize version;

#define titleKey @"title"
#define linkKey @"link"
#define descriptionKey @"description"


/*Public interface*/

- (NSDictionary *)newestItem
{
	// The news items are already sorted by published date, descending.
	return (self.newsItems)[0];
}

- (id) initWithTitle: (NSString *) title andDescription: (NSString *) description
{
	
	/*
	 Create an empty feed. Useful for synthetic feeds.
	 */
	
	if (self = [super init]) {
		NSMutableDictionary *header;
		flRdf = NO;
		header = [NSMutableDictionary dictionaryWithCapacity: 2];
		header[titleKey] = title;
		header[descriptionKey] = description;
		self.headerItems = header;
		newsItems = [[NSMutableArray alloc] initWithCapacity: 0];
		self.version = @"synthetic";
		
	}
	return self;
} /*initWithTitle*/

- (id) initWithData: (NSData *) rssData normalize: (BOOL) fl
{
	
	if (self = [super init]) {
		CFXMLTreeRef tree;
		flRdf = NO;
		normalize = fl;
		NS_DURING
		tree = CFXMLTreeCreateFromData (kCFAllocatorDefault, (__bridge CFDataRef)rssData,
										NULL,  kCFXMLParserSkipWhitespace, kCFXMLNodeCurrentVersion);
		NS_HANDLER
		tree = nil;
		NS_ENDHANDLER
		if (tree == nil) {
			/*If there was a problem parsing the RSS file,
			 raise an exception.*/
			
			NSException *exception = [NSException exceptionWithName: @"RSSParseFailed"
															 reason: @"The XML parser could not parse the RSS data." userInfo: nil];
			
			[exception raise];
		} /*if*/
		
		[self createheaderdictionary: tree];
		[self createitemsarray: tree];
		[self setversionstring: tree];
		CFRelease (tree);
	}
	return self;
} /*initWithData*/

- (id) initWithURL: (NSURL *) url normalize: (BOOL) fl
{
	return [self initWithURL: url normalize: fl userAgent: nil];
}

- (id) initWithURL: (NSURL *) url normalize: (BOOL) fl userAgent: (NSString*)userAgent
{
	NSData *rssData;
	
	NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL: url cachePolicy: NSURLRequestReloadIgnoringCacheData
													   timeoutInterval: 30.0];
	if (userAgent)
		[request setValue: userAgent forHTTPHeaderField: @"User-Agent"];
	
	NSURLResponse *response=0;
	NSError *error=0;
	rssData = [NSURLConnection sendSynchronousRequest: request returningResponse: &response error: &error];
	
	if (rssData == nil)
	{
		NSException *exception = [NSException exceptionWithName: @"RSSDownloadFailed"
														 reason: [error localizedFailureReason] userInfo: [error userInfo] ];
		[exception raise];
	}
	
	return [self initWithData: rssData normalize: fl];
} /*initWithUrl*/


#if !__has_feature(objc_arc)
- (void) dealloc
{	
	[headerItems release];
	[newsItems release];
	[version release];
	
	[super dealloc];
} /*dealloc*/
#endif


/*Private methods. Don't call these: they may change.*/

- (void) createheaderdictionary: (CFXMLTreeRef) tree {
	
	CFXMLTreeRef channelTree, childTree;
	CFXMLNodeRef childNode;
	int childCount, i;
	NSString *childName;
	NSMutableDictionary *headerItemsMutable;
	
	channelTree = [self getchanneltree: tree];
	
	if (channelTree == nil) {
		
		NSException *exception = [NSException exceptionWithName: @"RSSCreateHeaderDictionaryFailed"
														 reason: @"Couldn't find the channel tree." userInfo: nil];
		
		[exception raise];
	} /*if*/
	
	childCount = CFTreeGetChildCount (channelTree);
	
	headerItemsMutable = [NSMutableDictionary dictionaryWithCapacity: childCount];
	
	for (i = 0; i < childCount; i++) {
		childTree = CFTreeGetChildAtIndex (channelTree, i);
		childNode = CFXMLTreeGetNode (childTree);
		childName = (__bridge NSString*)(CFXMLNodeGetString(childNode));
		
		if ([childName hasPrefix: @"rss:"])
			childName = [childName substringFromIndex: 4];
		
		if ([childName isEqualToString: @"item"])
			break;
		
		if ([childName isEqualTo: @"image"])
			[self flattenimagechildren: childTree into: headerItemsMutable];
		
		headerItemsMutable[childName] = [self getelementvalue: childTree];
	} /*for*/
	
	headerItems = [headerItemsMutable copy];
} /*initheaderdictionary*/


- (void) createitemsarray: (CFXMLTreeRef) tree {
	
	CFXMLTreeRef channelTree, childTree, itemTree;
	CFXMLNodeRef childNode, itemNode;
	NSString *childName;
	NSString *itemName, *itemValue;
	CFIndex childCount, itemChildCount, i, j;
	NSMutableDictionary *itemDictionaryMutable;
	NSMutableArray *itemsArrayMutable;
	
	if (flRdf)
		channelTree = [self getnamedtree: tree name: @"rdf:RDF"];
	else
		channelTree = [self getchanneltree: tree];
	
	if (channelTree == nil) {
		NSException *exception = [NSException exceptionWithName: @"RSSCreateItemsArrayFailed"
														 reason: @"Couldn't find the news items." userInfo: nil];
		
		[exception raise];
	} /*if*/
	
	childCount = CFTreeGetChildCount (channelTree);
	itemsArrayMutable = [NSMutableArray arrayWithCapacity: childCount];
	
	for (i = 0; i < childCount; i++) {
		
		childTree = CFTreeGetChildAtIndex (channelTree, i);
		childNode = CFXMLTreeGetNode (childTree);
		childName = (__bridge NSString*)(CFXMLNodeGetString(childNode));
		
		if ([childName hasPrefix: @"rss:"])
			childName = [childName substringFromIndex: 4];
		
		if (![childName isEqualToString: @"item"])
			continue;
		
		itemChildCount = CFTreeGetChildCount (childTree);
		itemDictionaryMutable = [NSMutableDictionary dictionaryWithCapacity: itemChildCount];
		
		for (j = 0; j < itemChildCount; j++) {
			
			itemTree = CFTreeGetChildAtIndex (childTree, j);
			itemNode = CFXMLTreeGetNode (itemTree);
			itemName = (__bridge NSString*)(CFXMLNodeGetString(itemNode));
			
			if ([itemName hasPrefix: @"rss:"])
				itemName = [itemName substringFromIndex: 4];
			
			if ([itemName isEqualTo:@"enclosure"])
			{
				// Hack to add attributes to the dictionary in addition to children. (AMM)
				const CFXMLElementInfo *websiteInfo = CFXMLNodeGetInfoPtr(itemNode);
				NSMutableDictionary *enclosureDictionary = [NSMutableDictionary dictionary];
				NSDictionary *tmpAttrs = (__bridge NSDictionary *)(websiteInfo->attributes);
				for (NSString *current in [tmpAttrs keyEnumerator]) {
					enclosureDictionary[current] = tmpAttrs[current];
					
				}
				itemDictionaryMutable[itemName] = enclosureDictionary;
				continue;
			}
			
			itemValue = [self getelementvalue: itemTree];
			
			if ([itemName isEqualTo: @"source"])
				[self flattensourceattributes: itemNode into: itemDictionaryMutable];
			
			itemDictionaryMutable[itemName] = itemValue;
		} /*for*/
		
		if (normalize)
			[self normalizeRSSItem: itemDictionaryMutable];
		
		[itemsArrayMutable addObject: itemDictionaryMutable];
	} /*for*/
	
	// Sort the news items by published date, descending.
	// This comparator function is used to sort the RSS items by their published date.
	newsItems = [[NSMutableArray alloc] initWithArray:[itemsArrayMutable sortedArrayWithOptions:NSSortConcurrent usingComparator:^NSComparisonResult(id item1, id item2) {
		// We compare item2 with item1 instead of the other way 'round because we want descending, not ascending. Bit of a hack.
		return [(NSDate *)[NSDate dateWithNaturalLanguageString:item2[@"pubDate"]] compare:(NSDate *)[NSDate dateWithNaturalLanguageString:item1[@"pubDate"]]];
		
	}]];
} /*createitemsarray*/


- (void) setversionstring: (CFXMLTreeRef) tree {
	
	CFXMLTreeRef rssTree;
	const CFXMLElementInfo *elementInfo;
	CFXMLNodeRef node;
	
	if (flRdf) {
		self.version = @"rdf";
		return;
	} /*if*/
	
	rssTree = [self getnamedtree: tree name: @"rss"];
	
	node = CFXMLTreeGetNode(rssTree);
	
	elementInfo = CFXMLNodeGetInfoPtr(node);
	
	version = [[NSString alloc] initWithString: ((__bridge NSDictionary *)(elementInfo->attributes))[@"version"]];
} /*setversionstring*/


- (void) flattenimagechildren: (CFXMLTreeRef) tree into: (NSMutableDictionary *) dictionary {
	
	int childCount = CFTreeGetChildCount (tree);
	int i = 0;
	CFXMLTreeRef childTree;
	CFXMLNodeRef childNode;
	NSString *childName, *childValue, *keyName;
	
	if (childCount < 1)
		return;
	
	for (i = 0; i < childCount; i++) {
		childTree = CFTreeGetChildAtIndex (tree, i);
		childNode = CFXMLTreeGetNode (childTree);
		childName = (__bridge NSString*)(CFXMLNodeGetString(childNode));
		
		if ([childName hasPrefix: @"rss:"])
			childName = [childName substringFromIndex: 4];
		
		childValue = [self getelementvalue: childTree];
		keyName = [NSString stringWithFormat: @"image%@", childName];
		dictionary[keyName] = childValue;
	} /*for*/
} /*flattenimagechildren*/


- (void) flattensourceattributes: (CFXMLNodeRef) node into: (NSMutableDictionary *) dictionary {
	
	const CFXMLElementInfo *elementInfo;
	NSString *sourceHomeUrl, *sourceRssUrl;
	
	elementInfo = CFXMLNodeGetInfoPtr(node);
	
	NSDictionary *tmpAttrs = (__bridge NSDictionary*)(elementInfo->attributes);
	sourceHomeUrl = tmpAttrs[@"homeUrl"];
	sourceRssUrl = tmpAttrs[@"url"];
	
	if (sourceHomeUrl != nil)
		dictionary[@"sourceHomeUrl"] = sourceHomeUrl;
	
	if (sourceRssUrl != nil)
		dictionary[@"sourceRssUrl"] = sourceRssUrl;
} /*flattensourceattributes*/


- (CFXMLTreeRef) getchanneltree: (CFXMLTreeRef) tree {
	
	CFXMLTreeRef rssTree, channelTree;
	
	rssTree = [self getnamedtree: tree name: @"rss"];
	
	if (rssTree == nil) { /*It might be "rdf:RDF" instead, a 1.0 or greater feed.*/
		
		rssTree = [self getnamedtree: tree name: @"rdf:RDF"];
		
		if (rssTree != nil)
			flRdf = YES; /*This info will be needed later when creating the items array.*/
	} /*if*/
	
	if (rssTree == nil)
		return (nil);
	
	channelTree = [self getnamedtree: rssTree name: @"channel"];
	
	if (channelTree == nil)
		channelTree = [self getnamedtree: rssTree name: @"rss:channel"];
	
	return (channelTree);
} /*getchanneltree*/


- (CFXMLTreeRef)getnamedtree:(CFXMLTreeRef)currentTree name:(NSString *)name
{
	NSInteger childCount, i;
	CFXMLNodeRef xmlNode;
	CFXMLTreeRef xmlTreeNode;
	NSString *itemName;
	
	childCount = CFTreeGetChildCount(currentTree);
	
	for (i = childCount - 1; i >= 0; i--) {
		
		xmlTreeNode = CFTreeGetChildAtIndex (currentTree, i);
		xmlNode = CFXMLTreeGetNode (xmlTreeNode);
		itemName = (__bridge NSString*)(CFXMLNodeGetString(xmlNode));
		
		if ([itemName isEqualToString: name])
			return xmlTreeNode;
	} /*for*/
	
	return nil;
} /*getnamedtree*/


- (void)normalizeRSSItem:(NSMutableDictionary *)rssItem
{
	
	/*
	 Make sure item, link, and description are present and have
	 reasonable values. Description and link may be "".
	 Also trim white space, remove HTML when appropriate.
	 */
	
	NSString *description, *link, *title;
	BOOL nilDescription = NO;
	
	/*Description*/
	
	description = rssItem[descriptionKey];
	
	if (description == nil) {
		description = @"";
		nilDescription = YES;
	} /*if*/
	
	description = [description trimWhiteSpace];
	
	if ([description isEqualTo: @""])
		nilDescription = YES;
	
	rssItem[descriptionKey] = description;
	
	/*Link*/
	
	link = rssItem[linkKey];
	
	if ([NSString stringIsEmpty: link]) {
		
		/*Try to get a URL from the description.*/
		
		if (!nilDescription) {
			
			NSArray *stringComponents = [description componentsSeparatedByString: @"href=\""];
			
			if ([stringComponents count] > 1) {
				link = stringComponents[1];
				stringComponents = [link componentsSeparatedByString: @"\""];
				link = stringComponents[0];
			} /*if*/
		} /*if*/
	} /*if*/
	
	if (link == nil)
		link = @"";
	
	link = [link trimWhiteSpace];
	
	rssItem[linkKey] = link;
	
	/*Title*/
	
	title = rssItem[titleKey];
	
	if (title != nil) {
		
		title = [title stripHTML];
		
		title = [title trimWhiteSpace];
	} /*if*/
	
	if ([NSString stringIsEmpty: title]) {
		
		/*Grab a title from the description.*/
		
		if (!nilDescription) {
			
			NSArray *stringComponents = [description componentsSeparatedByString: @">"];
			
			if ([stringComponents count] > 1) {
				
				title = stringComponents[1];
				
				stringComponents = [title componentsSeparatedByString: @"<"];
				
				title = stringComponents[0];
				
				title = [title stripHTML];
				
				title = [title trimWhiteSpace];
			} /*if*/
			
			if ([NSString stringIsEmpty: title]) { /*use first part of description*/
				NSString *shortTitle = [[[description stripHTML] trimWhiteSpace] ellipsizeAfterNWords: 5];
				shortTitle = [shortTitle trimWhiteSpace];
				title = [NSString stringWithFormat: @"%@...", shortTitle];
			} /*else*/
		} /*if*/
		
		title = [title stripHTML];
		
		title = [title trimWhiteSpace];
		
		if ([NSString stringIsEmpty: title])
			title = @"Untitled";
	} /*if*/
	
	rssItem[titleKey] = title;
	
	/*dangerousmeta case: super-long title with no description*/
	
	if ((nilDescription) && ([title length] > 50)) {
		
		NSString *shortTitle = [[[title stripHTML] trimWhiteSpace] ellipsizeAfterNWords: 7];
		description = [NSString stringWithString:title];
		rssItem[descriptionKey] = description;
		title = [NSString stringWithFormat: @"%@...", shortTitle];
		rssItem[titleKey] = title;
	} /*if*/
	
	{ /*deal with entities*/
		
		const char *tempcstring;
		NSAttributedString *s = nil;
		NSString *convertedTitle = nil;
		NSArray *stringComponents;
		
		stringComponents = [title componentsSeparatedByString: @"&"];
		
		if ([stringComponents count] > 1) {
			
			stringComponents = [title componentsSeparatedByString: @";"];
			
			if ([stringComponents count] > 1) {
				
				int len;
				
				tempcstring = [title UTF8String];
				
				len = strlen (tempcstring);
				
				if (len > 0) {
					
					s = [[NSAttributedString alloc]
						 initWithHTML: [NSData dataWithBytes: tempcstring length: strlen (tempcstring)]
						 documentAttributes: NULL];
					
					convertedTitle = [s string];
					
					convertedTitle = [convertedTitle stripHTML];
					convertedTitle = [convertedTitle trimWhiteSpace];
				} /*if*/
				
				if ([NSString stringIsEmpty: convertedTitle])
					convertedTitle = @"Untitled";
				
				rssItem[@"convertedTitle"] = convertedTitle;
			} /*if*/
		} /*if*/
	} /*deal with entities*/
} /*normalizeRSSItem*/


- (NSString *) getelementvalue: (CFXMLTreeRef) tree
{
	CFXMLNodeRef node;
	CFXMLTreeRef itemTree;
	int childCount, ix;
	NSMutableString *valueMutable;
	NSString *name;
	
	childCount = CFTreeGetChildCount (tree);
	valueMutable = [[NSMutableString alloc] init];
	
	for (ix = 0; ix < childCount; ix++) {
		itemTree = CFTreeGetChildAtIndex(tree, ix);
		node = CFXMLTreeGetNode(itemTree);
		name = (__bridge NSString*)(CFXMLNodeGetString(node));
		
		if (name != nil) {
			
			if (CFXMLNodeGetTypeCode(node) == kCFXMLNodeTypeEntityReference) {
				
				if ([name isEqualTo: @"lt"])
					name = @"<";
				
				else if ([name isEqualTo: @"gt"])
					name = @">";
				
				else if ([name isEqualTo: @"quot"])
					name = @"\"";
				
				else if ([name isEqualTo: @"amp"])
					name = @"&";
				
				else if ([name isEqualTo: @"rsquo"])
					name = @"\"";
				
				else if ([name isEqualTo: @"lsquo"])
					name = @"\"";
				
				else if ([name isEqualTo: @"apos"])
					name = @"'";
				else
					name = [NSString stringWithFormat: @"&%@;", name];
			} /*if*/
			
			[valueMutable appendString: name];
		} /*if*/
	} /*for*/
	
	return [[NSString alloc] initWithString:valueMutable];
} /*getelementvalue*/

@end
