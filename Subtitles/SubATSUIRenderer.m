/*
 * SubATSUIRenderer.m
 * Created by Alexander Strange on 7/30/07.
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

#import "SubATSUIRenderer.h"
#import "SubImport.h"
#import "SubParsing.h"
#import "SubUtilities.h"
#import "Codecprintf.h"
#import "CommonUtils.h"
#import <pthread.h>

static float GetWinFontSizeScale(ATSFontRef font);
static void FindAllPossibleLineBreaks(TextBreakLocatorRef breakLocator, const unichar *uline, UniCharArrayOffset lineLen, unsigned char *breakOpportunities);
static ATSUFontID GetFontIDForSSAName(NSString *name);

#define declare_bitfield(name, bits) unsigned char name[bits / 8 + 1]; bzero(name, sizeof(name));
#define bitfield_set(name, bit) name[(bit) / 8] |= 1 << ((bit) % 8);
#define bitfield_test(name, bit) ((name[(bit) / 8] & (1 << ((bit) % 8))) != 0)

@interface SubATSUISpanEx : NSObject {
	@public;
	ATSUStyle style;
	CGColorRef primaryColor, outlineColor, shadowColor;
	Float32 outlineRadius, shadowDist, scaleX, scaleY, primaryAlpha, outlineAlpha, angle, platformSizeScale, fontSize;
	BOOL blurEdges, vertical;
	ATSUFontID font;
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

-(void)finalize
{
	ATSUDisposeStyle(style);
	CGColorRelease(primaryColor);
	CGColorRelease(outlineColor);
	CGColorRelease(shadowColor);
	[super finalize];
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
		platformSizeScale = sstyle->platformSizeScale;
		fontSize = sstyle->size;
		vertical = sstyle->vertical;
		font = GetFontIDForSSAName(sstyle->fontname);
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
	ret->platformSizeScale = platformSizeScale;
	ret->fontSize = fontSize;
	ret->font = font;
	ret->blurEdges = blurEdges;
	ret->vertical = vertical;
	
	return ret;
}

-(NSString*)description
{
	return [NSString stringWithFormat:@"SpanEx with alpha %f/%f", primaryAlpha, outlineAlpha];
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

-(SubATSUIRenderer*)initWithVideoWidth:(float)width videoHeight:(float)height;
{
	if (self = [super init]) {
		ATSUCreateTextLayout(&layout);
		breakbuffer = malloc(sizeof(UniCharArrayOffset) * 2);
		
		srgbCSpace = GetSRGBColorSpace();
		
		videoWidth = width;
		videoHeight = height;
		
		UCCreateTextBreakLocator(NULL, 0, kUCTextBreakLineMask, &breakLocator);
		context = [[SubContext alloc] initWithNonSSAType:kSubTypeSRT delegate:self];
		
		drawTextBounds = CFPreferencesGetAppBooleanValue(CFSTR("DrawSubTextBounds"), PERIAN_PREF_DOMAIN, NULL);
	}
	
	return self;
}

-(SubATSUIRenderer*)initWithSSAHeader:(NSString*)header videoWidth:(float)width videoHeight:(float)height;
{
	if (self = [super init]) {
		header = STStandardizeStringNewlines(header);
		
		NSDictionary *headers;
		NSArray *styles;
		SubParseSSAFile(header, &headers, &styles, NULL);
		
		breakbuffer = malloc(sizeof(UniCharArrayOffset) * 2);
		
		videoWidth = width;
		videoHeight = height;
		
		ATSUCreateTextLayout(&layout);
		srgbCSpace = GetSRGBColorSpace();
		
		UCCreateTextBreakLocator(NULL, 0, kUCTextBreakLineMask, &breakLocator);
		context = [[SubContext alloc] initWithHeaders:headers styles:styles delegate:self];
		
		drawTextBounds = CFPreferencesGetAppBooleanValue(CFSTR("DrawSubTextBounds"), PERIAN_PREF_DOMAIN, NULL);
	}
	
	return self;
}

-(void)dealloc
{
	[context release];
	free(breakbuffer);
	UCDisposeTextBreakLocator(&breakLocator);
	ATSUDisposeTextLayout(layout);
	[super dealloc];
}

-(void)finalize
{
	free(breakbuffer);
	UCDisposeTextBreakLocator(&breakLocator);
	ATSUDisposeTextLayout(layout);
	[super finalize];
}

-(void)completedHeaderParsing:(SubContext*)sc
{
	screenScaleX = videoWidth / sc->resX;
	screenScaleY = videoHeight / sc->resY;
}

-(float)aspectRatio
{
	return videoWidth / videoHeight;
}

static NSMutableDictionary *fontIDCache = nil;
static ItemCount fontCount;
static ATSUFontID *fontIDs = NULL;
static pthread_mutex_t fontIDMutex = PTHREAD_MUTEX_INITIALIZER;

static void CleanupFontIDCache() __attribute__((destructor));
static void CleanupFontIDCache()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	if (fontIDCache) [fontIDCache release];
	if (fontIDs) free(fontIDs);
	fontIDCache = nil;
	fontIDs = NULL;
	[pool release];
}

// Assumes ATSUFontID = ATSFontRef. This is true.
static ATSUFontID GetFontIDForSSAName(NSString *name)
{	
	NSNumber *idN = nil;
	NSString *lcname = [name lowercaseString];
	
	if (fontIDCache)
		idN = [fontIDCache objectForKey:lcname];
	
	if (idN)
		return [idN intValue];
	
	if (pthread_mutex_trylock(&fontIDMutex)) {
		// if another thread is looking a font up, it might be the same font
		// so restart and check the cache again
		pthread_mutex_lock(&fontIDMutex);
		pthread_mutex_unlock(&fontIDMutex);
		return GetFontIDForSSAName(name);
	}
	
	if (!fontIDCache)
		fontIDCache = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
					   [NSNumber numberWithInt:ATSFontFindFromName((CFStringRef)kSubDefaultFontName, kATSOptionFlagsDefault)],
					   kSubDefaultFontName, nil];
	
	ByteCount nlen = [name length];
	NSData *unameData;
	const unichar *uname = STUnicodeForString(name, &unameData);
	ATSUFontID font;
	
	// should be kFontMicrosoftPlatform for the platform code
	// but isn't for bug workarounds in 10.4
	ATSUFindFontFromName(uname, nlen * sizeof(unichar), kFontFamilyName, kFontNoPlatformCode, kFontNoScript, kFontNoLanguage, &font);
	
	[unameData release];
	
	if (font == kATSUInvalidFontID) font = ATSFontFindFromName((CFStringRef)name, kATSOptionFlagsDefault);
	if (font == kATSUInvalidFontID) { // try a case-insensitive search
		if (!fontIDs) {
			ATSUFontCount(&fontCount);
			fontIDs = malloc(sizeof(ATSUFontID[fontCount]));
			ATSUGetFontIDs(fontIDs, fontCount, &fontCount);
		}
				
		ByteCount len;
		ItemCount x, index;
		const ByteCount kBufLength = 1024/sizeof(unichar);
		unichar buf[kBufLength];
	  
		for (x = 0; x < fontCount && font == kATSUInvalidFontID; x++) {
			ATSUFindFontName(fontIDs[x], kFontFamilyName, kFontMicrosoftPlatform, kFontNoScript, kFontNoLanguage, kBufLength, (Ptr)buf, &len, &index);
			NSString *fname = [[NSString alloc] initWithCharactersNoCopy:buf length:len/sizeof(unichar) freeWhenDone:NO];

			if ([name caseInsensitiveCompare:fname] == NSOrderedSame)
				font = fontIDs[x];
			
			[fname release];
		}		
	}
	
	if (font == kATSUInvalidFontID && ![name isEqualToString:kSubDefaultFontName])
		font = [[fontIDCache objectForKey:kSubDefaultFontName] intValue]; // final fallback
	
	/*{
		NSString *fontPSName = nil;
		ATSFontGetPostScriptName(font, kATSOptionFlagsDefault, (CFStringRef*)&fontPSName);
		NSLog(@"Font lookup: %@ -> %@", name, fontPSName);
		[fontPSName autorelease];
	}*/
	[fontIDCache setValue:[NSNumber numberWithInt:font] forKey:lcname];
	
	pthread_mutex_unlock(&fontIDMutex);
	 
	return font;
}

-(void*)completedStyleParsing:(SubStyle*)s
{
	const ATSUAttributeTag tags[] = {kATSUStyleRenderingOptionsTag, kATSUSizeTag, kATSUQDBoldfaceTag, kATSUQDItalicTag, kATSUQDUnderlineTag, kATSUStyleStrikeThroughTag, kATSUFontTag};
	const ByteCount		 sizes[] = {sizeof(ATSStyleRenderingOptions), sizeof(Fixed), sizeof(Boolean), sizeof(Boolean), sizeof(Boolean), sizeof(Boolean), sizeof(ATSUFontID)};
	
	NSString *fn = s->fontname;
	ATSUFontID font = GetFontIDForSSAName(fn);
	ATSFontRef fontRef = font;
	ATSStyleRenderingOptions opt = kATSStyleApplyAntiAliasing;
	Fixed size;
	Boolean b = s->weight > 0, i = s->italic, u = s->underline, st = s->strikeout;
	ATSUStyle style;
		
	const ATSUAttributeValuePtr vals[] = {&opt, &size, &b, &i, &u, &st, &font};
	
	if (!s->platformSizeScale) s->platformSizeScale = GetWinFontSizeScale(fontRef);
	size = FloatToFixed(s->size * s->platformSizeScale * screenScaleY); //FIXME: several other values also change relative to PlayRes but aren't handled
	
	ATSUCreateStyle(&style);
	ATSUSetAttributes(style, sizeof(tags) / sizeof(ATSUAttributeTag), tags, sizes, vals);
	
	if (s->tracking > 0) { // bug in VSFilter: negative tracking in style lines is ignored
		Fixed tracking = FloatToFixed(s->tracking);
		
		SetATSUStyleOther(style, kATSUAfterWithStreamShiftTag, sizeof(Fixed), &tracking);
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

-(void)releaseStyleExtra:(void*)ex
{
	ATSUDisposeStyle(ex);
}

-(void*)spanExtraFromRenderDiv:(SubRenderDiv*)div
{
	return [[SubATSUISpanEx alloc] initWithStyle:(ATSUStyle)div->styleLine->ex subStyle:div->styleLine colorSpace:srgbCSpace];
}

-(void*)cloneSpanExtra:(SubRenderSpan*)span
{
	return [(SubATSUISpanEx*)span->ex clone];
}

-(void)releaseSpanExtra:(void*)ex
{
	SubATSUISpanEx *spanEx = ex;
	[spanEx release];
}

-(NSString*)describeSpanEx:(void*)ex
{
	SubATSUISpanEx *spanEx = ex;
	return [spanEx description];
}

static void UpdateFontNameSize(SubATSUISpanEx *spanEx, float screenScale)
{
	Fixed fSize = FloatToFixed(spanEx->fontSize * spanEx->platformSizeScale * screenScale);
	SetATSUStyleOther(spanEx->style, kATSUFontTag, sizeof(ATSUFontID), &spanEx->font);
	SetATSUStyleOther(spanEx->style, kATSUSizeTag, sizeof(Fixed), &fSize);
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
	ATSFontRef oldFont;
	
#define bv() bval = *(int*)p;
#define iv() ival = *(int*)p;
#define fv() fval = *(float*)p;
#define sv() sval = *(NSString**)p;
#define fixv() fv(); fixval = FloatToFixed(fval);
#define colorv() color = MakeCGColorFromRGBA(ParseSSAColor(*(int*)p), srgbCSpace);
	
	switch (tag) {
		case tag_b:
			bv(); // FIXME: font weight variations
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
			if (![sval length]) sval = div->styleLine->fontname;
			oldFont = spanEx->font;
			spanEx->vertical = ParseFontVerticality(&sval);
			spanEx->font = GetFontIDForSSAName(sval);
			if (oldFont != spanEx->font) spanEx->platformSizeScale = GetWinFontSizeScale(spanEx->font);
			UpdateFontNameSize(spanEx, screenScaleY);
			break;
		case tag_fs:
			fv();
			spanEx->fontSize = fval;
			UpdateFontNameSize(spanEx, screenScaleY);
			break;
		case tag_1c:
			CGColorRelease(spanEx->primaryColor);
			if (!isFirstSpan) div->render_complexity |= renderMultipleParts;
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
			spanEx->scaleX = fval / 100.;
			mat = CGAffineTransformMakeScale(spanEx->scaleX, spanEx->scaleY);
			SetATSUStyleOther(spanEx->style, kATSUFontMatrixTag, sizeof(CGAffineTransform), &mat);
			break;
		case tag_fscy:
			fv();
			spanEx->scaleY = fval / 100.;
			mat = CGAffineTransformMakeScale(spanEx->scaleX, spanEx->scaleY);
			SetATSUStyleOther(spanEx->style, kATSUFontMatrixTag, sizeof(CGAffineTransform), &mat);
			break;
		case tag_fsp:
			fixv();
			SetATSUStyleOther(spanEx->style, kATSUAfterWithStreamShiftTag, sizeof(Fixed), &fixval);
			break;
		case tag_frz:
			fv();
			if (!isFirstSpan) div->render_complexity |= renderComplexTransforms; // this one's hard
			spanEx->angle = fval;
			break;
		case tag_1a:
			iv();
			if (!isFirstSpan) div->render_complexity |= renderMultipleParts;
			spanEx->primaryAlpha = (255-ival)/255.;
			break;
		case tag_3a:
			iv();
			if (!isFirstSpan) div->render_complexity |= renderMultipleParts;
			spanEx->outlineAlpha = (255-ival)/255.;
			break;
		case tag_4a:
			iv();
			if (!isFirstSpan) div->render_complexity |= renderMultipleParts | renderManualShadows;
			spanEx->shadowColor = CloneCGColorWithAlpha(spanEx->shadowColor, (255-ival)/255.);
			break;
		case tag_alpha:
			iv();
			fval = (255-ival)/255.;
			if (!isFirstSpan) div->render_complexity |= renderMultipleParts | renderManualShadows;
			spanEx->primaryAlpha = spanEx->outlineAlpha = fval;
			spanEx->shadowColor = CloneCGColorWithAlpha(spanEx->shadowColor, fval);
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
		case tag_be:
			bv();
			if (!isFirstSpan) div->render_complexity |= renderMultipleParts; //FIXME: blur edges
			spanEx->blurEdges = bval;
			break;
		default:
			Codecprintf(NULL, "Unimplemented SSA tag #%d\n",tag);
	}
}

#pragma mark Rendering Helper Functions

// see comment for GetTypographicRectangleForLayout
static ATSUTextMeasurement GetLineHeight(ATSUTextLayout layout, UniCharArrayOffset lpos, Boolean includeDescent)
{
	ATSUTextMeasurement ascent = 0, descent = 0;
	
	ATSUGetLineControl(layout, lpos, kATSULineAscentTag,  sizeof(ATSUTextMeasurement), &ascent,  NULL);
	if (includeDescent) ATSUGetLineControl(layout, lpos, kATSULineDescentTag, sizeof(ATSUTextMeasurement), &descent, NULL);
	
	return ascent + descent;
}

static void ExpandCGRect(CGRect *rect, float radius)
{
	rect->origin.x -= radius;
	rect->origin.y -= radius;
	rect->size.height += radius*2.;
	rect->size.width += radius*2.;
}

// some broken fonts have very wrong typographic values set, and so this occasionally gives nonsense
// it should be checked against the real pixel box (see #if 0 below), but for correct fonts it looks much better
static void GetTypographicRectangleForLayout(ATSUTextLayout layout, UniCharArrayOffset *breaks, ItemCount breakCount, Fixed extraHeight, Fixed *lX, Fixed *lY, Fixed *height, Fixed *width)
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

		baseY += GetLineHeight(layout, breaks[i], YES) + extraHeight;
		
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
	
	if (lX) *lX = largeRect.left;
	if (lY) *lY = largeRect.bottom;
	*height = largeRect.bottom - largeRect.top;
	*width = largeRect.right - largeRect.left;
}

// Draw the text bounds on screen under the actual text
// Note that it almost never appears where it's supposed to be, and I'm not sure if that's my fault or ATSUI's
static void VisualizeLayoutLineHeights(CGContextRef c, ATSUTextLayout layout, UniCharArrayOffset *breaks, ItemCount breakCount, Fixed extraHeight, Fixed penX, Fixed penY, float screenHeight)
{
	ATSTrapezoid trap = {0};
	Rect pixRect = {0};
	ItemCount trapCount;
	int i;
	
	CGContextSetLineWidth(c, 3.0);
	
	for (i = breakCount; i >= 0; i--) {
		UniCharArrayOffset end = breaks[i+1];

		ATSUMeasureTextImage(layout, breaks[i], end-breaks[i], 0, 0, &pixRect);
		ATSUGetGlyphBounds(layout, 0, 0, breaks[i], end-breaks[i], kATSUseDeviceOrigins, 1, &trap, &trapCount);
		
		CGContextSetRGBStrokeColor(c, 1,0,0,1);
		CGContextBeginPath(c);
		CGContextMoveToPoint(c, FixedToFloat(penX + trap.upperLeft.x),  FixedToFloat(penY + trap.lowerLeft.y));
		CGContextAddLineToPoint(c, FixedToFloat(penX + trap.upperRight.x),  FixedToFloat(penY + trap.lowerRight.y));
		CGContextAddLineToPoint(c, FixedToFloat(penX + trap.lowerRight.x),  FixedToFloat(penY + trap.upperRight.y));
		CGContextAddLineToPoint(c, FixedToFloat(penX + trap.lowerLeft.x),  FixedToFloat(penY + trap.upperLeft.y));
		CGContextClosePath(c);
		CGContextStrokePath(c);
		CGContextSetRGBStrokeColor(c, 0, 0, 1, 1);
		CGContextStrokeRect(c, CGRectMake(FixedToFloat(penX) + pixRect.left, FixedToFloat(penY) + pixRect.top, pixRect.right - pixRect.left, pixRect.bottom - pixRect.top));
		
		penY += GetLineHeight(layout, breaks[i], YES) + extraHeight;
	}
}

#if 0
static void GetImageBoundingBoxForLayout(ATSUTextLayout layout, UniCharArrayOffset *breaks, ItemCount breakCount, Fixed extraHeight, Fixed *lX, Fixed *lY, Fixed *height, Fixed *width)
{
	Rect largeRect = {0};
	ATSUTextMeasurement baseY = 0;
	int i;
	
	for (i = breakCount; i >= 0; i--) {
		UniCharArrayOffset end = breaks[i+1];
		Rect rect;
		
		ATSUMeasureTextImage(layout, breaks[i], end-breaks[i], 0, baseY, &rect);
		
		baseY += GetLineHeight(layout, breaks[i], YES) + extraHeight;
		
		if (i == breakCount) largeRect = rect;
		
		largeRect.bottom = MAX(largeRect.bottom, rect.bottom);
		largeRect.left = MIN(largeRect.left, rect.left);
		largeRect.top = MIN(largeRect.top, rect.top);
		largeRect.right = MAX(largeRect.right, rect.right);
	}
	
	
	if (lX) *lX = IntToFixed(largeRect.left);
	if (lY) *lY = IntToFixed(largeRect.bottom);
	*height = IntToFixed(largeRect.bottom - largeRect.top);
	*width = IntToFixed(largeRect.right - largeRect.left);
}
#endif

enum {fillc, strokec};

static void SetColor(CGContextRef c, int whichcolor, CGColorRef col)
{
	if (whichcolor == fillc) CGContextSetFillColorWithColor(c, col);
	else CGContextSetStrokeColorWithColor(c, col);
}

static void MakeRunVertical(ATSUTextLayout layout, UniCharArrayOffset spanOffset, UniCharArrayOffset length)
{
	ATSUStyle style, vStyle;
	ATSUVerticalCharacterType vertical = kATSUStronglyVertical;
	
	ATSUGetRunStyle(layout, spanOffset, &style, NULL, NULL);
	ATSUCreateAndCopyStyle(style, &vStyle);
	SetATSUStyleOther(vStyle, kATSUVerticalCharacterTag, sizeof(vertical), &vertical);
	ATSUSetRunStyle(layout, vStyle, spanOffset, length);
	ATSUDisposeStyle(vStyle);
}

static void EnableVerticalForSpan(ATSUTextLayout layout, SubRenderDiv *div, const unichar *ubuffer, UniCharArrayOffset spanOffset, UniCharArrayOffset length)
{
	const unichar tategakiLowerBound = 0x02F1; // copied from http://source.winehq.org/source/dlls/gdi32/freetype.c
	int runStart, runAboveBound, i;
	
	runStart = spanOffset;
	runAboveBound = ubuffer[spanOffset] >= tategakiLowerBound;
	
	for (i = spanOffset+1; i < spanOffset + length; i++) {
		int isAboveBound = ubuffer[i] >= tategakiLowerBound;
		
		if (isAboveBound != runAboveBound) {
			if (runAboveBound)
				MakeRunVertical(layout, runStart, i - runStart);
			runStart = i;
			isAboveBound = runAboveBound;
		}
	}
	
	if (runAboveBound)
		MakeRunVertical(layout, runStart, i - runStart);
}

static void SetStyleSpanRuns(ATSUTextLayout layout, SubRenderDiv *div, const unichar *ubuffer)
{
	unsigned span_count = [div->spans count];
	int i;
	
	for (i = 0; i < span_count; i++) {
		SubRenderSpan *span = [div->spans objectAtIndex:i];
		UniCharArrayOffset next = (i == span_count-1) ? [div->text length] : ((SubRenderSpan*)[div->spans objectAtIndex:i+1])->offset, spanLen = next - span->offset;
		SubATSUISpanEx *ex = span->ex;

		ATSUSetRunStyle(layout, ex->style, span->offset, spanLen);
		
		if (ex->vertical) {
			EnableVerticalForSpan(layout, div, ubuffer, span->offset, spanLen);
		}
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

static UniCharArrayOffset BreakOneLineSpan(ATSUTextLayout layout, SubRenderDiv *div, unsigned char *breakOpportunities,
										   ATSLayoutRecord *records, ItemCount lineLen, Fixed idealLineWidth, Fixed originalLineWidth, Fixed maximumLineWidth, unsigned numBreaks, unsigned lastHardBreak)
{		
	int recOffset = 0;
	Fixed widthOffset = 0;
	BOOL foundABreak;
	UniCharArrayOffset lastBreakOffset = 0;
	
	do {
		int j, lastIndex = 0;
		ATSUTextMeasurement error = 0;
		foundABreak = NO;
				
		for (j = recOffset; j < lineLen; j++) {
			ATSLayoutRecord *rec = &records[j];
			Fixed recPos = rec->realPos - widthOffset;
			UniCharArrayOffset charOffset = rec->originalOffset/2 + lastHardBreak;

			if (bitfield_test(breakOpportunities, charOffset)) {
				if (recPos >= idealLineWidth) {
					error = recPos - idealLineWidth;

					if (lastIndex) {
						Fixed lastError = abs((records[lastIndex].realPos - widthOffset) - idealLineWidth);
						if (lastError < error || div->wrapStyle == kSubLineWrapBottomWider) {
							rec = &records[lastIndex];
							j = lastIndex;
							recPos = rec->realPos - widthOffset;
							charOffset = rec->originalOffset/2 + lastHardBreak;
						}
					}
					
					// try not to leave short trailing lines
					if ((recPos + (originalLineWidth - rec->realPos)) < maximumLineWidth) return 0;
						
					foundABreak = YES;
					lastBreakOffset = charOffset;
					ATSUSetSoftLineBreak(layout, charOffset);
					break;
				}
				
				lastIndex = j;
			}
		}
		
		widthOffset = records[j].realPos;
		recOffset = j;
		numBreaks--;
	} while (foundABreak && numBreaks);
		
	return (numBreaks == 0) ? 0 : lastBreakOffset;
}

static void BreakLinesEvenly(ATSUTextLayout layout, SubRenderDiv *div, TextBreakLocatorRef breakLocator, Fixed breakingWidth, const unichar *utext, unsigned textLen, ItemCount numHardBreaks)
{
	UniCharArrayOffset hardBreaks[numHardBreaks+2];
	declare_bitfield(breakOpportunities, textLen);
	float fBreakingWidth = FixedToFloat(breakingWidth);
	int i;
	
	ATSUGetSoftLineBreaks(layout, kATSUFromTextBeginning, kATSUToTextEnd, numHardBreaks, &hardBreaks[1], NULL);	
	FindAllPossibleLineBreaks(breakLocator, utext, textLen, breakOpportunities);
		
	hardBreaks[0] = 0;
	hardBreaks[numHardBreaks+1] = textLen;
	
	for (i = 0; i <= numHardBreaks; i++) {
		UniCharArrayOffset thisBreak = hardBreaks[i], nextBreak = hardBreaks[i+1];
		ATSUTextMeasurement leftEdge, rightEdge, ignore;
		
		ATSUGetUnjustifiedBounds(layout, thisBreak, nextBreak - thisBreak, &leftEdge, &rightEdge, &ignore, &ignore);
		Fixed lineWidth = rightEdge - leftEdge;
		float fLineWidth = FixedToFloat(lineWidth);
				
		if (lineWidth > breakingWidth) {
			ATSLayoutRecord *records;
			ItemCount numRecords;
			unsigned idealSplitLines = ceil(fLineWidth / fBreakingWidth);
			Fixed idealBreakWidth = FloatToFixed(fLineWidth / idealSplitLines);
			
			ATSUDirectGetLayoutDataArrayPtrFromTextLayout(layout, thisBreak, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent, (void*)&records, &numRecords);
						
			UniCharArrayOffset res = BreakOneLineSpan(layout, div, breakOpportunities, records, numRecords, idealBreakWidth, lineWidth, breakingWidth, idealSplitLines-1, thisBreak);
			
			ATSUDirectReleaseLayoutDataArrayPtr(NULL, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent, (void*)&records);
			
			if (res) ATSUBatchBreakLines(layout, res, nextBreak - res, breakingWidth, NULL);
		}
	}
}

static UniCharArrayOffset *FindLineBreaks(ATSUTextLayout layout, SubRenderDiv *div, TextBreakLocatorRef breakLocator, UniCharArrayOffset *breaks, ItemCount *nbreaks, Fixed breakingWidth, const unichar *utext, unsigned textLen)
{
	ItemCount breakCount=0;
	
	switch (div->wrapStyle) {
		case kSubLineWrapSimple:
			ATSUBatchBreakLines(layout, kATSUFromTextBeginning, kATSUToTextEnd, breakingWidth, &breakCount);
			break;
		case kSubLineWrapTopWider:
		case kSubLineWrapBottomWider:
		case kSubLineWrapNone:
			SetLayoutPositioning(layout, positiveInfinity, kSubAlignmentLeft);	
			ATSUBatchBreakLines(layout, kATSUFromTextBeginning, kATSUToTextEnd, positiveInfinity, &breakCount);
			if (div->wrapStyle != kSubLineWrapNone) {
				BreakLinesEvenly(layout, div, breakLocator, breakingWidth, utext, textLen, breakCount);
				ATSUGetSoftLineBreaks(layout, kATSUFromTextBeginning, kATSUToTextEnd, 0, NULL, &breakCount);
			}
			SetLayoutPositioning(layout, breakingWidth, div->alignH);	
			break;
	}
		
	breaks = realloc(breaks, sizeof(UniCharArrayOffset) * (breakCount+2));
	ATSUGetSoftLineBreaks(layout, kATSUFromTextBeginning, kATSUToTextEnd, breakCount, &breaks[1], NULL);
	
	breaks[0] = 0;
	breaks[breakCount+1] = textLen;
	
	*nbreaks = breakCount;
	return breaks;
}

typedef struct {
	UniCharArrayOffset *breaks;
	ItemCount breakCount;
	int lStart, lEnd;
	SInt8 direction;
} BreakContext;

enum {kTextLayerShadow, kTextLayerOutline, kTextLayerPrimary};

static BOOL SetupCGForSpan(CGContextRef c, SubATSUISpanEx *spanEx, SubATSUISpanEx *lastSpanEx, SubRenderDiv *div, int textType, BOOL endLayer)
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
			if_different(outlineRadius) CGContextSetLineWidth(c, spanEx->outlineRadius ? (spanEx->outlineRadius*2. + .5) : 0.);
			if_different(outlineColor)  SetColor(c, (div->styleLine->borderStyle == kSubBorderStyleNormal) ? strokec : fillc, spanEx->outlineColor);
			
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

static void RenderActualLine(ATSUTextLayout layout, UniCharArrayOffset thisBreak, UniCharArrayOffset lineLen, Fixed penX, Fixed penY, CGContextRef c, SubRenderDiv *div, SubATSUISpanEx *spanEx, int textType)
{
	//ATS bug(?) with some fonts:
	//drawing \n draws some random other character, so skip them
	//FIXME: maybe don't store newlines in div->text at all
	if ([div->text characterAtIndex:thisBreak+lineLen-1] == '\n') {
		lineLen--;
		if (!lineLen) return;
	}

	if (textType == kTextLayerOutline && div->styleLine->borderStyle == kSubBorderStyleBox) {
		ATSUTextMeasurement lineWidth, lineHeight, lineX, lineY;
		UniCharArrayOffset breaks[2] = {thisBreak, thisBreak + lineLen};
		GetTypographicRectangleForLayout(layout, breaks, 0, FloatToFixed(spanEx->outlineRadius), &lineX, &lineY, &lineHeight, &lineWidth);
		
		CGRect borderRect = CGRectMake(FixedToFloat(lineX + penX), FixedToFloat(penY - lineY), FixedToFloat(lineWidth), FixedToFloat(lineHeight));
		
		ExpandCGRect(&borderRect, spanEx->outlineRadius);
		
		borderRect.origin.x = floor(borderRect.origin.x);
		borderRect.origin.y = floor(borderRect.origin.y);
		borderRect.size.width = ceil(borderRect.size.width);
		borderRect.size.height = ceil(borderRect.size.height);
		
		CGContextFillRect(c, borderRect);
	} else ATSUDrawText(layout, thisBreak, lineLen, RoundFixed(penX), RoundFixed(penY));
}

static Fixed DrawTextLines(CGContextRef c, ATSUTextLayout layout, SubRenderDiv *div, const BreakContext breakc, Fixed penX, Fixed penY, SubATSUISpanEx *firstSpanEx, int textType)
{
	int i;
	BOOL endLayer = NO;
	SubATSUISpanEx *lastSpanEx = nil;
	const CGTextDrawingMode textModes[] = {kCGTextFillStroke, kCGTextStroke, kCGTextFill};
		
	CGContextSetTextDrawingMode(c, textModes[textType]);
	
	if (!(div->render_complexity & renderMultipleParts)) endLayer = SetupCGForSpan(c, firstSpanEx, lastSpanEx, div, textType, endLayer);
	
	for (i = breakc.lStart; i != breakc.lEnd; i -= breakc.direction) {
		UniCharArrayOffset thisBreak = breakc.breaks[i], nextBreak = breakc.breaks[i+1], linelen = nextBreak - thisBreak;
		float extraHeight = 0;
		
		if (!(div->render_complexity & renderMultipleParts)) {
			RenderActualLine(layout, thisBreak, linelen, penX, penY, c, div, firstSpanEx, textType);
			extraHeight = div->styleLine->outlineRadius;
		} else {
			int j, nspans = [div->spans count];
			
			//linear search for the next span to draw
			//FIXME: make sure this never skips any spans
			for (j = 0; j < nspans; j++) {
				SubRenderSpan *span = [div->spans objectAtIndex:j];
				SubATSUISpanEx *spanEx = span->ex;
				UniCharArrayOffset spanLen, drawStart, drawLen;
				
				if (j < nspans-1) {
					SubRenderSpan *nextSpan = [div->spans objectAtIndex:j+1];
					spanLen = nextSpan->offset - span->offset;
				} else spanLen = [div->text length] - span->offset;
				
				if (span->offset < thisBreak) { // text spans a newline
					drawStart = thisBreak;
					drawLen = spanLen - (thisBreak - span->offset);
				} else {
					drawStart = span->offset;
					drawLen = MIN(spanLen, nextBreak - span->offset);
				}
				
				if (spanLen == 0 || drawLen == 0) continue;
				if ((span->offset + spanLen) < thisBreak) continue; // too early
				if (span->offset >= nextBreak) break; // too far ahead

				endLayer = SetupCGForSpan(c, spanEx, lastSpanEx, div, textType, endLayer);
				RenderActualLine(layout, drawStart, drawLen, (textType == kTextLayerShadow) ? (penX + FloatToFixed(spanEx->shadowDist)) : penX, 
														 (textType == kTextLayerShadow) ? (penY - FloatToFixed(spanEx->shadowDist)) : penY, c, div, spanEx, textType);
				extraHeight = MAX(extraHeight, spanEx->outlineRadius);
				lastSpanEx = spanEx;
			}
		
		}

		penY += breakc.direction * (GetLineHeight(layout, thisBreak, YES) + FloatToFixed(extraHeight));
	}
		
	if (endLayer) CGContextEndTransparencyLayer(c);

	return penY;
}

static Fixed DrawOneTextDiv(CGContextRef c, ATSUTextLayout layout, SubRenderDiv *div, const BreakContext breakc, Fixed penX, Fixed penY)
{
	SubATSUISpanEx *firstSpanEx = ((SubRenderSpan*)[div->spans objectAtIndex:0])->ex;
	BOOL endLayer = NO;
	
	if (div->styleLine->borderStyle == kSubBorderStyleNormal && firstSpanEx->shadowDist) {
		if (!(div->render_complexity & renderManualShadows)) {
			endLayer = YES;
			CGContextSetShadowWithColor(c, CGSizeMake(firstSpanEx->shadowDist + .5, -(firstSpanEx->shadowDist + .5)), 0, firstSpanEx->shadowColor);
			CGContextBeginTransparencyLayer(c, NULL);
		} else DrawTextLines(c, layout, div, breakc, penX, penY, firstSpanEx, kTextLayerShadow);
	}
	
	DrawTextLines(c, layout, div, breakc, penX, penY, firstSpanEx, kTextLayerOutline);
	penY = DrawTextLines(c, layout, div, breakc, penX, penY, firstSpanEx, kTextLayerPrimary);
	
	if (endLayer) {
		CGContextEndTransparencyLayer(c);
		CGContextSetShadowWithColor(c, CGSizeMake(0,0), 0, NULL);
	}
	
	return penY;
}

#pragma mark Main Renderer Function

-(void)renderPacket:(NSString *)packet inContext:(CGContextRef)c width:(float)cWidth height:(float)cHeight
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSArray *divs = SubParsePacket(packet, context, self);
	unsigned div_count = [divs count], lastLayer = 0;
	int i;
	Fixed bottomPen = 0, topPen = 0, centerPen = 0, *storePen=NULL;
	
	CGContextSaveGState(c);
	
	if (cWidth != videoWidth || cHeight != videoHeight) CGContextScaleCTM(c, cWidth / videoWidth, cHeight / videoHeight);
	SetATSULayoutOther(layout, kATSUCGContextTag, sizeof(CGContextRef), &c);
	
	CGContextSetLineCap(c, kCGLineCapRound); // these two are needed to avoid spiky outlines on some fonts
	CGContextSetLineJoin(c, kCGLineJoinRound);
	CGContextSetInterpolationQuality(c, kCGInterpolationLow);
	CGContextSetShouldSmoothFonts(c, NO); // disables subpixel AA which suddenly appeared in some version of 10.5
										  // note that it's not documented as doing this, but does the right thing anyway.
	
	for (i = 0; i < div_count; i++) {
		SubRenderDiv *div = [divs objectAtIndex:i];
		unsigned textLen = [div->text length];
		if (!textLen || ![div->spans count]) continue;
		BOOL resetPens = NO, resetGState = NO;
		NSData *ubufferData;
		const unichar *ubuffer = STUnicodeForString(div->text, &ubufferData);
		
		if (div->layer != lastLayer || div->shouldResetPens) {
			resetPens = YES;
			lastLayer = div->layer;
		}
		
		//NSLog(@"%@", div);
		
		NSRect marginRect = NSMakeRect(div->marginL, div->marginV, context->resX - div->marginL - div->marginR, context->resY - div->marginV - div->marginV);
		
		marginRect.origin.x *= screenScaleX;
		marginRect.origin.y *= screenScaleY;
		marginRect.size.width *= screenScaleX;
		marginRect.size.height *= screenScaleY;

		Fixed penY, penX, breakingWidth = FloatToFixed(marginRect.size.width);
		BreakContext breakc = {0}; ItemCount breakCount;
		
		ATSUSetTextPointerLocation(layout, ubuffer, kATSUFromTextBeginning, kATSUToTextEnd, textLen);		
		ATSUSetTransientFontMatching(layout,TRUE);
		
		SetLayoutPositioning(layout, breakingWidth, div->alignH);
		SetStyleSpanRuns(layout, div, ubuffer);
		
		breakbuffer = FindLineBreaks(layout, div, breakLocator, breakbuffer, &breakCount, breakingWidth, ubuffer, textLen);

		ATSUTextMeasurement imageWidth, imageHeight, descent;
		UniCharArrayOffset *breaks = breakbuffer;
		
		if (div->positioned || div->alignV == kSubAlignmentMiddle)
			GetTypographicRectangleForLayout(layout, breaks, breakCount, FloatToFixed(div->styleLine->outlineRadius), NULL, NULL, &imageHeight, &imageWidth);
		
		if (div->positioned || div->alignV == kSubAlignmentBottom)
			ATSUGetLineControl(layout, kATSUFromTextBeginning, kATSULineDescentTag, sizeof(ATSUTextMeasurement), &descent, NULL);
		
#if 0
		{
			ATSUTextMeasurement ascent, descent;
			
			ATSUGetLineControl(layout, kATSUFromTextBeginning, kATSULineAscentTag,  sizeof(ATSUTextMeasurement), &ascent,  NULL);
			ATSUGetLineControl(layout, kATSUFromTextBeginning, kATSULineDescentTag, sizeof(ATSUTextMeasurement), &descent, NULL);
			
			NSLog(@"\"%@\" descent %f ascent %f\n", div->text, FixedToFloat(descent), FixedToFloat(ascent));
		}
#endif
		
		if (!div->positioned) {
			penX = FloatToFixed(NSMinX(marginRect));

			switch(div->alignV) {
				case kSubAlignmentBottom: default:
					if (!bottomPen || resetPens) {
						penY = FloatToFixed(NSMinY(marginRect)) + descent;
					} else penY = bottomPen;
					
					storePen = &bottomPen; breakc.lStart = breakCount; breakc.lEnd = -1; breakc.direction = 1;
					break;
				case kSubAlignmentMiddle:
					if (!centerPen || resetPens) {
						penY = FloatToFixed(NSMidY(marginRect)) - (imageHeight / 2) + descent;
					} else penY = centerPen;
					
					storePen = &centerPen; breakc.lStart = breakCount; breakc.lEnd = -1; breakc.direction = 1;
					break;
				case kSubAlignmentTop:
					if (!topPen || resetPens) {
						penY = FloatToFixed(NSMaxY(marginRect)) - GetLineHeight(layout, kATSUFromTextBeginning, NO);
					} else penY = topPen;
					
					storePen = &topPen; breakc.lStart = 0; breakc.lEnd = breakCount+1; breakc.direction = -1;
					break;
			}
		} else {
			penX = FloatToFixed(div->posX * screenScaleX);
			penY = FloatToFixed((context->resY - div->posY) * screenScaleY);
			
			switch (div->alignH) {
				case kSubAlignmentCenter: penX -= imageWidth / 2; break;
				case kSubAlignmentRight: penX -= imageWidth;
			}
			
			switch (div->alignV) {
				case kSubAlignmentMiddle: penY -= imageHeight / 2; break;
				case kSubAlignmentTop: penY -= imageHeight; break;
			}
			
			penY += descent;

			SetLayoutPositioning(layout, imageWidth, div->alignH);
			storePen = NULL; breakc.lStart = breakCount; breakc.lEnd = -1; breakc.direction = 1;
		}
		
		SubATSUISpanEx *firstspan = ((SubRenderSpan*)[div->spans objectAtIndex:0])->ex;

		// FIXME: we can only rotate an entire line at once
		if (firstspan->angle) {
			Fixed fangle = FloatToFixed(firstspan->angle);
			SetATSULayoutOther(layout, kATSULineRotationTag, sizeof(Fixed), &fangle);
			
			// FIXME: awful hack for SSA vertical text idiom
			// instead it needs to rotate text by hand or actually fix ATSUI's rotation origin
			if (firstspan->vertical && 
				div->alignV == kSubAlignmentMiddle && div->alignH == kSubAlignmentCenter) {
				CGContextSaveGState(c);
				CGContextTranslateCTM(c, FixedToFloat(imageWidth)/2, FixedToFloat(imageWidth)/2);
				resetGState = YES;
			}
		}
		
		if (drawTextBounds)
			VisualizeLayoutLineHeights(c, layout, breaks, breakCount, FloatToFixed(div->styleLine->outlineRadius), penX, penY, cHeight);

		breakc.breakCount = breakCount;
		breakc.breaks = breaks;
		
		penY = DrawOneTextDiv(c, layout, div, breakc, penX, penY);
		
		if (resetGState)
			CGContextRestoreGState(c);
		
		[ubufferData release];
		if (storePen) *storePen = penY;
	}
	
	CGContextRestoreGState(c);
	[pool release];
}

SubtitleRendererPtr SubInitSSA(char *header, size_t headerLen, int width, int height)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *hdr = [[NSString alloc] initWithBytesNoCopy:(void*)header length:headerLen encoding:NSUTF8StringEncoding freeWhenDone:NO];

	SubtitleRendererPtr s = [[SubATSUIRenderer alloc] initWithSSAHeader:hdr videoWidth:width videoHeight:height];
	[hdr release];
	CFRetain(s);
	[pool release];
	return s;
}

SubtitleRendererPtr SubInitNonSSA(int width, int height)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	SubtitleRendererPtr s = [[SubATSUIRenderer alloc] initWithVideoWidth:width videoHeight:height];
	CFRetain(s);
	[pool release];
	return s;
}

void SubRenderPacket(SubtitleRendererPtr s, CGContextRef c, CFStringRef str, int cWidth, int cHeight)
{
	[s renderPacket:(NSString*)str inContext:c width:cWidth height:cHeight];
}

void SubPrerollFromHeader(char *header, int headerLen)
{
	SubtitleRendererPtr s = headerLen ? SubInitSSA(header, headerLen, 640, 480)
								      : SubInitNonSSA(640, 480);
	/*
	CGColorSpaceRef csp = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	void *buf = malloc(640 * 480 * 4);
	CGContextRef c = CGBitmapContextCreate(buf,640,480,8,640 * 4,csp,kCGImageAlphaPremultipliedFirst);
	
	if (!headerLen) {
		SubRenderPacket(s, c, (CFStringRef)@"Abcde .", 640, 480);
	} else {
		NSArray *styles = [s->context->styles allValues];
		int i, nstyles = [styles count];
		
		for (i = 0; i < nstyles; i++) {
			SubStyle *sty = [styles objectAtIndex:i];
			NSString *line = [NSString stringWithFormat:@"0,0,%@,,0,0,0,,Abcde .", sty->name];
			SubRenderPacket(s, c, (CFStringRef)line, 640, 480);
		}
	}
	
	CGContextRelease(c);
	free(buf);
	CGColorSpaceRelease(csp);
	*/
	SubDisposeRenderer(s);
}

void SubDisposeRenderer(SubtitleRendererPtr s)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	CFRelease(s);
	[s release];
	[pool release];
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
// Some Windows fonts have one field set incorrectly(?), so we have to compensate.
// FIXME: This function doesn't read from the right fonts; if we're using italic variant, it should get the ATSFontRef for that
// This should be cached
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

static void FindAllPossibleLineBreaks(TextBreakLocatorRef breakLocator, const unichar *uline, UniCharArrayOffset lineLen, unsigned char *breakOpportunities)
{
	UniCharArrayOffset lastBreak = 0;
	
	while (1) {
		UniCharArrayOffset breakOffset = 0;
		OSStatus status;
		
		status = UCFindTextBreak(breakLocator, kUCTextBreakLineMask, kUCTextBreakLeadingEdgeMask | (lastBreak ? kUCTextBreakIterateMask : 0), uline, lineLen, lastBreak, &breakOffset);
		
		if (status != noErr || breakOffset >= lineLen) break;
		
		bitfield_set(breakOpportunities, breakOffset-1);
		lastBreak = breakOffset;
	}
}
