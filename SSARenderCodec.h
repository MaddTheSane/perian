//
//  SSARenderCodec.h
//  Perian
//
//  Created by Alexander Strange on 1/28/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Carbon/Carbon.h>

struct SSARenderGlobals;

typedef struct SSARenderGlobals *SSARenderGlobalsPtr;

extern SSARenderGlobalsPtr SSA_Init(const char *header, size_t size);
extern void SSA_RenderLine(SSARenderGlobalsPtr glob, CGContextRef c, const char *data, size_t size, float width, float height);
extern void SSA_Dispose(SSARenderGlobalsPtr glob);