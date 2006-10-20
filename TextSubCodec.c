
#if __MACH__
    #include <Carbon/Carbon.h>
    #include <QuickTime/QuickTime.h>
#else
    #include <ConditionalMacros.h>
    #include <Endian.h>
    #include <ImageCodec.h>
#endif

#include "TextSubCodec.h"

#define SubFontName "Helvetica Neue Condensed Bold"
// basically random numbers i picked, font size is a factor of the diagonal of the movie window, border is a factor of that
#define FontSizeRatio .035
#define BorderSizeRatio FontSizeRatio * .045

// Constants
const UInt8 kNumPixelFormatsSupportedTextSub = 1;

// Data structures
typedef struct	{
	ComponentInstance		self;
	ComponentInstance		delegateComponent;
	ComponentInstance		target;
	OSType**				wantedDestinationPixelTypeH;
	ImageCodecMPDrawBandUPP drawBandUPP;
	
	CGColorSpaceRef         colorSpace;
	
	ATSUStyle				textStyle;
	ATSUTextLayout			textLayout;
	//Ptr                     textBuffer;
} TextSubGlobalsRecord, *TextSubGlobals;

typedef struct {
	long		width;
	long		height;
	long		depth;
    long        dataSize;
    
    //CGContextRef    quartzContext;
    // pointer to the data the context currently is drawing into
    Ptr             baseAddr;
} TextSubDecompressRecord;

// Setup required for ComponentDispatchHelper.c
#define IMAGECODEC_BASENAME() 		TextSubCodec
#define IMAGECODEC_GLOBALS() 		TextSubGlobals storage

#define CALLCOMPONENT_BASENAME()	IMAGECODEC_BASENAME()
#define	CALLCOMPONENT_GLOBALS()		IMAGECODEC_GLOBALS()

#define COMPONENT_UPP_PREFIX()		uppImageCodec
#define COMPONENT_DISPATCH_FILE		"TextSubCodecDispatch.h"
#define COMPONENT_SELECT_PREFIX()  	kImageCodec

#define	GET_DELEGATE_COMPONENT()	(storage->delegateComponent)


#if __MACH__
	#include <CoreServices/Components.k.h>
	#include <QuickTime/ImageCodec.k.h>
	#include <QuickTime/ComponentDispatchHelper.c>
#else
	#include <Components.k.h>
	#include <ImageCodec.k.h>
	#include <ComponentDispatchHelper.c>
#endif
	
/* -- This Image Decompressor User the Base Image Decompressor Component --
	The base image decompressor is an Apple-supplied component
	that makes it easier for developers to create new decompressors.
	The base image decompressor does most of the housekeeping and
	interface functions required for a QuickTime decompressor component,
	including scheduling for asynchronous decompression.
*/

// Component Open Request - Required
pascal ComponentResult TextSubCodecOpen(TextSubGlobals glob, ComponentInstance self)
{
	ComponentResult err;

	// Allocate memory for our globals, set them up and inform the component manager that we've done so
	glob = (TextSubGlobals)NewPtrClear(sizeof(TextSubGlobalsRecord));
	if (err = MemError()) goto bail;

	SetComponentInstanceStorage(self, (Handle)glob);

	glob->self = self;
	glob->target = self;
	glob->wantedDestinationPixelTypeH = (OSType **)NewHandle(sizeof(OSType) * (kNumPixelFormatsSupportedTextSub + 1));
	if (err = MemError()) goto bail;
	glob->drawBandUPP = NULL;
	
	// Open and target an instance of the base decompressor as we delegate
	// most of our calls to the base decompressor instance
	err = OpenADefaultComponent(decompressorComponentType, kBaseCodecType, &glob->delegateComponent);
	if (err) goto bail;

	ComponentSetTarget(glob->delegateComponent, self);

bail:
	return err;
}

// Component Close Request - Required
pascal ComponentResult TextSubCodecClose(TextSubGlobals glob, ComponentInstance self)
{
	// Make sure to close the base component and dealocate our storage
	if (glob) {
		if (glob->delegateComponent) {
			CloseComponent(glob->delegateComponent);
		}
		if (glob->wantedDestinationPixelTypeH) {
			DisposeHandle((Handle)glob->wantedDestinationPixelTypeH);
		}
		if (glob->drawBandUPP) {
			DisposeImageCodecMPDrawBandUPP(glob->drawBandUPP);
		}
		if (glob->colorSpace) {
			CGColorSpaceRelease(glob->colorSpace);
		}
		if (glob->textStyle) {
			ATSUDisposeStyle(glob->textStyle);
		}
		if (glob->textLayout) {
			ATSUDisposeTextLayout(glob->textLayout);
		}
		DisposePtr((Ptr)glob);
	}

	return noErr;
}

// Component Version Request - Required
pascal ComponentResult TextSubCodecVersion(TextSubGlobals glob)
{
#pragma unused(glob)
	
    return kTextSubCodecVersion;
}

// Component Target Request
// 		Allows another component to "target" you i.e., you call another component whenever
// you would call yourself (as a result of your component being used by another component)
pascal ComponentResult TextSubCodecTarget(TextSubGlobals glob, ComponentInstance target)
{
	glob->target = target;
	return noErr;
}

// Component GetMPWorkFunction Request
//		Allows your image decompressor component to perform asynchronous decompression
// in a single MP task by taking advantage of the Base Decompressor. If you implement
// this selector, your DrawBand function must be MP-safe. MP safety means not
// calling routines that may move or purge memory and not calling any routines which
// might cause 68K code to be executed.
pascal ComponentResult TextSubCodecGetMPWorkFunction(TextSubGlobals glob, ComponentMPWorkFunctionUPP *workFunction, void **refCon)
{
	if (NULL == glob->drawBandUPP)
		glob->drawBandUPP = NewImageCodecMPDrawBandUPP((ImageCodecMPDrawBandProcPtr)TextSubCodecDrawBand);
		
	return ImageCodecGetBaseMPWorkFunction(glob->delegateComponent, workFunction, refCon, glob->drawBandUPP, glob);
}

#pragma mark-

// ImageCodecInitialize
//		The first function call that your image decompressor component receives from the base image
// decompressor is always a call to ImageCodecInitialize . In response to this call, your image decompressor
// component returns an ImageSubCodecDecompressCapabilities structure that specifies its capabilities.
pascal ComponentResult TextSubCodecInitialize(TextSubGlobals glob, ImageSubCodecDecompressCapabilities *cap)
{
#pragma unused(glob)

	// Secifies the size of the ImageSubCodecDecompressRecord structure
	// and say we can support asyncronous decompression
	// With the help of the base image decompressor, any image decompressor
	// that uses only interrupt-safe calls for decompression operations can
	// support asynchronous decompression.
	cap->decompressRecordSize = sizeof(TextSubDecompressRecord);
	cap->canAsync = true;
	
	cap->subCodecIsMultiBufferAware = true;

	return noErr;
}

// ImageCodecPreflight
// 		The base image decompressor gets additional information about the capabilities of your image
// decompressor component by calling ImageCodecPreflight. The base image decompressor uses this
// information when responding to a call to the ImageCodecPredecompress function,
// which the ICM makes before decompressing an image. You are required only to provide values for
// the wantedDestinationPixelSize and wantedDestinationPixelTypes fields and can also modify other
// fields if necessary.
pascal ComponentResult TextSubCodecPreflight(TextSubGlobals glob, CodecDecompressParams *p)
{
	CodecCapabilities *capabilities = p->capabilities;
	OSTypePtr         formats = *glob->wantedDestinationPixelTypeH;

	// Fill in formats for wantedDestinationPixelTypeH
	// Terminate with an OSType value 0  - see IceFloe #7
	// http://developer.apple.com/quicktime/icefloe/dispatch007.html
    
    // we want ARGB because Quartz can use it easily
    // Todo: add other possible pixel formats Quartz can handle
    *formats++	= k32ARGBPixelFormat;
	*formats++	= 0;
	
	// Specify the minimum image band height supported by the component
	// bandInc specifies a common factor of supported image band heights - 
	// if your component supports only image bands that are an even
    // multiple of some number of pixels high report this common factor in bandInc
	capabilities->bandMin = (**p->imageDescription).height;
	capabilities->bandInc = capabilities->bandMin;

	// Indicate the wanted destination using the wantedDestinationPixelTypeH previously set up
	capabilities->wantedPixelSize  = 0; 	
    **glob->wantedDestinationPixelTypeH = k32ARGBPixelFormat;
    
	p->wantedDestinationPixelTypes = glob->wantedDestinationPixelTypeH;

	// Specify the number of pixels the image must be extended in width and height if
	// the component cannot accommodate the image at its given width and height
	capabilities->extendWidth = 0;
	capabilities->extendHeight = 0;
	
	if (glob->colorSpace == NULL)
		glob->colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	
	return noErr;
}

// ImageCodecBeginBand
// 		The ImageCodecBeginBand function allows your image decompressor component to save information about
// a band before decompressing it. This function is never called at interrupt time. The base image decompressor
// preserves any changes your component makes to any of the fields in the ImageSubCodecDecompressRecord
// or CodecDecompressParams structures. If your component supports asynchronous scheduled decompression, it
// may receive more than one ImageCodecBeginBand call before receiving an ImageCodecDrawBand call.
pascal ComponentResult TextSubCodecBeginBand(TextSubGlobals glob, CodecDecompressParams *p, ImageSubCodecDecompressRecord *drp, long flags)
{
	long offsetH, offsetV;
	TextSubDecompressRecord *myDrp = (TextSubDecompressRecord *)drp->userDecompressRecord;

	offsetH = (long)(p->dstRect.left - p->dstPixMap.bounds.left) * (long)(p->dstPixMap.pixelSize >> 3);
	offsetV = (long)(p->dstRect.top - p->dstPixMap.bounds.top) * (long)drp->rowBytes;

	drp->baseAddr = p->dstPixMap.baseAddr + offsetH + offsetV;
	
	// Let base codec know that all our frames are key frames (a.k.a., sync samples)
	// This allows the base codec to perform frame dropping on our behalf if needed 
    drp->frameType = kCodecFrameTypeKey;

	myDrp->width = (**p->imageDescription).width;
	myDrp->height = (**p->imageDescription).height;
	myDrp->depth = (**p->imageDescription).depth;
    myDrp->dataSize = p->bufferSize;
	
	return noErr;
}

// ImageCodecDrawBand
//		The base image decompressor calls your image decompressor component's ImageCodecDrawBand function
// to decompress a band or frame. Your component must implement this function. If the ImageSubCodecDecompressRecord
// structure specifies a progress function or data-loading function, the base image decompressor will never call ImageCodecDrawBand
// at interrupt time. If the ImageSubCodecDecompressRecord structure specifies a progress function, the base image decompressor
// handles codecProgressOpen and codecProgressClose calls, and your image decompressor component must not implement these functions.
// If not, the base image decompressor may call the ImageCodecDrawBand function at interrupt time.
// When the base image decompressor calls your ImageCodecDrawBand function, your component must perform the decompression specified
// by the fields of the ImageSubCodecDecompressRecord structure. The structure includes any changes your component made to it
// when performing the ImageCodecBeginBand function. If your component supports asynchronous scheduled decompression,
// it may receive more than one ImageCodecBeginBand call before receiving an ImageCodecDrawBand call.
pascal ComponentResult TextSubCodecDrawBand(TextSubGlobals glob, ImageSubCodecDecompressRecord *drp)
{
	OSErr err = noErr;
	int i;
	TextSubDecompressRecord *myDrp = (TextSubDecompressRecord *)drp->userDecompressRecord;
	float diagonalLength = sqrtf(myDrp->width*myDrp->width+myDrp->height*myDrp->height);
	ATSUTextMeasurement lineWidth = Long2Fix(myDrp->width), lineHeight = Long2Fix(20);
//	char *dataPtr = (char *)drp->codecData;
//	ICMDataProcRecordPtr dataProc = drp->dataProcRecord.dataProc ? &drp->dataProcRecord : NULL;
	
    CGContextRef c = CGBitmapContextCreate(drp->baseAddr, myDrp->width, myDrp->height,
										   8, drp->rowBytes,  glob->colorSpace,
										   kCGImageAlphaPremultipliedFirst);
	
	if (!glob->textStyle) { //TODO: set language region based on track language... does that even matter?
		ATSUAttributeTag tags[] = {kATSUSizeTag, kATSURGBAlphaColorTag, kATSUStyleRenderingOptionsTag, kATSUFontTag, kATSUQDBoldfaceTag};
		ByteCount		 sizes[] = {sizeof(Fixed), sizeof(ATSURGBAlphaColor), sizeof(ATSStyleRenderingOptions), sizeof(ATSUFontID), sizeof(Boolean)};
		Fixed			 size = X2Fix(diagonalLength * FontSizeRatio); ATSURGBAlphaColor yellow = {.9,.9,.1,1};
		Boolean			 trueval = TRUE; ATSUFontID fid; ATSStyleRenderingOptions rend = kATSStyleApplyAntiAliasing;
		ATSUAttributeValuePtr vals[] = {&size,&yellow,&rend,&fid,&trueval};
		ATSUCreateStyle(&glob->textStyle);
		ATSUFindFontFromName(SubFontName,strlen(SubFontName),kFontFullName,kFontNoPlatform,kFontNoScript,kFontNoLanguage,&fid);
		ATSUSetAttributes(glob->textStyle,5, tags, sizes, vals);
	}
	
	if (!glob->textLayout) {
		ATSUAttributeTag tags[] = {kATSULineFlushFactorTag, kATSULineWidthTag};
		ByteCount		 sizes[] = {sizeof(Fract),sizeof(ATSUTextMeasurement)};
		Fract			 lf = kATSUCenterAlignment; ATSUTextMeasurement w = Long2Fix(myDrp->width);
		ATSUAttributeValuePtr vals[] = {&lf, &w};
		ATSUCreateTextLayout(&glob->textLayout);
		ATSUSetLayoutControls(glob->textLayout, 2, tags, sizes, vals);
	}
	
	// QuickTime doesn't like it if we complain too much
    if (!c || !glob->textStyle)
		return noErr;

	char textBuffer[myDrp->dataSize + 1];
	
	memcpy(textBuffer, drp->codecData, myDrp->dataSize);
	textBuffer[myDrp->dataSize] = '\0';

	ATSUAttributeTag cgc[] = {kATSUCGContextTag};
	ByteCount cgc_s[] = {sizeof(CGContextRef)};
	ATSUAttributeValuePtr cgc_v[] = {&c};
	
	CFStringRef cfsub = CFStringCreateWithCString(NULL, textBuffer, kCFStringEncodingUTF8);
	int sublen = CFStringGetLength(cfsub);
	
	UniChar		uc[sublen];
	ItemCount	breakCount;
	
	ATSUSetLayoutControls(glob->textLayout, 1, cgc, cgc_s, cgc_v);
	
	CFStringGetCharacters(cfsub, CFRangeMake(0, sublen), uc);
	ATSUSetTextPointerLocation(glob->textLayout,uc,kATSUFromTextBeginning,kATSUToTextEnd,sublen);
	
	ATSUSetRunStyle(glob->textLayout,glob->textStyle,kATSUFromTextBeginning,kATSUToTextEnd);
	ATSUSetTransientFontMatching(glob->textLayout,TRUE); // auto-match fonts for other scripts
														 // TODO: make it match boldface fonts instead of regular
	
	ATSUBatchBreakLines(glob->textLayout,kATSUFromTextBeginning,kATSUToTextEnd,lineWidth,&breakCount); // line wrapping
	ATSUGetSoftLineBreaks(glob->textLayout,kATSUFromTextBeginning,kATSUToTextEnd,0,NULL,&breakCount);
	UniCharArrayOffset breaks[breakCount+2]; // 0 = beginning, 1...n-1 automatic line breaks, n end of text
	ATSUGetSoftLineBreaks(glob->textLayout,kATSUFromTextBeginning,kATSUToTextEnd,breakCount,&breaks[1],&breakCount);
	
	breaks[0] = 0;
	breaks[breakCount+1] = kATSUToTextEnd;
	
	CGContextClearRect(c, CGRectMake(0, 0, myDrp->width, myDrp->height));
	CGContextSetRGBStrokeColor(c, 0,0,0,1);
	CGContextSetTextDrawingMode(c, kCGTextFillStroke);
	CGContextSetLineWidth(c, diagonalLength * BorderSizeRatio);

	for (i = breakCount; i >= 0; i--) {
		int end = breaks[i+1];
		if (end == kATSUToTextEnd) end=sublen;
		ATSUDrawText(glob->textLayout, breaks[i], end-breaks[i], Long2Fix(0), lineHeight);

		ATSUTextMeasurement ascent, descent; ByteCount unused;
		ATSUGetLineControl(glob->textLayout, breaks[i], kATSULineAscentTag, sizeof(ATSUTextMeasurement), &ascent, &unused);
		ATSUGetLineControl(glob->textLayout, breaks[i], kATSULineDescentTag, sizeof(ATSUTextMeasurement), &descent, &unused);
		lineHeight += ascent + descent;
	}
	
	CFRelease(cfsub);
	CGContextSynchronize(c);
	CGContextRelease(c);
	
	return err;
}

// ImageCodecEndBand
//		The ImageCodecEndBand function notifies your image decompressor component that decompression of a band has finished or
// that it was terminated by the Image Compression Manager. Your image decompressor component is not required to implement
// the ImageCodecEndBand function. The base image decompressor may call the ImageCodecEndBand function at interrupt time.
// After your image decompressor component handles an ImageCodecEndBand call, it can perform any tasks that are required
// when decompression is finished, such as disposing of data structures that are no longer needed. Because this function
// can be called at interrupt time, your component cannot use this function to dispose of data structures; this
// must occur after handling the function. The value of the result parameter should be set to noErr if the band or frame was
// drawn successfully. If it is any other value, the band or frame was not drawn.
pascal ComponentResult TextSubCodecEndBand(TextSubGlobals glob, ImageSubCodecDecompressRecord *drp, OSErr result, long flags)
{
#pragma unused(glob, drp,result, flags)
	
	return noErr;
}

// ImageCodecQueueStarting
// 		If your component supports asynchronous scheduled decompression, the base image decompressor calls your image decompressor component's
// ImageCodecQueueStarting function before decompressing the frames in the queue. Your component is not required to implement this function.
// It can implement the function if it needs to perform any tasks at this time, such as locking data structures.
// The base image decompressor never calls the ImageCodecQueueStarting function at interrupt time.
pascal ComponentResult TextSubCodecQueueStarting(TextSubGlobals glob)
{
#pragma unused(glob)
	
	return noErr;
}

// ImageCodecQueueStopping
//		 If your image decompressor component supports asynchronous scheduled decompression, the ImageCodecQueueStopping function notifies
// your component that the frames in the queue have been decompressed. Your component is not required to implement this function.
// After your image decompressor component handles an ImageCodecQueueStopping call, it can perform any tasks that are required when decompression
// of the frames is finished, such as disposing of data structures that are no longer needed. 
// The base image decompressor never calls the ImageCodecQueueStopping function at interrupt time.
pascal ComponentResult TextSubCodecQueueStopping(TextSubGlobals glob)
{
#pragma unused(glob)
	
	return noErr;
}

// ImageCodecGetCompressedImageSize
// 		Your component receives the ImageCodecGetCompressedImageSize request whenever an application calls the ICM's GetCompressedImageSize function.
// You can use the ImageCodecGetCompressedImageSize function when you are extracting a single image from a sequence; therefore, you don't have an
// image description structure and don't know the exact size of one frame. In this case, the Image Compression Manager calls the component to determine
// the size of the data. Your component should return a long integer indicating the number of bytes of data in the compressed image. You may want to store
// the image size somewhere in the image description structure, so that you can respond to this request quickly. Only decompressors receive this request.
pascal ComponentResult TextSubCodecGetCompressedImageSize(TextSubGlobals glob, ImageDescriptionHandle desc, Ptr data, long dataSize, ICMDataProcRecordPtr dataProc, long *size)
{
#pragma	unused(glob,dataProc,desc)
	if (size == NULL) 
		return paramErr;

//	*size = EndianU32_BtoN(dataSize);

	return noErr;
}

// ImageCodecGetCodecInfo
//		Your component receives the ImageCodecGetCodecInfo request whenever an application calls the Image Compression Manager's GetCodecInfo function.
// Your component should return a formatted compressor information structure defining its capabilities.
// Both compressors and decompressors may receive this request.
pascal ComponentResult TextSubCodecGetCodecInfo(TextSubGlobals glob, CodecInfo *info)
{
	OSErr err = noErr;

	if (info == NULL) {
		err = paramErr;
	} else {
		CodecInfo **tempCodecInfo;

		err = GetComponentResource((Component)glob->self, codecInfoResourceType, kTextSubCodecResource, (Handle *)&tempCodecInfo);
		if (err == noErr) {
			*info = **tempCodecInfo;
			DisposeHandle((Handle)tempCodecInfo);
		}
	}

	return err;
}
