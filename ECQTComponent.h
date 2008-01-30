//
//  ECQTComponent.h
//  QTManager
//
//  Created by Ken on 12/15/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

typedef enum _componentScope
{
	componentIsLocal = 0,
	componentIsGlobal = 1,
	componentIsSystem = 2
	
} componentScope;


@interface ECQTComponent : NSObject 
{
	componentScope mWhere;

	NSMutableDictionary* mParameters;
}

+(id)componentWithPath:(NSString*)path;

-(void)setComponentInfo;
-(void)setComponentScope;

-(componentScope)isWhere;

-(BOOL)extended;
-(void)setEnabled:(BOOL)enabled;

-(NSString*)name;
-(NSString*)location;
-(NSString*)version;
-(NSString*)type;
-(NSNumber*)compVersion;
-(NSNumber*)compFlags;
-(NSString*)arches;
-(NSString*)whereSort;
-(NSNumber*)componentEnabled;

-(NSString*)types;
-(NSString*)names;


@end
