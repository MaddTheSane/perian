/*
 * TextSubCodec.c
 * Created by David Conrad on 3/21/06.
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

#if __MACH__
    #include <Carbon/Carbon.h>
    #include <QuickTime/QuickTime.h>
#else
    #include <ConditionalMacros.h>
    #include <Endian.h>
    #include <ImageCodec.h>
#endif

#include "PerianResourceIDs.h"
#include "SubATSUIRenderer.h"

// Data structures
typedef struct TextSubGlobalsRecord {
	ComponentInstance		self;
	ComponentInstance		delegateComponent;
	ComponentInstance		target;
	OSType**				wantedDestinationPixelTypeH;
	ImageCodecMPDrawBandUPP drawBandUPP;
	
	CGColorSpaceRef         colorSpace;

	SubtitleRendererPtr		ssa;
	Boolean					translateSRT;
} TextSubGlobalsRecord, *TextSubGlobals;

typedef struct {
	long		width;
	long		height;
	long		depth;
    long        dataSize;
    
    Ptr             baseAddr;
	OSType			pixelFormat;
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

#define kNumPixelFormatsSupportedTextSub 2

static CFMutableStringRef CFStringCreateMutableWithBytes(CFAllocatorRef alloc, char *cStr, size_t size, CFStringEncoding encoding) {
	CFStringRef                s1 = CFStringCreateWithBytes(alloc,(UInt8*)cStr,size,encoding,false);
	if (!s1) return NULL;
	CFMutableStringRef s2 = CFStringCreateMutableCopy(alloc,0,s1);
	CFRelease(s1);
	return s2;
}

/* -- This Image Decompressor Uses the Base Image Decompressor Component --
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
	glob->wantedDestinationPixelTypeH = (OSType **)NewHandleClear((kNumPixelFormatsSupportedTextSub+1) * sizeof(OSType));
	if (err = MemError()) goto bail;
	glob->drawBandUPP = NULL;
	glob->ssa = NULL;
	glob->colorSpace = NULL;
	glob->translateSRT = true;
	
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
		if (glob->ssa) SubDisposeRenderer(glob->ssa);
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
	cap->subCodecSupportsDecodeSmoothing = true;

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
    *formats++	= k32RGBAPixelFormat;
	*formats++	= k32ARGBPixelFormat;
	
	// Specify the minimum image band height supported by the component
	// bandInc specifies a common factor of supported image band heights - 
	// if your component supports only image bands that are an even
    // multiple of some number of pixels high report this common factor in bandInc
	capabilities->bandMin = (**p->imageDescription).height;
	capabilities->bandInc = capabilities->bandMin;

	// Indicate the wanted destination using the wantedDestinationPixelTypeH previously set up
	capabilities->wantedPixelSize  = 0; 	
    
	p->wantedDestinationPixelTypes = glob->wantedDestinationPixelTypeH;

	// Specify the number of pixels the image must be extended in width and height if
	// the component cannot accommodate the image at its given width and height
	capabilities->extendWidth = 0;
	capabilities->extendHeight = 0;
	
	capabilities->flags |= codecCanAsync | codecCanAsyncWhen | codecCanScale;
	capabilities->flags2 |= codecDrawsHigherQualityScaled;    
	
	if (!glob->ssa) {
		if ((**p->imageDescription).cType == kSubFormatSSA) {
			long count;
			glob->translateSRT = false;
			
			CountImageDescriptionExtensionType(p->imageDescription,kSubFormatSSA,&count);
			if (count == 1) {
				Handle ssaheader;
				GetImageDescriptionExtension(p->imageDescription,&ssaheader,kSubFormatSSA,1);
				
				glob->ssa = SubInitSSA(*ssaheader, GetHandleSize(ssaheader), (**p->imageDescription).width, (**p->imageDescription).height);
			} 
		} 
		
		if (!glob->ssa) glob->ssa = SubInitNonSSA((**p->imageDescription).width,(**p->imageDescription).height);
				
		glob->colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	}
	
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
	TextSubDecompressRecord *myDrp = (TextSubDecompressRecord *)drp->userDecompressRecord;
	
	// Let base codec know that all our frames are key frames (a.k.a., sync samples)
	// This allows the base codec to perform frame dropping on our behalf if needed 
    drp->frameType = kCodecFrameTypeKey;

	myDrp->pixelFormat = p->dstPixMap.pixelFormat;
	myDrp->width = p->dstRect.right - p->dstRect.left;
	myDrp->height = p->dstRect.bottom - p->dstRect.top;
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
	TextSubDecompressRecord *myDrp = (TextSubDecompressRecord *)drp->userDecompressRecord;
	CGImageAlphaInfo alphaFormat = (myDrp->pixelFormat == k32ARGBPixelFormat) ? kCGImageAlphaPremultipliedFirst : kCGImageAlphaPremultipliedLast;

    CGContextRef c = CGBitmapContextCreate(drp->baseAddr, myDrp->width, myDrp->height,
										   8, drp->rowBytes,  glob->colorSpace,
										   alphaFormat);
	
	CGContextClearRect(c, CGRectMake(0,0, myDrp->width, myDrp->height));
	
	CFMutableStringRef buf;
	
	if (drp->codecData[0] == '\n' && myDrp->dataSize == 1) goto leave; // skip empty packets
	
	if (glob->translateSRT) {
		buf = CFStringCreateMutableWithBytes(NULL, drp->codecData, myDrp->dataSize, kCFStringEncodingUTF8);
		if (!buf) goto leave;
		CFStringFindAndReplace(buf, CFSTR("<i>"),  CFSTR("{\\i1}"), CFRangeMake(0,CFStringGetLength(buf)), 0);
		CFStringFindAndReplace(buf, CFSTR("</i>"), CFSTR("{\\i0}"), CFRangeMake(0,CFStringGetLength(buf)), 0);
		CFStringFindAndReplace(buf, CFSTR("<"),    CFSTR("{"),      CFRangeMake(0,CFStringGetLength(buf)), 0);
		CFStringFindAndReplace(buf, CFSTR(">"),    CFSTR("}"),      CFRangeMake(0,CFStringGetLength(buf)), 0);
	} else {
		buf = (CFMutableStringRef)CFStringCreateWithBytes(NULL, (UInt8*)drp->codecData, myDrp->dataSize, kCFStringEncodingUTF8, false);
		if (!buf) goto leave;
	}
	
	SubRenderPacket(glob->ssa,c,buf,myDrp->width,myDrp->height);
		
	CFRelease(buf);
	
leave:
	CGContextRelease(c);
	return noErr;
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

// ImageCodecGetSourceDataGammaLevel
// Returns 1.8, the gamma for the Generic RGB Profile (didn't change on 10.6...).
// We should really just render sRGB instead, but that doesn't have an exact gamma value.
pascal ComponentResult TextSubCodecGetSourceDataGammaLevel(TextSubGlobals glob, Fixed *sourceDataGammaLevel)
{
	*sourceDataGammaLevel = FloatToFixed(1.8);
	return noErr;
}

// ImageCodecGetCodecInfo
//		Your component receives the ImageCodecGetCodecInfo request whenever an application calls the Image Compression Manager's GetCodecInfo function.
// Your component should return a formatted compressor information structure defining its capabilities.
// Both compressors and decompressors may receive this request.
pascal ComponentResult TextSubCodecGetCodecInfo(TextSubGlobals glob, CodecInfo *info)
{
	OSErr err = noErr;
	ComponentDescription desc;
	short resid;
	
	GetComponentInfo((Component)glob->self, &desc, 0, 0, 0);
	
	if (desc.componentSubType == kSubFormatSSA)
		resid = kSSASubCodecResourceID;
	else
		resid = kTextSubCodecResourceID;

	if (info == NULL) {
		err = paramErr;
	} else {
		CodecInfo **tempCodecInfo;

		err = GetComponentResource((Component)glob->self, codecInfoResourceType, resid, (Handle *)&tempCodecInfo);
		if (err == noErr) {
			*info = **tempCodecInfo;
			DisposeHandle((Handle)tempCodecInfo);
		}
	}

	return err;
}