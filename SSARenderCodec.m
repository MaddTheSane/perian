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

#import "SSARenderCodec.h"
#import "SSADocument.h"
#import "SSATagParsing.h"

typedef struct SSARenderGlobals
{
	SSADocument *document;
	Boolean plaintext;
} SSARenderGlobals;

SSARenderGlobalsPtr SSA_Init(const char *header, size_t size, float width, float height)
{
	SSARenderGlobalsPtr g = (SSARenderGlobalsPtr)NewPtr(sizeof(SSARenderGlobals));
	NSString *hdr = [[NSString alloc] initWithBytesNoCopy:(void*)header length:size encoding:NSUTF8StringEncoding freeWhenDone:NO];
	g->document = [[SSADocument alloc] init];
	g->plaintext = false;
	[g->document loadHeader:hdr width:width height:height];
	[hdr release];
	return g;
}

SSARenderGlobalsPtr SSA_InitNonSSA(float width, float height)
{
	SSARenderGlobalsPtr g = (SSARenderGlobalsPtr)NewPtr(sizeof(SSARenderGlobals));
	g->document = [[SSADocument alloc] init];
	g->plaintext = true;
	[g->document loadDefaultsWithWidth:width height:height];
	return g;
}

static void GetTypographicRectangleForLayout(SSARenderEntity *re, UniCharArrayOffset *breaks, ItemCount breakCount, Fixed *height, Fixed *width, unsigned baseX, unsigned baseY, float iheight)
{
	ATSTrapezoid trap = {0};
	ItemCount trapCount;
	FixedRect largeRect = {0};
	ATSUTextMeasurement ascent, descent;
	
	int i;
	for (i = breakCount; i >= 0; i--) {		
		UniCharArrayOffset end = breaks[i+1];
		FixedRect rect;
		OSStatus err;
		
		err=ATSUGetGlyphBounds(re->layout,baseX,FloatToFixed(iheight) - baseY,breaks[i],end-breaks[i],kATSUseDeviceOrigins,1,&trap,&trapCount);
		
		ATSUGetLineControl(re->layout, breaks[i], kATSULineAscentTag, sizeof(ATSUTextMeasurement), &ascent, NULL);
		ATSUGetLineControl(re->layout, breaks[i], kATSULineDescentTag, sizeof(ATSUTextMeasurement), &descent, NULL);
		
		baseY += ascent + descent;

		rect.bottom = MAX(trap.lowerLeft.y, trap.lowerRight.y);
		rect.left = MIN(trap.lowerLeft.x, trap.upperLeft.x);
		rect.top = MIN(trap.upperLeft.y, trap.upperRight.y);
		rect.right = MAX(trap.lowerRight.x, trap.upperRight.x);
		
		if (i == breakCount) largeRect = rect;
		
		largeRect.bottom = MAX(largeRect.bottom, rect.bottom);
		largeRect.left = MIN(largeRect.left, rect.left);
		largeRect.top = MIN(largeRect.top, rect.top);
		largeRect.right = MAX(largeRect.right, rect.right);
	}
	*height = largeRect.bottom - largeRect.top;
	*width = largeRect.right - largeRect.left;
}
/*
typedef struct CurveCallbackData {
	Float32Point origin, current;
	float		 windowHeight;
	CGContextRef c;
} CurveCallbackData;

static OSStatus SSA_CubicMoveTo(const Float32Point *p, void *callBackDataPtr)
{
	CurveCallbackData *cd = callBackDataPtr;
	float x, y;
	
	x = cd->origin.x + p->x;
	y = cd->windowHeight - (cd->origin.y + p->y);
	
	CGContextMoveToPoint(cd->c,x,y);
	
	cd->current.x = x;
	cd->current.y = y;
	return noErr;
}

static OSStatus SSA_CubicLineTo(const Float32Point *p, void *callBackDataPtr)
{
	CurveCallbackData *cd = callBackDataPtr;
	float x, y;
	
	x = cd->origin.x + p->x;
	y = cd->windowHeight - (cd->origin.y + p->y);
	
	if (x == cd->current.x && y == cd->current.y) return noErr;
	
	CGContextAddLineToPoint(cd->c,x,y);
	
	cd->current.x = x;
	cd->current.y = y;
	return noErr;	
}

static OSStatus SSA_CubicCurveTo(const Float32Point *p1, const Float32Point *p2, const Float32Point *p3, void *callBackDataPtr)
{
	CurveCallbackData *cd = callBackDataPtr;
	float x[3] = {p1->x,p2->x,p3->x};
	float y[3] = {p1->y,p2->y,p3->y};
	int i;
	for (i = 0; i < 3; i++) x[i] += cd->origin.x;
	for (i = 0; i < 3; i++) y[i] = cd->windowHeight - (cd->origin.y + y[i]);
	
	CGContextAddCurveToPoint(cd->c,x[0],y[0],x[1],y[1],x[2],y[2]);
	
	cd->current.x = x[2];
	cd->current.y = y[2];
	return noErr;
}
*/

typedef enum DrawingMode {shadowtext, foreground} DrawingMode;

static void DrawOneStyleSpan(SSARenderEntity *re, SSAStyleSpan *span, CGContextRef c, UniCharArrayOffset linepos, UniCharCount lineclength, ATSUTextMeasurement baseX, ATSUTextMeasurement baseY, DrawingMode mode)
{
	Fixed lineRot = FloatToFixed(span->angle - re->style->angle);
	if (span->shadow > 0 && span->outline == 0) span->outline = 1;
	
	CGContextSetLineWidth(c,span->outline * 2.);

	if (re->multipart_drawing) SetATSULayoutOther(re->layout,kATSULineRotationTag,sizeof(Fixed),&lineRot);
		
	if (mode == shadowtext) {
		Fixed shadOffset = FloatToFixed(span->shadow);
		if (span->shadow == 0) return;
		
		CGContextSetRGBFillColor(c,span->color.shadow.red,span->color.shadow.green,span->color.shadow.blue,span->color.shadow.alpha);
		CGContextSetRGBStrokeColor(c,span->color.shadow.red,span->color.shadow.green,span->color.shadow.blue,span->color.shadow.alpha);
		CGContextSetTextDrawingMode(c, kCGTextFillStroke);	

		ATSUDrawText(re->layout,linepos,lineclength,baseX + shadOffset,baseY - shadOffset);
	} else {
		CGContextSetRGBFillColor(c,span->color.primary.red,span->color.primary.green,span->color.primary.blue,span->color.primary.alpha);
		CGContextSetRGBStrokeColor(c,span->color.outline.red,span->color.outline.green,span->color.outline.blue,span->color.outline.alpha);

		if (span->outline > 0) {
			CGContextSetTextDrawingMode(c,kCGTextStroke);
			ATSUDrawText(re->layout,linepos,lineclength,baseX,baseY);
			CGContextSetTextDrawingMode(c,kCGTextFill);
		}
		
		ATSUDrawText(re->layout,linepos,lineclength,baseX,baseY);
	}
	
}

static void SSA_DrawTextLine(SSARenderEntity *re, UniCharArrayOffset linepos, UniCharCount lineclength, ATSUTextMeasurement baseX, ATSUTextMeasurement baseY, DrawingMode mode, CGContextRef c)
{
	int i;
	//ATSCubicMoveToUPP cmoveP;
	//ATSCubicLineToUPP clineP;
	//ATSCubicCurveToUPP ccurveP;
	
	if (!re->multipart_drawing) {
		DrawOneStyleSpan(re,re->styles[0],c,linepos,lineclength,baseX,baseY,mode);
	} else {
		for (i = 0; i < re->style_count; i++) {
			SSAStyleSpan *span = re->styles[i];
			UniCharArrayOffset spanst, spanl;
			
			if ((span->range.location + span->range.length) < linepos) continue;
			if (span->range.location >= (linepos + lineclength)) break;
			
			spanl = span->range.length;
			
			if (span->range.location < linepos) {
				spanst = linepos;
				spanl -= linepos - span->range.location;
			} else spanst = span->range.location;
			
			DrawOneStyleSpan(re,re->styles[i],c,spanst,spanl,baseX,baseY,mode);
		}
	}
}

static Fixed GetLineHeight(ATSUTextLayout layout, UniCharArrayOffset lpos)
{
	ATSUTextMeasurement ascent, descent;
	
	ATSUGetLineControl(layout, lpos, kATSULineAscentTag,  sizeof(ATSUTextMeasurement), &ascent,  NULL);
	ATSUGetLineControl(layout, lpos, kATSULineDescentTag, sizeof(ATSUTextMeasurement), &descent, NULL);
	
	return ascent + descent;
}

static Fixed SSA_DrawLineArray(SSARenderEntity *re, int lstart, int lend, int lstep, char direction, UniCharArrayOffset *breaks, Fixed penX, Fixed penY, DrawingMode mode, CGContextRef c)
{
	int i;
	
	for (i = lstart; i != lend; i += lstep) {		
		SSA_DrawTextLine(re, breaks[i], breaks[i+1] - breaks[i], penX, penY, mode, c);
		
		penY += direction * GetLineHeight(re->layout, breaks[i]);
	}
	
	return penY;
}
							   
void SSA_RenderLine(SSARenderGlobalsPtr glob, CGContextRef c, CFStringRef cfSub, float cWidth, float cHeight)
{
	ItemCount breakCount;
	Fixed penY=0,penX;
	Fixed lastTopPenY=-1, lastBottomPenY=-1, lastCenterPenY=-1, *storePenY, ignoredPenY;
	int i, lstart, lend, lstep, subcount, j; char direction;
	if (!(glob && glob->document)) return;
	SSADocument *ssa = glob->document;
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *curSub = (NSString*)cfSub;
	NSArray *rentities = ParseSubPacket(curSub,ssa,glob->plaintext);
	SSARenderEntity *last_re = nil;
	OSStatus err;
	
	CGContextClearRect(c, CGRectMake(0,0,cWidth,cHeight));
	CGContextScaleCTM(c, cWidth / ssa->resX, cHeight / ssa->resY);
	subcount = [rentities count];
	
	for (j = 0; j < subcount; j++) {
		SSARenderEntity *re = (SSARenderEntity*)[rentities objectAtIndex:j];
		if (re->is_shape) continue;
		ATSUTextLayout layout = re->layout;
		BOOL dirty_layout = NO;
		
		if (last_re && re->marginv != last_re->marginv) {lastTopPenY = lastBottomPenY = lastCenterPenY = -1;}
					
		size_t sublen = [re->nstext length];
						
		err=ATSUSetTextPointerLocation(layout,re->text,kATSUFromTextBeginning,kATSUToTextEnd,sublen);
		
		ATSUSetTransientFontMatching(layout,TRUE);
		
		for (i = 0; i < re->style_count; i++) ATSUSetRunStyle(layout,re->styles[i]->astyle,re->styles[i]->range.location,re->styles[i]->range.length);
		
		SetATSULayoutOther(layout,kATSUCGContextTag,sizeof(CGContextRef),&c);
		if (!re->multipart_drawing) {
			Fixed lineRot = FloatToFixed(re->styles[0]->angle);
			SetATSULayoutOther(re->layout,kATSULineRotationTag,sizeof(Fixed),&lineRot);
		}
		
		ATSUBatchBreakLines(layout,kATSUFromTextBeginning,kATSUToTextEnd,IntToFixed(re->usablewidth),&breakCount); 
		ATSUGetSoftLineBreaks(layout,kATSUFromTextBeginning,kATSUToTextEnd,0,NULL,&breakCount);
		UniCharArrayOffset breaks[breakCount+2];
		ATSUGetSoftLineBreaks(layout,kATSUFromTextBeginning,kATSUToTextEnd,breakCount,&breaks[1],&breakCount);
		
		breaks[0] = 0;
		breaks[breakCount+1] = sublen;
		
		penX = IntToFixed(re->marginl);
		
		if (re->posx == -1) {
			ATSUTextMeasurement descent, total = 0;
			
			switch (re->valign)
			{
				case S_BottomAlign: default: //bottom
					ATSUGetLineControl(layout, kATSUFromTextBeginning, kATSULineDescentTag, sizeof(ATSUTextMeasurement), &descent, NULL);
					penY = (lastBottomPenY!=-1)?lastBottomPenY:(MAX(IntToFixed(re->marginv), descent) + FloatToFixed(shadow)); direction = 1;
					lstart = breakCount; lend = -1; lstep = -1;
					storePenY = &lastBottomPenY;
					break;
				case S_MiddleAlign: // center
					penY = (lastCenterPenY!=-1)?lastCenterPenY:(FloatToFixed((ssa->resY - FixedToFloat(total)) / 2.));  direction = -1;
					lstart = 0; lend = breakCount+1; lstep = 1;
					storePenY = &lastCenterPenY;
					break;
				case S_TopAlign: //top					
					penY = (lastTopPenY!=-1)?lastTopPenY:(FloatToFixed(ssa->resY - re->marginv) - GetLineHeight(layout, kATSUFromTextBeginning)); direction = -1;
					lstart = 0; lend = breakCount+1; lstep = 1;
					storePenY = &lastTopPenY;
					break;
			}
		}
		else {
			Fixed imageHeight, imageWidth;
			
			GetTypographicRectangleForLayout(re,breaks,breakCount,&imageHeight,&imageWidth,penX,penY,ssa->resY);
			
			penX = IntToFixed(re->posx);
			penY = FloatToFixed((ssa->resY - re->posy));
//			NSLog(@"pos (%d,%d) w %d (image: %f, %f)",re->posx,re->posy,re->usablewidth,FixedToFloat(imageWidth),FixedToFloat(imageHeight));			
			switch(re->halign) {
				case S_CenterAlign:
					penX -= imageWidth / 2;
					break;
				case S_RightAlign:
					penX -= imageWidth;
			}
			
			switch(re->valign) {
				case S_MiddleAlign:
					penY -= imageHeight / 2;
					break;
				case S_RightAlign:
					penY -= imageHeight;
			}
			
			SetATSULayoutOther(layout,kATSULineWidthTag,sizeof(Fixed),&imageWidth);
			dirty_layout = YES;
			
			direction = 1;
			lstart = breakCount; lend = -1; lstep = -1;
			storePenY = &ignoredPenY;
		}
		
		CGContextSetLineJoin(c, kCGLineJoinRound);
		CGContextSetLineCap(c, kCGLineCapRound);
				
		SSA_DrawLineArray(re, lstart, lend, lstep, direction, breaks, penX, penY, shadowtext, c);
		
		penY = SSA_DrawLineArray(re, lstart, lend, lstep, direction, breaks, penX, penY, foreground, c);
		
		if (dirty_layout) {
			Fixed fwidth = IntToFixed(re->usablewidth);
			SetATSULayoutOther(layout,kATSULineWidthTag,sizeof(Fixed),&fwidth);
		}
		
		*storePenY = penY;
		
		last_re = re;
	}
	
	[pool release];
}

void SSA_Dispose(SSARenderGlobalsPtr glob)
{
	[glob->document release];
	DisposePtr((Ptr)glob);
}