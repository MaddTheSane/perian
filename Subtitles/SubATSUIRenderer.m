//
//  SubATSUIRenderer.m
//  SSARender2
//
//  Created by Alexander Strange on 7/30/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "SubATSUIRenderer.h"
#import "SubImport.h"
#import "SubParsing.h"
#import "SubUtilities.h"

static float GetWinFontSizeScale(ATSFontRef font);

@interface SubATSUISpanEx : NSObject {
	@public;
	ATSUStyle style;
	CGColorRef primaryColor, outlineColor, shadowColor;
	Float32 outlineRadius, shadowDist, scaleX, scaleY, primaryAlpha, outlineAlpha, angle;
}
-(SubATSUISpanEx*)initWithStyle:(ATSUStyle)style_ subStyle:(SubStyle*)sstyle colorSpace:(CGColorSpaceRef)cs;
-(SubATSUISpanEx*)clone;
@end

@implementation SubATSUISpanEx
-(void)dealloc
{
	ATSUDisposeStyle(style);
	CGColorRelease(primaryColor);
	CGColorRelease(outlineColor);
	CGColorRelease(shadowColor);
	[super dealloc];
}

static CGColorRef MakeCGColorFromRGBA(SubRGBAColor c, CGColorSpaceRef cspace)
{
	const float components[] = {c.red, c.green, c.blue, c.alpha};
	
	return CGColorCreate(cspace, components);
}

static CGColorRef MakeCGColorFromRGBOpaque(SubRGBAColor c, CGColorSpaceRef cspace)
{
	SubRGBAColor c2 = c;
	c2.alpha = 1;
	return MakeCGColorFromRGBA(c2, cspace);
}

static CGColorRef CloneCGColorWithAlpha(CGColorRef c, float alpha)
{
	CGColorRef new = CGColorCreateCopyWithAlpha(c, alpha);
	CGColorRelease(c);
	return new;
}

-(SubATSUISpanEx*)initWithStyle:(ATSUStyle)style_ subStyle:(SubStyle*)sstyle colorSpace:(CGColorSpaceRef)cs
{
	if (self = [super init]) {
		ATSUCreateAndCopyStyle(style_, &style);
		
		primaryColor = MakeCGColorFromRGBOpaque(sstyle->primaryColor, cs);
		primaryAlpha = sstyle->primaryColor.alpha;
		outlineColor = MakeCGColorFromRGBOpaque(sstyle->outlineColor, cs);
		outlineAlpha = sstyle->outlineColor.alpha;
		shadowColor  = MakeCGColorFromRGBA(sstyle->shadowColor,  cs);
		outlineRadius = sstyle->outlineRadius;
		shadowDist = sstyle->shadowDist;
		scaleX = sstyle->scaleX / 100.;
		scaleY = sstyle->scaleY / 100.;
		angle = sstyle->angle;
	}
	
	return self;
}

-(SubATSUISpanEx*)clone
{
	SubATSUISpanEx *ret = [[SubATSUISpanEx alloc] init];
	
	ATSUCreateAndCopyStyle(style, &ret->style);
	ret->primaryColor = CGColorRetain(primaryColor);
	ret->primaryAlpha = primaryAlpha;
	ret->outlineColor = CGColorRetain(outlineColor);
	ret->outlineAlpha = outlineAlpha;
	ret->shadowColor = CGColorRetain(shadowColor);
	ret->outlineRadius = outlineRadius;
	ret->shadowDist = shadowDist;
	ret->scaleX = scaleX;
	ret->scaleY = scaleY;
	ret->angle = angle;
	
	return [ret autorelease];
}
@end

#define span_ex(span) ((SubATSUISpanEx*)span->ex)

static void SetATSUStyleFlag(ATSUStyle style, ATSUAttributeTag t, Boolean v)
{
	const ATSUAttributeTag tags[] = {t};
	const ByteCount		 sizes[] = {sizeof(v)};
	const ATSUAttributeValuePtr vals[] = {&v};
	
	ATSUSetAttributes(style,1,tags,sizes,vals);
}

static void SetATSUStyleOther(ATSUStyle style, ATSUAttributeTag t, ByteCount s, const ATSUAttributeValuePtr v)
{
	const ATSUAttributeTag tags[] = {t};
	const ByteCount		 sizes[] = {s};
	const ATSUAttributeValuePtr vals[] = {v};
	
	ATSUSetAttributes(style,1,tags,sizes,vals);
}

static void SetATSULayoutOther(ATSUTextLayout l, ATSUAttributeTag t, ByteCount s, const ATSUAttributeValuePtr v)
{
	const ATSUAttributeTag tags[] = {t};
	const ByteCount		 sizes[] = {s};
	const ATSUAttributeValuePtr vals[] = {v};
	
	ATSUSetLayoutControls(l,1,tags,sizes,vals);
}

@implementation SubATSUIRenderer

static CGColorSpaceRef GetSRGBColorSpace() {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSData *d = [NSData dataWithContentsOfMappedFile:@"/System/Library/ColorSync/Profiles/sRGB Profile.icc"];
	CGColorSpaceRef ret;
	
	if (d) {
		CMProfileLocation loc;
		loc.locType = cmPtrBasedProfile;
		loc.u.ptrLoc.p = (char*)[d bytes];
		CMProfileRef prof;
		
		CMOpenProfile(&prof,&loc);
		ret = CGColorSpaceCreateWithPlatformColorSpace(prof);
		CMCloseProfile(prof);
	} else ret = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	
	[pool release];
	return ret;
}

-(SubATSUIRenderer*)initWithVideoWidth:(float)width videoHeight:(float)height;
{
	if (self = [super init]) {
		ATSUCreateTextLayout(&layout);
		ubuffer = malloc(sizeof(unichar) * 128);
		srgbCSpace = GetSRGBColorSpace();
		
		origWidth = width;
		origHeight = height;
		
		[[SubContext alloc] initWithNonSSAType:kSubTypeSRT delegate:self];
	}
	
	return self;
}

-(SubATSUIRenderer*)initWithSSAHeader:(NSString*)header videoWidth:(float)width videoHeight:(float)height;
{
	if (self = [super init]) {
		unsigned hlength = [header length];
		unichar *uheader = malloc(sizeof(unichar) * hlength);
		
		header = STStandardizeStringNewlines(header);
		[header getCharacters:uheader];
		
		NSDictionary *headers;
		NSArray *styles;
		SubParseSSAFile(uheader, hlength, &headers, &styles, NULL);
		free(uheader);
		
		ubuffer = malloc(sizeof(unichar) * 128);
		
		origWidth = width;
		origHeight = height;
		
		ATSUCreateTextLayout(&layout);
		srgbCSpace = GetSRGBColorSpace();
		
		[[SubContext alloc] initWithHeaders:headers styles:styles extraData:header delegate:self];
	}
	
	return self;
}

-(void)dealloc
{
	[context release];
	free(ubuffer);
	[super dealloc];
}

-(void)completedHeaderParsing:(SubContext*)sc
{
	context = sc;
	screenScaleX = origWidth / context->resX;
	screenScaleY = origHeight / context->resY;
}

-(float)aspectRatio
{
	return origWidth / origHeight;
}

-(void*)completedStyleParsing:(SubStyle*)s
{
	const ATSUAttributeTag tags[] = {kATSUStyleRenderingOptionsTag, kATSUSizeTag, kATSUQDBoldfaceTag, kATSUQDItalicTag, kATSUQDUnderlineTag, kATSUStyleStrikeThroughTag, kATSUFontTag};
	const ByteCount		 sizes[] = {sizeof(ATSStyleRenderingOptions), sizeof(Fixed), sizeof(Boolean), sizeof(Boolean), sizeof(Boolean), sizeof(Boolean), sizeof(ATSUFontID)};
	
	ATSFontRef fontRef = ATSFontFindFromName((CFStringRef)s->fontname,kATSOptionFlagsDefault);
	ATSUFontID font = FMGetFontFromATSFontRef(fontRef);
	ATSStyleRenderingOptions opt = kATSStyleApplyAntiAliasing;
	Fixed size;
	Boolean b = s->bold, i = s->italic, u = s->underline, st = s->strikeout;
	ATSUStyle style;
		
	const ATSUAttributeValuePtr vals[] = {&opt, &size, &b, &i, &u, &st, &font};
	
	if (font == kATSUInvalidFontID) {
		fontRef = ATSFontFindFromName((CFStringRef)@"Helvetica",kATSOptionFlagsDefault);
		font = FMGetFontFromATSFontRef(fontRef);
	}
	
	if (!s->platformSizeScale) s->platformSizeScale = GetWinFontSizeScale(fontRef);
	size = FloatToFixed(s->size * s->platformSizeScale * screenScaleY);
	
	ATSUCreateStyle(&style);
	ATSUSetAttributes(style, sizeof(tags) / sizeof(ATSUAttributeTag), tags, sizes, vals);
	
	if (s->tracking) {
		Fixed tracking = FloatToFixed(s->tracking);
		
		SetATSUStyleOther(style, kATSUTrackingTag, sizeof(Fixed), &tracking);
	}
	
	if (s->scaleX != 100. || s->scaleY != 100.) {
		CGAffineTransform mat = CGAffineTransformMakeScale(s->scaleX / 100., s->scaleY / 100.);
		
		SetATSUStyleOther(style, kATSUFontMatrixTag, sizeof(CGAffineTransform), &mat);
	}
	
	const ATSUFontFeatureType ftypes[] = {kLigaturesType, kTypographicExtrasType, kTypographicExtrasType, kTypographicExtrasType};
	const ATSUFontFeatureSelector fsels[] = {kCommonLigaturesOnSelector, kSmartQuotesOnSelector, kPeriodsToEllipsisOnSelector, kHyphenToEnDashOnSelector};
	
	ATSUSetFontFeatures(style, sizeof(ftypes) / sizeof(ATSUFontFeatureType), ftypes, fsels);
	
	return style;
}

-(void)releaseStyleEx:(void*)ex
{
	ATSUDisposeStyle(ex);
}

-(void*)spanExtraFromRenderDiv:(SubRenderDiv*)div
{
	return [[[SubATSUISpanEx alloc] initWithStyle:(ATSUStyle)div->styleLine->ex subStyle:div->styleLine colorSpace:srgbCSpace] autorelease];
}

-(void*)cloneSpanExtra:(SubRenderSpan*)span
{
	return [(SubATSUISpanEx*)span->ex clone];
}

-(void)disposeSpanExtra:(void*)ex
{
	SubATSUISpanEx *spanEx = ex;
	[spanEx release];
}

enum {renderMultipleParts = 1, // call ATSUDrawText more than once, needed for color/border changes in the middle of lines
	  renderManualShadows = 2, // CG shadows can't change inside a line... probably
	  renderComplexTransforms = 4}; // can't draw text at all, have to transform each vertex. needed for 3D perspective, or \frz in the middle of a line

-(void)spanChangedTag:(SSATagType)tag span:(SubRenderSpan*)span div:(SubRenderDiv*)div param:(void*)p
{
	SubATSUISpanEx *spanEx = span->ex;
	BOOL isFirstSpan = [div->spans count] == 0;
	Boolean bval;
	int ival;
	float fval;
	NSString *sval;
	Fixed fixval;
	CGColorRef color;
	CGAffineTransform mat;
	
#define bv() bval = *(int*)p;
#define iv() ival = *(int*)p;
#define fv() fval = *(float*)p;
#define sv() sval = *(NSString**)p;
#define fixv() fv(); fixval = FloatToFixed(fval);
#define colorv() color = MakeCGColorFromRGBA(ParseSSAColor(*(int*)p), srgbCSpace);
	
	switch (tag) {
		case tag_b:
			bv();
			SetATSUStyleFlag(spanEx->style, kATSUQDBoldfaceTag, bval != 0);
			break; 
		case tag_i:
			bv();
			SetATSUStyleFlag(spanEx->style, kATSUQDItalicTag, bval);
			break; 
		case tag_u:
			bv();
			SetATSUStyleFlag(spanEx->style, kATSUQDUnderlineTag, bval);
			break; 
		case tag_s:
			bv();
			SetATSUStyleFlag(spanEx->style, kATSUStyleStrikeThroughTag, bval);
			break; 
		case tag_bord:
			fv();
			if (!isFirstSpan) div->render_complexity |= renderMultipleParts;
			spanEx->outlineRadius = fval;
			break;
		case tag_shad:
			fv();
			if (!isFirstSpan) div->render_complexity |= renderMultipleParts | renderManualShadows;
			spanEx->shadowDist = fval;
			break;
		case tag_fn:
			sv();
			ATSUFontID font = FMGetFontFromATSFontRef(ATSFontFindFromName((CFStringRef)sval,kATSOptionFlagsDefault));
			if (font) SetATSUStyleOther(spanEx->style, kATSUFontTag, sizeof(ATSUFontID), &font);
			break;
		case tag_fs:
			fv();
			fixval = FloatToFixed(fval * div->styleLine->platformSizeScale);
			SetATSUStyleOther(spanEx->style, kATSUSizeTag, sizeof(Fixed), &fixval);
			break;
		case tag_1c:
			CGColorRelease(spanEx->primaryColor);
			colorv();
			spanEx->primaryColor = color;
			break;
		case tag_3c:
			CGColorRelease(spanEx->outlineColor);
			if (!isFirstSpan) div->render_complexity |= renderMultipleParts;
			{
				SubRGBAColor rgba = ParseSSAColor(*(int*)p);
				spanEx->outlineColor = MakeCGColorFromRGBOpaque(rgba, srgbCSpace);
				spanEx->outlineAlpha = rgba.alpha;
			}
			break;
		case tag_4c:
			CGColorRelease(spanEx->shadowColor);
			if (!isFirstSpan) div->render_complexity |= renderMultipleParts | renderManualShadows;
			colorv();
			spanEx->shadowColor = color;
			break;
		case tag_fscx:
			fv();
			fval /= 100.;
			mat = CGAffineTransformMakeScale(fval, spanEx->scaleY);
			spanEx->scaleX = fval;
			SetATSUStyleOther(spanEx->style, kATSUFontMatrixTag, sizeof(CGAffineTransform), &mat);
			break;
		case tag_fscy:
			fv();
			fval /= 100.;
			mat = CGAffineTransformMakeScale(spanEx->scaleX, fval);
			spanEx->scaleY = fval;
			SetATSUStyleOther(spanEx->style, kATSUFontMatrixTag, sizeof(CGAffineTransform), &mat);
			break;
		case tag_fsp:
			fixv();
			SetATSUStyleOther(spanEx->style, kATSUTrackingTag, sizeof(Fixed), &fixval);
			break;
		case tag_frz:
			if (!isFirstSpan) div->render_complexity |= renderComplexTransforms; // this one's hard
			fv();
			spanEx->angle = fval;
			break;
		case tag_1a:
			iv();
			spanEx->primaryAlpha = 1.-(ival/255.);
			break;
		case tag_3a:
			iv();
			if (!isFirstSpan) div->render_complexity |= renderMultipleParts;
			spanEx->outlineAlpha = 1.-(ival/255.);
			break;
		case tag_4a:
			iv();
			if (!isFirstSpan) div->render_complexity |= renderMultipleParts | renderManualShadows;
			spanEx->shadowColor = CloneCGColorWithAlpha(spanEx->shadowColor, 1.-(ival/255.));
			break;
		case tag_r:
			sv();
			if (!isFirstSpan) div->render_complexity |= renderMultipleParts | renderManualShadows;
			{
				SubStyle *style = [context->styles objectForKey:sval];
				if (!style) style = div->styleLine;
				
				[spanEx release];
				span->ex = [[SubATSUISpanEx alloc] initWithStyle:(ATSUStyle)style->ex subStyle:style colorSpace:srgbCSpace];
			}
			break;
		default:
			NSLog(@"Perian: unimplemented tag %d",tag);
	}
}

#pragma mark Rendering Helper Functions

static ATSUTextMeasurement GetLineHeight(ATSUTextLayout layout, UniCharArrayOffset lpos)
{
	ATSUTextMeasurement ascent, descent;
	
	ATSUGetLineControl(layout, lpos, kATSULineAscentTag,  sizeof(ATSUTextMeasurement), &ascent,  NULL);
	ATSUGetLineControl(layout, lpos, kATSULineDescentTag, sizeof(ATSUTextMeasurement), &descent, NULL);
	
	return ascent + descent;
}

static void GetTypographicRectangleForLayout(ATSUTextLayout layout, UniCharArrayOffset *breaks, ItemCount breakCount, Fixed *height, Fixed *width)
{
	ATSTrapezoid trap = {0};
	ItemCount trapCount;
	FixedRect largeRect = {0};
	Fixed baseY = 0;
	int i;

	for (i = breakCount; i >= 0; i--) {		
		UniCharArrayOffset end = breaks[i+1];
		FixedRect rect;
		
		ATSUGetGlyphBounds(layout, 0, baseY, breaks[i], end-breaks[i], kATSUseDeviceOrigins, 1, &trap, &trapCount);

		baseY += GetLineHeight(layout, breaks[i]);
		
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

enum {fillc, strokec};

static void SetColor(CGContextRef c, int whichcolor, CGColorRef col)
{
	if (whichcolor == fillc) CGContextSetFillColorWithColor(c, col);
	else CGContextSetStrokeColorWithColor(c, col);
}

static void SetStyleSpanRuns(ATSUTextLayout layout, SubRenderDiv *div)
{
	unsigned span_count = [div->spans count];
	int i;
	
	for (i = 0; i < span_count; i++) {
		SubRenderSpan *span = [div->spans objectAtIndex:i];
		UniCharArrayOffset next = (i == span_count-1) ? [div->text length] : ((SubRenderSpan*)[div->spans objectAtIndex:i+1])->offset;
		ATSUSetRunStyle(layout, span_ex(span)->style, span->offset, next - span->offset);
	}
}

static Fixed RoundFixed(Fixed n) {return IntToFixed(FixedToInt(n));}

static void SetLayoutPositioning(ATSUTextLayout layout, Fixed lineWidth, UInt8 align)
{
	const ATSUAttributeTag tags[] = {kATSULineFlushFactorTag, kATSULineWidthTag, kATSULineRotationTag};
	const ByteCount		 sizes[] = {sizeof(Fract), sizeof(ATSUTextMeasurement), sizeof(Fixed)};
	Fract alignment;
	Fixed fixzero = 0;
	const ATSUAttributeValuePtr vals[] = {&alignment, &lineWidth, &fixzero};
	
	switch (align) {
		case kSubAlignmentLeft:
			alignment = FloatToFract(0);
			break;
		case kSubAlignmentCenter:
			alignment = kATSUCenterAlignment;
			break;
		case kSubAlignmentRight:
			alignment = fract1;
			break;
	}
	
	ATSUSetLayoutControls(layout,sizeof(vals) / sizeof(ATSUAttributeValuePtr),tags,sizes,vals);
}

static UniCharArrayOffset *FindLineBreaks(ATSUTextLayout layout, SubRenderDiv *div, ItemCount *nbreaks, Fixed breakingWidth, unsigned textLen)
{
	UniCharArrayOffset *breaks;
	ItemCount breakCount=0;
	
	switch (div->wrapStyle) {
		case kSubLineWrapTopWider:
		case kSubLineWrapSimple:
		case kSubLineWrapBottomWider:
			ATSUBatchBreakLines(layout, kATSUFromTextBeginning, kATSUToTextEnd, breakingWidth, &breakCount);
			break;
		case kSubLineWrapNone:
			ATSUBatchBreakLines(layout, kATSUFromTextBeginning, kATSUToTextEnd, positiveInfinity, &breakCount);
			break;
	}
	
	breaks = malloc(sizeof(UniCharArrayOffset) * (breakCount+2));
	ATSUGetSoftLineBreaks(layout, kATSUFromTextBeginning, kATSUToTextEnd, breakCount, &breaks[1], NULL);
	
	breaks[0] = 0;
	breaks[breakCount+1] = textLen;
		
	*nbreaks = breakCount;
	return breaks;
}

typedef struct {
	UniCharArrayOffset *breaks;
	ItemCount breakCount;
	unsigned lStart, lEnd;
	SInt8 direction;
} BreakContext;

enum {kTextLayerShadow, kTextLayerOutline, kTextLayerPrimary};

static BOOL SetupCGForSpan(CGContextRef c, SubATSUISpanEx *spanEx, SubATSUISpanEx *lastSpanEx, int textType, BOOL endLayer)
{	
#define if_different(x) if (!lastSpanEx || lastSpanEx-> x != spanEx-> x)
	
	switch (textType) {
		case kTextLayerShadow:
			if_different(shadowColor) {
				if (endLayer) CGContextEndTransparencyLayer(c);

				SetColor(c, fillc, spanEx->shadowColor);
				SetColor(c, strokec, spanEx->shadowColor);
				if (CGColorGetAlpha(spanEx->shadowColor) != 1.) {
					endLayer = YES;
					CGContextBeginTransparencyLayer(c, NULL);
				} else endLayer = NO;
			}
			break;
			
		case kTextLayerOutline:
			if_different(outlineRadius) CGContextSetLineWidth(c, spanEx->outlineRadius*2. + .5);
			if_different(outlineColor)  SetColor(c, strokec, spanEx->outlineColor);
			
			if_different(outlineAlpha) {
				if (endLayer) CGContextEndTransparencyLayer(c);
				
				CGContextSetAlpha(c, spanEx->outlineAlpha);
				if (spanEx->outlineAlpha != 1.) {
					endLayer = YES;
					CGContextBeginTransparencyLayer(c, NULL);
				} else endLayer = NO;
			}
				
			break;
		case kTextLayerPrimary:
			if_different(primaryColor) SetColor(c, fillc, spanEx->primaryColor);
			
			if_different(primaryAlpha) {
				if (endLayer) CGContextEndTransparencyLayer(c);

				CGContextSetAlpha(c, spanEx->primaryAlpha);
				if (spanEx->primaryAlpha != 1.) {
					endLayer = YES;
					CGContextBeginTransparencyLayer(c, NULL);
				} else endLayer = NO;
			}
			break;
	}
	
	return endLayer;
}

static Fixed DrawTextLines(CGContextRef c, ATSUTextLayout layout, SubRenderDiv *div, const BreakContext breakc, Fixed penX, Fixed penY, SubATSUISpanEx *firstSpanEx, int textType)
{
	int i;
	BOOL endLayer = NO;
	SubATSUISpanEx *lastSpanEx = nil;
	const CGTextDrawingMode textModes[] = {kCGTextFillStroke, kCGTextStroke, kCGTextFill};
		
	CGContextSetTextDrawingMode(c, textModes[textType]);
	
	if (!(div->render_complexity & renderMultipleParts)) endLayer = SetupCGForSpan(c, firstSpanEx, lastSpanEx, textType, endLayer);
	
	for (i = breakc.lStart; i != breakc.lEnd; i -= breakc.direction) {
		UniCharArrayOffset thisBreak = breakc.breaks[i], nextBreak = breakc.breaks[i+1], linelen = nextBreak - thisBreak;
		float extraHeight = 0;
		
		if (!(div->render_complexity & renderMultipleParts)) {
			ATSUDrawText(layout, thisBreak, linelen, RoundFixed(penX), RoundFixed(penY));
			extraHeight = div->styleLine->outlineRadius;
		} else {
			int j, spans = [div->spans count];
			for (j = 0; j < spans; j++) {
				SubRenderSpan *span = [div->spans objectAtIndex:j];
				SubATSUISpanEx *spanEx = span->ex;
				UniCharArrayOffset spanLen, drawStart, drawLen;
				
				if (j < spans-1) {
					SubRenderSpan *nextSpan = [div->spans objectAtIndex:j+1];
					spanLen = nextSpan->offset - span->offset;
				} else spanLen = [div->text length] - span->offset;
				
				if (spanLen == 0) continue;
				if ((span->offset + spanLen) < thisBreak) continue;
				if (span->offset >= nextBreak) break;
				
				if (span->offset < thisBreak) {
					drawStart = thisBreak;
					drawLen = spanLen - (thisBreak - span->offset);
				} else {
					drawStart = span->offset;
					drawLen = spanLen;
				}
				
				endLayer = SetupCGForSpan(c, spanEx, lastSpanEx, textType, endLayer);
				ATSUDrawText(layout, drawStart, drawLen, RoundFixed((textType == kTextLayerShadow) ? (penX + FloatToFixed(spanEx->shadowDist)) : penX), 
														 RoundFixed((textType == kTextLayerShadow) ? (penY - FloatToFixed(spanEx->shadowDist)) : penY));
				extraHeight = MAX(extraHeight, spanEx->outlineRadius);
			}
		
		}

		penY += breakc.direction * (GetLineHeight(layout, thisBreak) + FloatToFixed(extraHeight));
	}
	
	if (endLayer) CGContextEndTransparencyLayer(c);

	return penY;
}

static Fixed DrawOneTextDiv(CGContextRef c, ATSUTextLayout layout, SubRenderDiv *div, const BreakContext breakc, Fixed penX, Fixed penY)
{
	SubATSUISpanEx *firstSpanEx = ((SubRenderSpan*)[div->spans objectAtIndex:0])->ex;
	BOOL endLayer = NO;
	
	if (!(div->render_complexity & renderManualShadows)) {
		if (firstSpanEx->shadowDist) {
			endLayer = YES;
			CGContextSetShadowWithColor(c, CGSizeMake(firstSpanEx->shadowDist + .5, -(firstSpanEx->shadowDist + .5)), 0, firstSpanEx->shadowColor);
			CGContextBeginTransparencyLayer(c, NULL);
		}
	} else DrawTextLines(c, layout, div, breakc, penX, penY, firstSpanEx, kTextLayerShadow);
	
	DrawTextLines(c, layout, div, breakc, penX, penY, firstSpanEx, kTextLayerOutline);
	penY = DrawTextLines(c, layout, div, breakc, penX, penY, firstSpanEx, kTextLayerPrimary);
	
	if (endLayer) CGContextEndTransparencyLayer(c);
	
	return penY;
}

#pragma mark Main Renderer Function

-(void)renderPacket:(NSString *)packet inContext:(CGContextRef)c width:(float)cWidth height:(float)cHeight
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	ubuffer = realloc(ubuffer, sizeof(unichar) * [packet length]);
	NSArray *divs = SubParsePacket(packet, context, self, ubuffer);
	unsigned div_count = [divs count];
	int i;
	Fixed bottomPen = 0, topPen = 0, centerPen = 0, *storePen=NULL;
	
	CGContextSaveGState(c);
	
	if (cWidth != origWidth || cHeight != origHeight) CGContextScaleCTM(c, cWidth / origWidth, cHeight / origHeight);
	SetATSULayoutOther(layout, kATSUCGContextTag, sizeof(CGContextRef), &c);
	
	CGContextSetLineCap(c, kCGLineCapRound);
	CGContextSetLineJoin(c, kCGLineJoinRound);
	
	for (i = 0; i < div_count; i++) {
		SubRenderDiv *div = [divs objectAtIndex:i];
		unsigned textLen = [div->text length];
		if (!textLen) continue;
		
		NSRect marginRect = NSMakeRect(div->marginL, div->marginV, context->resX - div->marginL - div->marginR, context->resY - div->marginV - div->marginV);
		
		marginRect.origin.x *= screenScaleX;
		marginRect.origin.y *= screenScaleY;
		marginRect.size.width *= screenScaleX;
		marginRect.size.height *= screenScaleY;

		Fixed penY, penX, breakingWidth = FloatToFixed(marginRect.size.width); BreakContext breakc = {0};

		[div->text getCharacters:ubuffer];
		
		ATSUSetTextPointerLocation(layout, ubuffer, kATSUFromTextBeginning, kATSUToTextEnd, textLen);		
		ATSUSetTransientFontMatching(layout,TRUE);
		
		SetLayoutPositioning(layout, breakingWidth, div->alignH);	

		SetStyleSpanRuns(layout, div);
		
		ItemCount breakCount;
		UniCharArrayOffset *breaks = FindLineBreaks(layout, div, &breakCount, breakingWidth, textLen);
		
		if (div->posX == -1) {
			penX = FloatToFixed(NSMinX(marginRect));

			switch(div->alignV) {
				case kSubAlignmentBottom: default:
					if (!bottomPen) {
						ATSUTextMeasurement bottomLineDescent;
						ATSUGetLineControl(layout, kATSUFromTextBeginning, kATSULineDescentTag, sizeof(ATSUTextMeasurement), &bottomLineDescent, NULL);
						penY = FloatToFixed(NSMinY(marginRect)) + bottomLineDescent;
					} else penY = bottomPen;
					
					storePen = &bottomPen; breakc.lStart = breakCount; breakc.lEnd = -1; breakc.direction = 1;
					break;
				case kSubAlignmentMiddle:
					if (!centerPen) {
						ATSUTextMeasurement imageWidth, imageHeight;
						
						GetTypographicRectangleForLayout(layout, breaks, breakCount, &imageHeight, &imageWidth);
						penY = (FloatToFixed(NSMidY(marginRect)) / 2) + (imageHeight / 2);
					} else penY = centerPen;
					
					storePen = &centerPen; breakc.lStart = 0; breakc.lEnd = breakCount+1; breakc.direction = -1;
					break;
				case kSubAlignmentTop:
					if (!topPen) {
						penY = FloatToFixed(NSMaxY(marginRect)) - GetLineHeight(layout, kATSUFromTextBeginning);
					} else penY = topPen;
					
					storePen = &topPen; breakc.lStart = 0; breakc.lEnd = breakCount+1; breakc.direction = -1;
					break;
			}
		} else {
			ATSUTextMeasurement imageWidth, imageHeight;
						
			GetTypographicRectangleForLayout(layout, breaks, breakCount, &imageHeight, &imageWidth);

			penX = FloatToFixed(div->posX * screenScaleX);
			penY = FloatToFixed((context->resY - div->posY) * screenScaleY);
			
			switch (div->alignH) {
				case kSubAlignmentCenter: penX -= imageWidth / 2; break;
				case kSubAlignmentRight: penX -= imageWidth;
			}
			
			switch (div->alignV) {
				case kSubAlignmentMiddle: penY -= imageHeight / 2; break;
				case kSubAlignmentTop: penY -= imageHeight;
			}
						
			SetLayoutPositioning(layout, imageWidth, div->alignH);
			storePen = NULL; breakc.lStart = breakCount; breakc.lEnd = -1; breakc.direction = 1;
		}
		
		{
			SubATSUISpanEx *firstspan = ((SubRenderSpan*)[div->spans objectAtIndex:0])->ex;
			Fixed fangle = FloatToFixed(firstspan->angle);
			
			SetATSULayoutOther(layout, kATSULineRotationTag, sizeof(Fixed), &fangle);
		}
		
		breakc.breakCount = breakCount;
		breakc.breaks = breaks;
		
		penY = DrawOneTextDiv(c, layout, div, breakc, penX, penY);
		
		if (storePen) *storePen = penY;
		
		free(breaks);
	}
	
	CGContextRestoreGState(c);
	[pool release];
}

extern SubtitleRendererPtr SubInitForSSA(char *header, size_t headerLen, int width, int height)
{
	NSString *hdr = [[NSString alloc] initWithBytesNoCopy:(void*)header length:headerLen encoding:NSUTF8StringEncoding freeWhenDone:NO];

	SubtitleRendererPtr s = [[SubATSUIRenderer alloc] initWithSSAHeader:hdr videoWidth:width videoHeight:height];
	[hdr release];
	return s;
}

extern SubtitleRendererPtr SubInitNonSSA(int width, int height)
{
	return [[SubATSUIRenderer alloc] initWithVideoWidth:width videoHeight:height];
}

extern CGColorSpaceRef SubGetColorSpace(SubtitleRendererPtr s)
{
	return s->srgbCSpace;
}

extern void SubRenderPacket(SubtitleRendererPtr s, CGContextRef c, CFStringRef str, int cWidth, int cHeight)
{
	[s renderPacket:(NSString*)str inContext:c width:cWidth height:cHeight];
}

extern void SubDisposeRenderer(SubtitleRendererPtr s)
{
	[s release];
}
@end

#pragma options align=mac68k
typedef struct TT_Header
{
    Fixed   Table_Version;
    Fixed   Font_Revision;
	
    SInt32  CheckSum_Adjust;
    SInt32  Magic_Number;
	
    UInt16  Flags;
    UInt16  Units_Per_EM;
	
    SInt32  Created [2];
    SInt32  Modified[2];
	
    SInt16  xMin;
    SInt16  yMin;
    SInt16  xMax;
    SInt16  yMax;
	
    UInt16  Mac_Style;
    UInt16  Lowest_Rec_PPEM;
	
    SInt16  Font_Direction;
    SInt16  Index_To_Loc_Format;
    SInt16  Glyph_Data_Format;
	
} TT_Header;

//Windows/OS/2 TrueType metrics table
typedef struct TT_OS2
{
    UInt16   version;                /* 0x0001 - more or 0xFFFF */
    SInt16   xAvgCharWidth;
    UInt16   usWeightClass;
    UInt16   usWidthClass;
    SInt16   fsType;
    SInt16   ySubscriptXSize;
    SInt16   ySubscriptYSize;
    SInt16   ySubscriptXOffset;
    SInt16   ySubscriptYOffset;
    SInt16   ySuperscriptXSize;
    SInt16   ySuperscriptYSize;
    SInt16   ySuperscriptXOffset;
    SInt16   ySuperscriptYOffset;
    SInt16   yStrikeoutSize;
    SInt16   yStrikeoutPosition;
    SInt16   sFamilyClass;
	
    UInt8    panose[10];
	
    UInt32   ulUnicodeRange1;        /* Bits 0-31   */
    UInt32   ulUnicodeRange2;        /* Bits 32-63  */
    UInt32   ulUnicodeRange3;        /* Bits 64-95  */
    UInt32   ulUnicodeRange4;        /* Bits 96-127 */
	
    SInt8    achVendID[4];
	
    UInt16   fsSelection;
    UInt16   usFirstCharIndex;
    UInt16   usLastCharIndex;
    SInt16   sTypoAscender;
    SInt16   sTypoDescender;
    SInt16   sTypoLineGap;
    UInt16   usWinAscent;
    UInt16   usWinDescent;
	
    /* only version 1 tables: */
	
    UInt32   ulCodePageRange1;       /* Bits 0-31   */
    UInt32   ulCodePageRange2;       /* Bits 32-63  */
	
    /* only version 2 tables: */
	
    SInt16   sxHeight;
    SInt16   sCapHeight;
    UInt16   usDefaultChar;
    UInt16   usBreakChar;
    UInt16   usMaxContext;
	
} TT_OS2;
#pragma options align=reset

// Windows and OS X use different TrueType fields to measure text.
// Some Windows fonts have one field set incorrectly, so we have to compensate.
// XXX This function doesn't read from the right fonts; if we're using italic variant, it should get the ATSFontRef for that
static float GetWinFontSizeScale(ATSFontRef font)
{
	TT_Header headTable = {0};
	TT_OS2 os2Table = {0};
	ByteCount os2Size = 0, headSize = 0;
	
	OSErr err = ATSFontGetTable(font, 'OS/2', 0, 0, NULL, &os2Size);
	if (!os2Size || err) return 1;
	
	err = ATSFontGetTable(font, 'head', 0, 0, NULL, &headSize);
	if (!headSize || err) return 1;

	ATSFontGetTable(font, 'head', 0, headSize, &headTable, &headSize);
	ATSFontGetTable(font, 'OS/2', 0, os2Size, &os2Table, &os2Size);
		
	// ppem = units_per_em * lfheight / (winAscent + winDescent) c.f. WINE
	// lfheight being SSA font size
	unsigned oA = EndianU16_BtoN(os2Table.usWinAscent), oD = EndianU16_BtoN(os2Table.usWinDescent);
	unsigned winSize = oA + oD;
		
	unsigned unitsPerEM = EndianU16_BtoN(headTable.Units_Per_EM);
	
	return (winSize && unitsPerEM) ? ((float)unitsPerEM / (float)winSize) : 1;
}
