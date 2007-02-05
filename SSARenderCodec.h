/*
 *  SSARenderCodec.m
 *  Copyright (c) 2007 Perian Project
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; 
 *  version 2.1 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#import <Carbon/Carbon.h>

struct SSARenderGlobals;

typedef struct SSARenderGlobals *SSARenderGlobalsPtr;

extern SSARenderGlobalsPtr SSA_Init(const char *header, size_t size, float width, float height);
extern SSARenderGlobalsPtr SSA_InitNonSSA(float width, float height);
extern void SSA_RenderLine(SSARenderGlobalsPtr glob, CGContextRef c, CFStringRef str, float width, float height);
extern void SSA_Dispose(SSARenderGlobalsPtr glob);
