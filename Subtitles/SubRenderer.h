/*
 *  SubRender.h
 *  SSARender2
 *
 *  Created by Alexander Strange on 7/28/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#import <Cocoa/Cocoa.h>

@class SubStyle, SubContext, SubRenderDiv, SubRenderSpan;

typedef enum {tag_b=0, tag_i, tag_u, tag_s, tag_bord, tag_shad, tag_be, tag_fn, tag_fs, tag_fscx, tag_fscy, tag_fsp, tag_frx, tag_fry, tag_frz, tag_1c, tag_2c, tag_3c, tag_4c, 
	  tag_1a, tag_2a, tag_3a, tag_4a, tag_r} SSATagType;

@interface SubRenderer : NSObject
-(void)completedHeaderParsing:(SubContext*)sc;
-(void*)completedStyleParsing:(SubStyle*)s;
-(void)releaseStyleExtra:(void*)ex;
-(void*)spanExtraFromRenderDiv:(SubRenderDiv*)div;
-(void*)cloneSpanExtra:(SubRenderSpan*)span;
-(void)releaseSpanExtra:(void*)ex;
-(void)spanChangedTag:(SSATagType)tag span:(SubRenderSpan*)span div:(SubRenderDiv*)div param:(void*)p;
-(float)aspectRatio;
@end