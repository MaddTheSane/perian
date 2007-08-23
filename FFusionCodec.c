//---------------------------------------------------------------------------
//FFusion
//Alternative DivX Codec for MacOS X
//version 2.2 (build 72)
//by Jerome Cornet
//Copyright 2002-2003 Jerome Cornet
//parts by Dan Christiansen
//Copyright 2003 Dan Christiansen
//from DivOSX by Jamby
//Copyright 2001-2002 Jamby
//uses libavcodec from ffmpeg 0.4.6
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
//---------------------------------------------------------------------------
// Source Code
//---------------------------------------------------------------------------

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>
#include <Accelerate/Accelerate.h>

#include "FFusionCodec.h"
#include "EI_Image.h"
#include "avcodec.h"
#include "Codecprintf.h"
#include "ColorConversions.h"
#include "bitstream_info.h"
#include "FrameBuffer.h"
#include "CommonUtils.h"

void inline swapFrame(AVFrame * *a, AVFrame * *b)
{
	AVFrame *t = *a;
	*a = *b;
	*b = t;
}

//---------------------------------------------------------------------------
// Types
//---------------------------------------------------------------------------

// 32 because that's ffmpeg's INTERNAL_BUFFER_SIZE
#define FFUSION_MAX_BUFFERS 32

typedef struct
{
	AVFrame		*frame;
	bool		used;
	long		frameNumber;
} FFusionBuffer;

typedef enum
{
	PACKED_QUICKTIME_KNOWS_ORDER, /* This is for non-stupid container formats which actually know the difference between decode and display order */
	PACKED_ALL_IN_FIRST_FRAME, /* This is for the divx hack which contains a P frame and all subsequent B frames in a single frame. Ex: I, PB, -, PB, -, I...*/
	PACKED_DELAY_BY_ONE_FRAME /* This is for stupid containers where frames are soley writen in decode order.  Ex: I, P, B, P, B, P, I.... */
} FFusionPacked;

/* Why do these small structs?  It makes the usage of these variables clearer, no other reason */

/* globs used by the BeginBand routine */
struct begin_glob
{
	FFusionParserContext	*parser;
	long			lastFrame;
	long			lastIFrame;
	int				lastFrameType;
	int				futureType;
	FrameData		*lastPFrameData;
};

/* globs used by the DecodeBand routine */
struct decode_glob
{
	long			lastFrame;
	FFusionBuffer	*futureBuffer;
};

typedef struct
{
    ComponentInstance		self;
    ComponentInstance		delegateComponent;
    ComponentInstance		target;
    ImageCodecMPDrawBandUPP 	drawBandUPP;
    Handle			pixelTypes;
    AVCodec			*avCodec;
    AVCodecContext	*avContext;
    OSType			componentType;
    char			hasy420;
	FILE			*fileLog;
	AVFrame			lastDisplayedFrame;
	FFusionPacked	packedType;
	FFusionBuffer	buffers[FFUSION_MAX_BUFFERS];	// the buffers which the codec has retained
	int				lastAllocatedBuffer;		// the index of the buffer which was last allocated 
												// by the codec (and is the latest in decode order)	
	struct begin_glob	begin;
	FFusionData		data;
	struct decode_glob	decode;
} FFusionGlobalsRecord, *FFusionGlobals;

typedef struct
{
    long			width;
    long			height;
    long			depth;
    OSType			pixelFormat;
	int				decoded;
	long			frameNumber;
	long			GOPStartFrameNumber;
	long			bufferSize;
	FFusionBuffer	*buffer;
	FrameData		*frameData;
} FFusionDecompressRecord;


//---------------------------------------------------------------------------
// Prototypes of private subroutines
//---------------------------------------------------------------------------

static OSErr FFusionDecompress(AVCodecContext *context, UInt8 *dataPtr, ICMDataProcRecordPtr dataProc, long width, long height, AVFrame *picture, long length);
static int FFusionGetBuffer(AVCodecContext *s, AVFrame *pic);
static void FFusionReleaseBuffer(AVCodecContext *s, AVFrame *pic);
static void SetupMultithreadedDecoding(AVCodecContext *s, enum CodecID codecID);

int GetPPUserPreference();
void SetPPUserPreference(int value);
pascal OSStatus HandlePPDialogWindowEvent(EventHandlerCallRef  nextHandler, EventRef theEvent, void* userData);
pascal OSStatus HandlePPDialogControlEvent(EventHandlerCallRef  nextHandler, EventRef theEvent, void* userData);
void ChangeHintText(int value, ControlRef staticTextField);

extern void initLib();

//---------------------------------------------------------------------------
// Component Dispatcher
//---------------------------------------------------------------------------

#define IMAGECODEC_BASENAME() 		FFusionCodec
#define IMAGECODEC_GLOBALS() 		FFusionGlobals storage

#define CALLCOMPONENT_BASENAME()	IMAGECODEC_BASENAME()
#define	CALLCOMPONENT_GLOBALS()		IMAGECODEC_GLOBALS()

#define COMPONENT_UPP_PREFIX()		uppImageCodec
#define COMPONENT_DISPATCH_FILE		"FFusionCodecDispatch.h"
#define COMPONENT_SELECT_PREFIX()  	kImageCodec

#define	GET_DELEGATE_COMPONENT()	(storage->delegateComponent)

#include <QuickTime/ImageCodec.k.h>
#include <QuickTime/ComponentDispatchHelper.c>

void FFusionRunUpdateCheck()
{
    CFDateRef lastRunDate = CFPreferencesCopyAppValue(CFSTR("NextRunDate"), CFSTR("org.perian.Perian"));
    CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
    
	if (lastRunDate != nil && CFDateGetAbsoluteTime(lastRunDate) > now)
        return;

    if(lastRunDate != nil)
        CFRelease(lastRunDate);
    
    //Two places to check, home dir and /
    
    CFStringRef home = (CFStringRef)NSHomeDirectory();
    CFMutableStringRef location = CFStringCreateMutableCopy(NULL, 0, home);
    CFStringAppend(location, CFSTR("/Library/PreferencePanes/Perian.prefPane/Contents/Resources/PerianUpdateChecker.app"));
    
    char fileRep[1024];
    FSRef updateCheckRef;
    Boolean doCheck = FALSE;
    
    if(CFStringGetFileSystemRepresentation(location, fileRep, 1024))
        if(FSPathMakeRef((UInt8 *)fileRep, &updateCheckRef, NULL) == noErr)
            doCheck = TRUE;
    
    CFRelease(location);
    if(doCheck == FALSE)
    {
        CFStringRef absLocation = CFSTR("/Library/PreferencePanes/Perian.prefPane/Contents/Resources/PerianUpdateChecker.app");
        if(CFStringGetFileSystemRepresentation(absLocation, fileRep, 1024))
            if(FSPathMakeRef((UInt8 *)fileRep, &updateCheckRef, NULL) != noErr)
                return;  //We have failed
    }
    
    LSOpenFSRef(&updateCheckRef, NULL);    
}

//---------------------------------------------------------------------------
// Component Routines
//---------------------------------------------------------------------------

// -- This Image Decompressor Use the Base Image Decompressor Component --
//	The base image decompressor is an Apple-supplied component
//	that makes it easier for developers to create new decompressors.
//	The base image decompressor does most of the housekeeping and
//	interface functions required for a QuickTime decompressor component,
//	including scheduling for asynchronous decompression.

//-----------------------------------------------------------------
// Component Open Request - Required
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecOpen(FFusionGlobals glob, ComponentInstance self)
{
    ComponentResult err;
    ComponentDescription descout;
    ComponentDescription cd;
    Component c = 0;
    long bitfield;
	
    cd.componentType = 'imdc';
    cd.componentSubType = 'y420';
    cd.componentManufacturer = 0;
    cd.componentFlags = 0;
    cd.componentFlagsMask = 0;
    
    GetComponentInfo((Component)self, &descout, 0, 0, 0);
	
  //  FourCCprintf("Opening component for type ", descout.componentSubType);
	
    // Allocate memory for our globals, set them up and inform the component manager that we've done so
	
    glob = (FFusionGlobals)NewPtrClear(sizeof(FFusionGlobalsRecord));
    
    if (err = MemError())
    {
        Codecprintf(NULL, "Unable to allocate globals! Exiting.\n");            
    }
    else
    {
        SetComponentInstanceStorage(self, (Handle)glob);
        
        glob->self = self;
        glob->target = self;
        glob->drawBandUPP = NULL;
        glob->pixelTypes = NewHandle(10 * sizeof(OSType));
        glob->avCodec = 0;
        glob->hasy420 = 0;
        glob->componentType = descout.componentSubType;
		glob->packedType = PACKED_ALL_IN_FIRST_FRAME;  //Unless we have reason to believe otherwise.
		glob->data.frames = NULL;
		glob->begin.parser = NULL;
#ifdef FILELOG
		glob->fileLog = fopen("/tmp/perian.log", "a");
#else
		glob->fileLog = NULL;
#endif
		
//        c = FindNextComponent(c, &cd);
		
        if (c != 0)
        {            
            Gestalt(gestaltSystemVersion, &bitfield);
            
            if (bitfield >= 0x1010)
            {
  //              Codecprintf(glob->fileLog, "Use speedy y420 component\n");
                glob->hasy420 = 1;
            }
        }
        else
        {
  //          Codecprintf(glob->fileLog, "Use slow y420 component\n");
        }
		
        // Open and target an instance of the base decompressor as we delegate
        // most of our calls to the base decompressor instance
        
        err = OpenADefaultComponent(decompressorComponentType, kBaseCodecType, &glob->delegateComponent);
        if (!err)
        {
            ComponentSetTarget(glob->delegateComponent, self);
        }
        else
        {
            Codecprintf(glob->fileLog, "Error opening the base image decompressor! Exiting.\n");
        }
        FFusionRunUpdateCheck();
    }
    
    return err;
}

//-----------------------------------------------------------------
// Component Close Request - Required
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecClose(FFusionGlobals glob, ComponentInstance self)
{
    // Make sure to close the base component and deallocate our storage
    
    if (glob) 
    {
        if (glob->delegateComponent) 
        {
            CloseComponent(glob->delegateComponent);
        }
        
        if (glob->drawBandUPP) 
        {
            DisposeImageCodecMPDrawBandUPP(glob->drawBandUPP);
        }
        
        if (glob->avCodec)
        {
            avcodec_close(glob->avContext);
        }
				
        if (glob->avContext)
        {
			if (glob->avContext->extradata)
				free(glob->avContext->extradata);
						
            av_free(glob->avContext);
        }
		
		if (glob->begin.parser)
		{
			freeFFusionParser(glob->begin.parser);
		}
		
		if (glob->pixelTypes)
		{
			DisposeHandle(glob->pixelTypes);
		}
		
		FFusionDataFree(&(glob->data));
        
		if(glob->fileLog)
			fclose(glob->fileLog);
		
        DisposePtr((Ptr)glob);
    }
	
    return noErr;
}

//-----------------------------------------------------------------
// Component Version Request - Required
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecVersion(FFusionGlobals glob)
{
    return kFFusionCodecVersion;
}

//-----------------------------------------------------------------
// Component Target Request
//-----------------------------------------------------------------
// Allows another component to "target" you i.e., you call 
// another component whenever you would call yourself (as a result 
// of your component being used by another component)
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecTarget(FFusionGlobals glob, ComponentInstance target)
{
    glob->target = target;
	
    return noErr;
}

//-----------------------------------------------------------------
// Component GetMPWorkFunction Request
//-----------------------------------------------------------------
// Allows your image decompressor component to perform asynchronous 
// decompression in a single MP task by taking advantage of the 
// Base Decompressor. If you implement this selector, your DrawBand 
// function must be MP-safe. MP safety means not calling routines 
// that may move or purge memory and not calling any routines which
// might cause 68K code to be executed. Ideally, your DrawBand 
// function should not make any API calls whatsoever. Obviously 
// don't implement this if you're building a 68k component.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecGetMPWorkFunction(FFusionGlobals glob, ComponentMPWorkFunctionUPP *workFunction, void **refCon)
{
	if (glob->drawBandUPP == NULL)
		glob->drawBandUPP = NewImageCodecMPDrawBandUPP((ImageCodecMPDrawBandProcPtr)FFusionCodecDrawBand);
	
	return ImageCodecGetBaseMPWorkFunction(glob->delegateComponent, workFunction, refCon, glob->drawBandUPP, glob);
}

//-----------------------------------------------------------------
// ImageCodecInitialize
//-----------------------------------------------------------------
// The first function call that your image decompressor component 
// receives from the base image decompressor is always a call to 
// ImageCodecInitialize . In response to this call, your image 
// decompressor component returns an ImageSubCodecDecompressCapabilities 
// structure that specifies its capabilities.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecInitialize(FFusionGlobals glob, ImageSubCodecDecompressCapabilities *cap)
{
	
    // Secifies the size of the ImageSubCodecDecompressRecord structure
    // and say we can support asyncronous decompression
    // With the help of the base image decompressor, any image decompressor
    // that uses only interrupt-safe calls for decompression operations can
    // support asynchronous decompression.
	
    cap->decompressRecordSize = sizeof(FFusionDecompressRecord) + 12;
    cap->canAsync = true;
	
	// QT 7
	if(cap->recordSize > offsetof(ImageSubCodecDecompressCapabilities, baseCodecShouldCallDecodeBandForAllFrames))
	{
		cap->subCodecIsMultiBufferAware = true;
		cap->subCodecSupportsOutOfOrderDisplayTimes = true;
		cap->baseCodecShouldCallDecodeBandForAllFrames = true;
		cap->subCodecSupportsScheduledBackwardsPlaybackWithDifferenceFrames = true;
		cap->subCodecSupportsDrawInDecodeOrder = true; 
		cap->subCodecSupportsDecodeSmoothing = true; 
	}
	
    return noErr;
}

//-----------------------------------------------------------------
// ImageCodecPreflight
//-----------------------------------------------------------------
// The base image decompressor gets additional information about the 
// capabilities of your image decompressor component by calling 
// ImageCodecPreflight. The base image decompressor uses this
// information when responding to a call to the ImageCodecPredecompress 
// function, which the ICM makes before decompressing an image. You 
// are required only to provide values for the wantedDestinationPixelSize 
// and wantedDestinationPixelTypes fields and can also modify other
// fields if necessary.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecPreflight(FFusionGlobals glob, CodecDecompressParams *p)
{
    OSType *pos;
    int index;
    CodecCapabilities *capabilities = p->capabilities;
    Byte* myptr;
	long count = 0;
	Handle imgDescExt;
	
    // We first open libavcodec library and the codec corresponding
    // to the fourCC if it has not been done before
    
    if (!glob->avCodec)
    {
		enum CodecID codecID = CODEC_ID_MPEG4;
		
		initLib();
		initFFusionParsers();
		
        switch (glob->componentType)
        {
            case 'MPG4':	// MS-MPEG4 v1
            case 'mpg4':
            case 'DIV1':
            case 'div1':
                codecID = CODEC_ID_MSMPEG4V1;
				break;
				
            case 'MP42':	// MS-MPEG4 v2
            case 'mp42':
            case 'DIV2':
            case 'div2':
                codecID = CODEC_ID_MSMPEG4V2;
				break;
				
            case 'div6':	// DivX 3
            case 'DIV6':
            case 'div5':
            case 'DIV5':
            case 'div4':
            case 'DIV4':
            case 'div3':
            case 'DIV3':
            case 'MP43':
            case 'mp43':
            case 'MPG3':
            case 'mpg3':
            case 'AP41':
            case 'COL0':
            case 'col0':
            case 'COL1':
            case 'col1':
            case '3IVD':	// 3ivx
            case '3ivd':
                codecID = CODEC_ID_MSMPEG4V3;
				break;
				
			case 'mp4v':	// MPEG4 part 2 in mov/mp4
				glob->packedType = PACKED_QUICKTIME_KNOWS_ORDER;
            case 'divx':	// DivX 4
            case 'DIVX':
            case 'mp4s':
            case 'MP4S':
            case 'm4s2':
            case 'M4S2':
            case 0x04000000:
            case 'UMP4':
            case 'DX50':	// DivX 5
            case 'XVID':	// XVID
            case 'xvid':
            case 'XviD':
            case 'XVIX':
            case 'BLZ0':
            case '3IV2':	// 3ivx
            case '3iv2':
			case 'RMP4':	// Miscellaneous
			case 'SEDG':
			case 'WV1F':
			case 'FMP4':
			case 'SMP4':
                codecID = CODEC_ID_MPEG4;
				break;

			case 'avc1':	// H.264 in mov/mp4/mkv
				glob->packedType = PACKED_QUICKTIME_KNOWS_ORDER;
			case 'H264':	// H.264 in AVI
			case 'h264':
			case 'X264':
			case 'x264':
			case 'AVC1':
			case 'DAVC':
			case 'VSSH':
				codecID = CODEC_ID_H264;
				break;

			case 'FLV1':
				codecID = CODEC_ID_FLV1;
				break;

			case 'FSV1':
				codecID = CODEC_ID_FLASHSV;
				break;

			case 'VP60':
			case 'VP61':
			case 'VP62':
				codecID = CODEC_ID_VP6;
				break;

			case 'VP6F':
			case 'FLV4':
				codecID = CODEC_ID_VP6F;
				break;

			case 'I263':
			case 'i263':
				codecID = CODEC_ID_H263I;
				break;

			case 'VP30':
			case 'VP31':
				codecID = CODEC_ID_VP3;
				break;
				
			case 'HFYU':
				codecID = CODEC_ID_HUFFYUV;
				break;

			case 'FFVH':
				codecID = CODEC_ID_FFVHUFF;
				break;
				
			case 'MPEG':
			case 'mpg1':
			case 'mp1v':
				codecID = CODEC_ID_MPEG1VIDEO;
				break;
				
			case 'MPG2':
			case 'mpg2':
			case 'mp2v':
				codecID = CODEC_ID_MPEG2VIDEO;
				break;
				
			case 'FPS1':
				codecID = CODEC_ID_FRAPS;
				break;
				
			case 'SNOW':
				codecID = CODEC_ID_SNOW;
				break;
				
            default:
				Codecprintf(glob->fileLog, "Warning! Unknown codec type! Using MPEG4 by default.\n");
        }
		
		glob->avCodec = avcodec_find_decoder(codecID);
//		if(glob->packedType != PACKED_QUICKTIME_KNOWS_ORDER)
			glob->begin.parser = ffusionParserInit(codecID);
                
        // we do the same for the AVCodecContext since all context values are
        // correctly initialized when calling the alloc function
        
        glob->avContext = avcodec_alloc_context();
		
		// Use low delay
		glob->avContext->flags |= CODEC_FLAG_LOW_DELAY;
		
        // Image size is mandatory for DivX-like codecs
        
        glob->avContext->width = (**p->imageDescription).width;
        glob->avContext->height = (**p->imageDescription).height;
		
        // We also pass the FourCC since it allows the H263 hybrid decoder
        // to make the difference between the various flavours of DivX
        
        myptr = (unsigned char *)&(glob->componentType);
        glob->avContext->codec_tag = (myptr[3] << 24) + (myptr[2] << 16) + (myptr[1] << 8) + myptr[0];
        
		// avc1 requires the avcC extension
		if (glob->componentType == 'avc1') {
			count = isImageDescriptionExtensionPresent(p->imageDescription, 'avcC');
			
			if (count >= 1) {
				imgDescExt = NewHandle(0);
				GetImageDescriptionExtension(p->imageDescription, &imgDescExt, 'avcC', 1);
				
				glob->avContext->extradata = calloc(1, GetHandleSize(imgDescExt) + FF_INPUT_BUFFER_PADDING_SIZE);
				memcpy(glob->avContext->extradata, *imgDescExt, GetHandleSize(imgDescExt));
				glob->avContext->extradata_size = GetHandleSize(imgDescExt);
				
				DisposeHandle(imgDescExt);
			} else {
				count = isImageDescriptionExtensionPresent(p->imageDescription, 'strf');
				
				// avc1 in AVI, need to reorder frames
				if (count >= 1)
					glob->packedType = PACKED_ALL_IN_FIRST_FRAME;
			}
		} else if (glob->componentType == 'mp4v') {
			count = isImageDescriptionExtensionPresent(p->imageDescription, 'esds');
			
			if (count >= 1) {
				imgDescExt = NewHandle(0);
				GetImageDescriptionExtension(p->imageDescription, &imgDescExt, 'esds', 1);
				
				ReadESDSDescExt(imgDescExt, &glob->avContext->extradata, &glob->avContext->extradata_size);
				
				DisposeHandle(imgDescExt);
			}
		} else {
			count = isImageDescriptionExtensionPresent(p->imageDescription, 'strf');
			
			if (count >= 1) {
				imgDescExt = NewHandle(0);
				GetImageDescriptionExtension(p->imageDescription, &imgDescExt, 'strf', 1);
				
				if (GetHandleSize(imgDescExt) - 40 > 0) {
					glob->avContext->extradata = calloc(1, GetHandleSize(imgDescExt) - 40 + FF_INPUT_BUFFER_PADDING_SIZE);
					memcpy(glob->avContext->extradata, *imgDescExt + 40, GetHandleSize(imgDescExt) - 40);
					glob->avContext->extradata_size = GetHandleSize(imgDescExt) - 40;
				}
				DisposeHandle(imgDescExt);
			}
		}
		
		if(glob->avContext->extradata_size != 0 && glob->begin.parser != NULL)
			ffusionParseExtraData(glob->begin.parser, glob->avContext->extradata, glob->avContext->extradata_size);
		
		// some hooks into ffmpeg's buffer allocation to get frames in 
		// decode order without delay more easily
		glob->avContext->opaque = glob;
		glob->avContext->get_buffer = FFusionGetBuffer;
		glob->avContext->release_buffer = FFusionReleaseBuffer;
		
		// multi-slice decoding
		SetupMultithreadedDecoding(glob->avContext, codecID);
		
        // Finally we open the avcodec 
        
        if (avcodec_open(glob->avContext, glob->avCodec))
        {
            Codecprintf(glob->fileLog, "Error opening avcodec!\n");
            
            return -2;
        }
		
		// we allocate some space for copying the frame data since we need some padding at the end
		// for ffmpeg's optimized bitstream readers. Size doesn't really matter, it'll grow if need be
		FFusionDataSetup(&(glob->data), 256, 64*1024);
    }
    
    // Specify the minimum image band height supported by the component
    // bandInc specifies a common factor of supported image band heights - 
    // if your component supports only image bands that are an even
    // multiple of some number of pixels high report this common factor in bandInc
    
    capabilities->bandMin = (**p->imageDescription).height;
    capabilities->bandInc = capabilities->bandMin;
	
    // libavcodec 0.4.x is no longer stream based i.e. you cannot pass just
    // an arbitrary amount of data to the library.
    // Instead we have to tell QT to just pass the data corresponding 
    // to one frame
	
    capabilities->flags |= codecWantsSpecialScaling;
    
    p->requestedBufferWidth = (**p->imageDescription).width;
    p->requestedBufferHeight = (**p->imageDescription).height;
    
    // Indicate the pixel depth the component can use with the specified image
    // normally should be capabilities->wantedPixelSize = (**p->imageDescription).depth;
    // but we don't care since some DivX are so bugged that the depth information
    // is not correct
    
    capabilities->wantedPixelSize = 0;
    
    // Type of pixels used in output
    // If QuickTime got the y420 component it is cool
    // since libavcodec ouputs y420
    // If not we'll do some king of conversion to 2vuy
    
    HLock(glob->pixelTypes);
    pos = *((OSType **)glob->pixelTypes);
    
    index = 0;
	
	switch (glob->avContext->pix_fmt)
	{
		case PIX_FMT_BGR24:
			pos[index++] = k24RGBPixelFormat;
			break;
		case PIX_FMT_RGB32:
			pos[index++] = k32ARGBPixelFormat;
			break;
		case PIX_FMT_YUV420P:
		default:
			if (glob->hasy420)
			{
				pos[index++] = 'y420';
			}
			else
			{
				pos[index++] = k2vuyPixelFormat;        
			}
			break;
	}
    
    pos[index++] = 0;
    HUnlock(glob->pixelTypes);
	
    p->wantedDestinationPixelTypes = (OSType **)glob->pixelTypes;
    
    // Specify the number of pixels the image must be extended in width and height if
    // the component cannot accommodate the image at its given width and height
    // It is not the case here
    
    capabilities->extendWidth = 0;
    capabilities->extendHeight = 0;
    
	capabilities->flags |= codecCanAsync | codecCanAsyncWhen;
	
    
    return noErr;
}

static int qtTypeForFrameInfo(int original, int fftype, int skippable)
{
	if(fftype == FF_I_TYPE)
	{
		if(!skippable)
			return kCodecFrameTypeKey;
	}
	else if(skippable)
		return kCodecFrameTypeDroppableDifference;
	else if(fftype != 0)
		return kCodecFrameTypeDifference;	
	return original;
}

//-----------------------------------------------------------------
// ImageCodecBeginBand
//-----------------------------------------------------------------
// The ImageCodecBeginBand function allows your image decompressor 
// component to save information about a band before decompressing 
// it. This function is never called at interrupt time. The base 
// image decompressor preserves any changes your component makes to 
// any of the fields in the ImageSubCodecDecompressRecord or 
// CodecDecompressParams structures. If your component supports 
// asynchronous scheduled decompression, it may receive more than
// one ImageCodecBeginBand call before receiving an ImageCodecDrawBand 
// call.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecBeginBand(FFusionGlobals glob, CodecDecompressParams *p, ImageSubCodecDecompressRecord *drp, long flags)
{	
    long offsetH, offsetV;
    FFusionDecompressRecord *myDrp = (FFusionDecompressRecord *)drp->userDecompressRecord;
	int redisplayFirstFrame = 0;
	
    //////
  /*  IBNibRef 		nibRef;
    WindowRef 		window;
    OSStatus		err;
    CFBundleRef		bundleRef;
    EventHandlerUPP  	handlerUPP, handlerUPP2;
    EventTypeSpec    	eventType;
    ControlID		controlID;
    ControlRef		theControl;
    KeyMap		currentKeyMap;
    int			userPreference; */
    ///////
	
    offsetH = (long)(p->dstRect.left - p->dstPixMap.bounds.left) * (long)(p->dstPixMap.pixelSize >> 3);
    offsetV = (long)(p->dstRect.top - p->dstPixMap.bounds.top) * (long)drp->rowBytes;
    
    myDrp->width = (**p->imageDescription).width;
    myDrp->height = (**p->imageDescription).height;
    myDrp->depth = (**p->imageDescription).depth;
	
    myDrp->pixelFormat = p->dstPixMap.pixelFormat;
	myDrp->decoded = p->frameTime ? (0 != (p->frameTime->flags & icmFrameAlreadyDecoded)) : false;
	myDrp->frameData = NULL;
	myDrp->buffer = NULL;
	
	if(myDrp->decoded)
	{
		int i;
		for (i = 0; i < FFUSION_MAX_BUFFERS; i++) {
			if (glob->buffers[i].used && glob->buffers[i].frameNumber == myDrp->frameNumber) {
				myDrp->buffer = &glob->buffers[i];
				break;
			}
		}
		return noErr;
	}
	
	if(glob->packedType != PACKED_QUICKTIME_KNOWS_ORDER && p->frameNumber != glob->begin.lastFrame + 1)
	{
		if(glob->decode.lastFrame < p->frameNumber && p->frameNumber < glob->begin.lastFrame + 1)
		{
			/* We already began this sucker, but haven't decoded it yet, find the data */
			FrameData *frameData = NULL;
			frameData = FFusionDataFind(&glob->data, p->frameNumber);
			if(frameData != NULL)
			{
				myDrp->frameData = frameData;
				drp->frameType = qtTypeForFrameInfo(drp->frameType, myDrp->frameData->type, myDrp->frameData->skippabble);
				myDrp->frameNumber = p->frameNumber;
				myDrp->GOPStartFrameNumber = glob->begin.lastIFrame;
				frameData->hold = 0;
				if(frameData->prereqFrame)
					frameData->prereqFrame->hold = 1;
				return noErr;
			}
		}
		else
		{
			/* Reset context, safe marking in such a case */
			glob->begin.lastFrameType = FF_I_TYPE;
			glob->data.unparsedFrames.dataSize = 0;
			glob->begin.lastPFrameData = NULL;
			redisplayFirstFrame = 1;
		}
	}
	
	if(glob->begin.parser != NULL)
	{
		int parsedBufSize;
		int type = 0;
		int skippable = 0;
		uint8_t *buffer = (uint8_t *)drp->codecData;
		int bufferSize = p->bufferSize;
		
		if(glob->data.unparsedFrames.dataSize != 0)
		{
			buffer = glob->data.unparsedFrames.buffer;
			bufferSize = glob->data.unparsedFrames.dataSize;
		}
		
		if(ffusionParse(glob->begin.parser, buffer, bufferSize, &parsedBufSize, &type, &skippable) == 0)
		{
			/* parse failed */
			myDrp->bufferSize = bufferSize;
			if(glob->begin.futureType != 0)
			{
				/* Assume our previously decoded P frame */
				type = glob->begin.futureType;
				glob->begin.futureType = 0;
				myDrp->frameData = glob->begin.lastPFrameData;
			}
			else
			{
				Codecprintf(NULL, "parse failed frame %d with size %d\n", p->frameNumber, bufferSize);
				if(glob->data.unparsedFrames.dataSize != 0)
					Codecprintf(NULL, ", parser had extra data\n");				
			}
		}
		else if(glob->packedType != PACKED_QUICKTIME_KNOWS_ORDER)
		{
			if(type == FF_B_TYPE && glob->packedType == PACKED_ALL_IN_FIRST_FRAME && glob->begin.futureType == 0)
				/* Badly framed.  We hit a B frame after it was supposed to be displayed, switch to delaying by a frame */
				glob->packedType = PACKED_DELAY_BY_ONE_FRAME;
			
			if(FFusionCreateDataBuffer(&(glob->data), (uint8_t *)drp->codecData, parsedBufSize))
				myDrp->frameData = FFusionDataAppend(&(glob->data), parsedBufSize, type);
			if(type != FF_I_TYPE)
				myDrp->frameData->prereqFrame = glob->begin.lastPFrameData;
			if(glob->packedType == PACKED_DELAY_BY_ONE_FRAME)
			{
				if(type != FF_B_TYPE)
				{
					FrameData *nextPFrame = myDrp->frameData;
					FrameData *lastPFrame = glob->begin.lastPFrameData;
					if(lastPFrame != NULL)
						/* Mark the next P or I frame, predictive decoding */
						lastPFrame->nextFrame = nextPFrame;
					myDrp->frameData = lastPFrame;
					glob->begin.lastPFrameData = nextPFrame;

					if(redisplayFirstFrame)
						myDrp->frameData = nextPFrame;
				}
				
				int displayType = glob->begin.lastFrameType;
				glob->begin.lastFrameType = type;
				type = displayType;
			}
			else if(type != FF_B_TYPE)
				glob->begin.lastPFrameData = myDrp->frameData;

			if(type == FF_I_TYPE && glob->packedType == PACKED_ALL_IN_FIRST_FRAME)
				/* Wipe memory of past P frames */
				glob->begin.futureType = 0;
			
			if(parsedBufSize < p->bufferSize)
			{
				int oldType = type;
				
				buffer += parsedBufSize;
				bufferSize -= parsedBufSize;
				int success = ffusionParse(glob->begin.parser, buffer, bufferSize, &parsedBufSize, &type, &skippable);
				if(success && type == FF_B_TYPE)
				{
					/* A B frame follows us, so setup the P frame for the future and set dependencies */
					glob->begin.futureType = oldType;
					if(glob->begin.lastPFrameData != NULL)
						/* Mark the next P or I frame, predictive decoding */
						glob->begin.lastPFrameData->nextFrame = myDrp->frameData;
					glob->begin.lastPFrameData = myDrp->frameData;
					if(FFusionCreateDataBuffer(&(glob->data), buffer, parsedBufSize))
						myDrp->frameData = FFusionDataAppend(&(glob->data), parsedBufSize, type);
					myDrp->frameData->prereqFrame = glob->begin.lastPFrameData;
					buffer += parsedBufSize;
					bufferSize -= parsedBufSize;
				}
				if(bufferSize > 0)
					FFusionDataSetUnparsed(&(glob->data), buffer, bufferSize);
				else
					glob->data.unparsedFrames.dataSize = 0;
			}
			else
				glob->data.unparsedFrames.dataSize = 0;
			myDrp->bufferSize = 0;
		}
		else
		{
			myDrp->bufferSize = bufferSize;
		}
		
		drp->frameType = qtTypeForFrameInfo(drp->frameType, type, skippable);
		if(myDrp->frameData != NULL)
		{
			myDrp->frameData->frameNumber = p->frameNumber;
			myDrp->frameData->skippabble = skippable;
		}
	}
	else
		myDrp->bufferSize = p->bufferSize;
	glob->begin.lastFrame = p->frameNumber;
	if(drp->frameType == kCodecFrameTypeKey)
		glob->begin.lastIFrame = p->frameNumber;
	myDrp->frameNumber = p->frameNumber;
	myDrp->GOPStartFrameNumber = glob->begin.lastIFrame;
    return noErr;
}

static OSErr PrereqDecompress(FrameData *prereq, AVCodecContext *context, ICMDataProcRecordPtr dataProc, long width, long height, AVFrame *picture)
{
	FrameData *preprereq = FrameDataCheckPrereq(prereq);
	if(preprereq)
		PrereqDecompress(preprereq, context, dataProc, width, height, picture);
	
	unsigned char *dataPtr = (unsigned char *)prereq->buffer;
	int dataSize = prereq->dataSize;
	
	OSErr err = FFusionDecompress(context, dataPtr, dataProc, width, height, picture, dataSize);
	
	return err;
}

pascal ComponentResult FFusionCodecDecodeBand(FFusionGlobals glob, ImageSubCodecDecompressRecord *drp, unsigned long flags)
{
	OSErr err = noErr;
	AVFrame tempFrame;

    FFusionDecompressRecord *myDrp = (FFusionDecompressRecord *)drp->userDecompressRecord;

	avcodec_get_frame_defaults(&tempFrame);
	
	// QuickTime will drop H.264 frames when necessary if a sample dependency table exists
	// we don't want to flush buffers in that case.
	if(glob->packedType != PACKED_QUICKTIME_KNOWS_ORDER && myDrp->frameNumber != glob->decode.lastFrame + 1)
	{
		/* Skipped some frames in here */
		if(drp->frameType == kCodecFrameTypeKey || myDrp->GOPStartFrameNumber > glob->decode.lastFrame || myDrp->frameNumber < glob->decode.lastFrame)
		{
			/* If this is a key frame or the P frame before us is after the last frame (skip ahead), or we are before the last decoded frame (skip back) *
			 * then we are in a whole new GOP */
			avcodec_flush_buffers(glob->avContext);
		}
	}
	
	if(myDrp->frameData && myDrp->frameData->decoded && glob->decode.futureBuffer != NULL)
	{
		myDrp->buffer = glob->decode.futureBuffer;
		myDrp->decoded = true;
#if 0	/* Need to make sure this frame's data is not eradicated during the decompress */
		FrameData *nextFrame = myDrp->frameData->nextFrame;
		if(nextFrame != NULL)
		{
			FFusionDecompress(glob->avContext, nextFrame->buffer, NULL, myDrp->width, myDrp->height, &tempFrame, nextFrame->dataSize);
			if(tempFrame.data[0] != NULL)
			{
				glob->decode.futureBuffer = (FFusionBuffer *)tempFrame.opaque;
				nextFrame->decoded = TRUE;
			}
		}
		else
#endif
			glob->decode.futureBuffer = NULL;
		FFusionDataMarkRead(&(glob->data), myDrp->frameData);
		glob->decode.lastFrame = myDrp->frameNumber;
		return err;
	}
			
	FrameData *frameData = NULL;
	unsigned char *dataPtr = NULL;
	unsigned int dataSize;
	if(glob->packedType != PACKED_QUICKTIME_KNOWS_ORDER && myDrp->frameData != NULL && myDrp->frameData->decoded == 0)
	{
		/* Pull from our buffer */
		frameData = myDrp->frameData;
		FrameData *prereq = FrameDataCheckPrereq(frameData);
		
		if(prereq)
		{
			err = PrereqDecompress(prereq, glob->avContext, NULL, myDrp->width, myDrp->height, &tempFrame);
			if(tempFrame.data[0] != NULL)
			{
				glob->decode.futureBuffer = (FFusionBuffer *)tempFrame.opaque;
				prereq->decoded = TRUE;
			}
		}
		dataPtr = (unsigned char *)frameData->buffer;
		dataSize = frameData->dataSize;
		frameData->decoded = TRUE;
	}
	else
	{
		/* data is already set up properly for us */
		dataSize = myDrp->bufferSize;
		FFusionCreateDataBuffer(&(glob->data), (uint8_t *)drp->codecData, dataSize);
		dataPtr = glob->data.buffer;
	}
	ICMDataProcRecordPtr dataProc = drp->dataProcRecord.dataProc ? &drp->dataProcRecord : NULL;
		
	avcodec_get_frame_defaults(&tempFrame);
	err = FFusionDecompress(glob->avContext, dataPtr, dataProc, myDrp->width, myDrp->height, &tempFrame, dataSize);
	
	if (glob->packedType == PACKED_QUICKTIME_KNOWS_ORDER) {
		myDrp->buffer = &glob->buffers[glob->lastAllocatedBuffer];
		myDrp->buffer->frameNumber = myDrp->frameNumber;
		myDrp->decoded = true;
		return err;
	}
	if(tempFrame.data[0] == NULL)
		myDrp->buffer = NULL;
	else
		myDrp->buffer = (FFusionBuffer *)tempFrame.opaque;
	
	if(tempFrame.pict_type == FF_I_TYPE)
		/* Wipe memory of past P frames */
		glob->decode.futureBuffer = NULL;
	glob->decode.lastFrame = myDrp->frameNumber;
	myDrp->decoded = true;
	
	FFusionDataMarkRead(&(glob->data), frameData);

	return err;
}

//-----------------------------------------------------------------
// ImageCodecDrawBand
//-----------------------------------------------------------------
// The base image decompressor calls your image decompressor 
// component's ImageCodecDrawBand function to decompress a band or 
// frame. Your component must implement this function. If the 
// ImageSubCodecDecompressRecord structure specifies a progress function 
// or data-loading function, the base image decompressor will never call 
// ImageCodecDrawBand at interrupt time. If the ImageSubCodecDecompressRecord 
// structure specifies a progress function, the base image decompressor
// handles codecProgressOpen and codecProgressClose calls, and your image 
// decompressor component must not implement these functions.
// If not, the base image decompressor may call the ImageCodecDrawBand 
// function at interrupt time. When the base image decompressor calls your 
// ImageCodecDrawBand function, your component must perform the decompression 
// specified by the fields of the ImageSubCodecDecompressRecord structure. 
// The structure includes any changes your component made to it when
// performing the ImageCodecBeginBand function. If your component supports 
// asynchronous scheduled decompression, it may receive more than one 
// ImageCodecBeginBand call before receiving an ImageCodecDrawBand call.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecDrawBand(FFusionGlobals glob, ImageSubCodecDecompressRecord *drp)
{
    OSErr err = noErr;
    FFusionDecompressRecord *myDrp = (FFusionDecompressRecord *)drp->userDecompressRecord;
	int i, j;
	
	if(!myDrp->decoded)
		err = FFusionCodecDecodeBand(glob, drp, 0);
	
	AVFrame *picture;
	
	if (myDrp->buffer)
		picture = myDrp->buffer->frame;
	else
		picture = &glob->lastDisplayedFrame;
	
	if(!picture || picture->data[0] == 0)
	{
		if(glob->lastDisplayedFrame.data[0] != NULL)
			//Display last frame
			picture = &(glob->lastDisplayedFrame);
		else
		{ 
			//Can't display anything so put up a black frame
			Ptr addr = drp->baseAddr; 
			for(i=0; i<myDrp->height; i++) 
			{ 
				for(j=0; j<myDrp->width*2; j+=2) 
				{ 
					addr[j] = 0x80; 
					addr[j+1] = 0x10; 
					addr[j+2] = 0x80; 
					addr[j+3] = 0x10; 
				} 
				addr += drp->rowBytes; 
			} 
			return noErr; 
		}
	}
	else
	{
		memcpy(&(glob->lastDisplayedFrame), picture, sizeof(AVFrame));
	}
	
	if (myDrp->pixelFormat == 'y420' && glob->avContext->pix_fmt == PIX_FMT_YUV420P)
	{
		FastY420((UInt8 *)drp->baseAddr, picture);
	}
	else if (myDrp->pixelFormat == k2vuyPixelFormat && glob->avContext->pix_fmt == PIX_FMT_YUV420P)
	{
		Y420toY422((UInt8 *)drp->baseAddr, drp->rowBytes, myDrp->width, myDrp->height, picture);
	}
	else if (myDrp->pixelFormat == k24RGBPixelFormat && glob->avContext->pix_fmt == PIX_FMT_BGR24)
	{
		BGR24toRGB24((UInt8 *)drp->baseAddr, drp->rowBytes, myDrp->width, myDrp->height, picture);
	}
	else if (myDrp->pixelFormat == k32ARGBPixelFormat && glob->avContext->pix_fmt == PIX_FMT_RGB32)
	{
		RGB32toRGB32((UInt8 *)drp->baseAddr, drp->rowBytes, myDrp->width, myDrp->height, picture);
	}
	else if (myDrp->pixelFormat == k2vuyPixelFormat && glob->avContext->pix_fmt == PIX_FMT_YUV422P)
	{
		Y422toY422((UInt8 *)drp->baseAddr, drp->rowBytes, myDrp->width, myDrp->height, picture);
	}
	else
	{
		Codecprintf(glob->fileLog, "Unsupported conversion from PIX_FMT %d to %c%c%c%c (%08x) buffer\n",
					glob->avContext->pix_fmt, myDrp->pixelFormat >> 24 & 0xff, myDrp->pixelFormat >> 16 & 0xff, 
					myDrp->pixelFormat >> 8 & 0xff, myDrp->pixelFormat & 0xff, myDrp->pixelFormat);
	}
	
    return err;
}

//-----------------------------------------------------------------
// ImageCodecEndBand
//-----------------------------------------------------------------
// The ImageCodecEndBand function notifies your image decompressor 
// component that decompression of a band has finished or
// that it was terminated by the Image Compression Manager. Your 
// image decompressor component is not required to implement the 
// ImageCodecEndBand function. The base image decompressor may call 
// the ImageCodecEndBand function at interrupt time.
// After your image decompressor component handles an ImageCodecEndBand 
// call, it can perform any tasks that are required when decompression
// is finished, such as disposing of data structures that are no longer 
// needed. Because this function can be called at interrupt time, your 
// component cannot use this function to dispose of data structures; 
// this must occur after handling the function. The value of the result
// parameter should be set to noErr if the band or frame was
// drawn successfully.
// If it is any other value, the band or frame was not drawn.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecEndBand(FFusionGlobals glob, ImageSubCodecDecompressRecord *drp, OSErr result, long flags)
{
    return noErr;
}

//-----------------------------------------------------------------
// ImageCodecQueueStarting
//-----------------------------------------------------------------
// If your component supports asynchronous scheduled decompression,
// the base image decompressor calls your image decompressor 
// component's ImageCodecQueueStarting function before decompressing 
// the frames in the queue. Your component is not required to implement 
// this function. It can implement the function if it needs to perform 
// any tasks at this time, such as locking data structures.
// The base image decompressor never calls the ImageCodecQueueStarting 
// function at interrupt time.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecQueueStarting(FFusionGlobals glob)
{	
    return noErr;
}

//-----------------------------------------------------------------
// ImageCodecQueueStopping
//-----------------------------------------------------------------
// If your image decompressor component supports asynchronous scheduled 
// decompression, the ImageCodecQueueStopping function notifies your 
// component that the frames in the queue have been decompressed. 
// Your component is not required to implement this function.
// After your image decompressor component handles an ImageCodecQueueStopping 
// call, it can perform any tasks that are required when decompression
// of the frames is finished, such as disposing of data structures that 
// are no longer needed. 
// The base image decompressor never calls the ImageCodecQueueStopping 
// function at interrupt time.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecQueueStopping(FFusionGlobals glob)
{	
    return noErr;
}

//-----------------------------------------------------------------
// ImageCodecGetCompressedImageSize
//-----------------------------------------------------------------
// Your component receives the ImageCodecGetCompressedImageSize request 
// whenever an application calls the ICM's GetCompressedImageSize function.
// You can use the ImageCodecGetCompressedImageSize function when you 
// are extracting a single image from a sequence; therefore, you don't have 
// an image description structure and don't know the exact size of one frame. 
// In this case, the Image Compression Manager calls the component to determine
// the size of the data. Your component should return a long integer indicating 
// the number of bytes of data in the compressed image. You may want to store
// the image size somewhere in the image description structure, so that you can 
// respond to this request quickly. Only decompressors receive this request.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecGetCompressedImageSize(FFusionGlobals glob, ImageDescriptionHandle desc, Ptr data, long dataSize, ICMDataProcRecordPtr dataProc, long *size)
{
    ImageFramePtr framePtr = (ImageFramePtr)data;
	
    if (size == NULL) 
		return paramErr;
	
    *size = EndianU32_BtoN(framePtr->frameSize) + sizeof(ImageFrame);
	
    return noErr;
}

//-----------------------------------------------------------------
// ImageCodecGetCodecInfo
//-----------------------------------------------------------------
// Your component receives the ImageCodecGetCodecInfo request whenever 
// an application calls the Image Compression Manager's GetCodecInfo 
// function.
// Your component should return a formatted compressor information 
// structure defining its capabilities.
// Both compressors and decompressors may receive this request.
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecGetCodecInfo(FFusionGlobals glob, CodecInfo *info)
{
    OSErr err = noErr;
	
    if (info == NULL) 
    {
        err = paramErr;
    }
    else 
    {
        CodecInfo **tempCodecInfo;
		
        switch (glob->componentType)
        {
            case 'MPG4':	// MS-MPEG4 v1
            case 'mpg4':
            case 'DIV1':
            case 'div1':
                err = GetComponentResource((Component)glob->self, codecInfoResourceType, kDivX1CodecInfoResID, (Handle *)&tempCodecInfo);
                break;
                
            case 'MP42':	// MS-MPEG4 v2
            case 'mp42':
            case 'DIV2':
            case 'div2':
                err = GetComponentResource((Component)glob->self, codecInfoResourceType, kDivX2CodecInfoResID, (Handle *)&tempCodecInfo);
                break;
                
            case 'div6':	// DivX 3
            case 'DIV6':
            case 'div5':
            case 'DIV5':
            case 'div4':
            case 'DIV4':
            case 'div3':
            case 'DIV3':
            case 'MP43':
            case 'mp43':
            case 'MPG3':
            case 'mpg3':
            case 'AP41':
            case 'COL0':
            case 'col0':
            case 'COL1':
            case 'col1':
                err = GetComponentResource((Component)glob->self, codecInfoResourceType, kDivX3CodecInfoResID, (Handle *)&tempCodecInfo);
                break;
				
            case 'divx':	// DivX 4
            case 'DIVX':
            case 'mp4s':
            case 'MP4S':
            case 'm4s2':
            case 'M4S2':
            case 0x04000000:
            case 'UMP4':
                err = GetComponentResource((Component)glob->self, codecInfoResourceType, kDivX4CodecInfoResID, (Handle *)&tempCodecInfo);
                break;
                
            case 'DX50':
                err = GetComponentResource((Component)glob->self, codecInfoResourceType, kDivX5CodecInfoResID, (Handle *)&tempCodecInfo);
                break;
                
            case 'XVID':	// XVID
            case 'xvid':
            case 'XviD':
            case 'XVIX':
            case 'BLZ0':
                err = GetComponentResource((Component)glob->self, codecInfoResourceType, kXVIDCodecInfoResID, (Handle *)&tempCodecInfo);
                break;
                
            case '3IVD':	// 3ivx
            case '3ivd':
            case '3IV2':
            case '3iv2':
                err = GetComponentResource((Component)glob->self, codecInfoResourceType, k3ivxCodecInfoResID, (Handle *)&tempCodecInfo);
                break;
				
			case 'RMP4':	// Miscellaneous
			case 'SEDG':
			case 'WV1F':
			case 'FMP4':
			case 'SMP4':
			case 'mp4v':
				err = GetComponentResource((Component)glob->self, codecInfoResourceType, kMPEG4CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'H264':	// H.264
			case 'h264':
			case 'X264':
			case 'x264':
			case 'AVC1':
			case 'avc1':
			case 'DAVC':
			case 'VSSH':
				err = GetComponentResource((Component)glob->self, codecInfoResourceType, kH264CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'FLV1':
				err = GetComponentResource((Component)glob->self, codecInfoResourceType, kFLV1CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'FSV1':
				err = GetComponentResource((Component)glob->self, codecInfoResourceType, kFlashSVCodecInfoResID, (Handle *)&tempCodecInfo);
				break;
			
			case 'VP60':
			case 'VP61':
			case 'VP62':
			case 'VP6F':
			case 'FLV4':
				err = GetComponentResource((Component)glob->self, codecInfoResourceType, kVP6CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'I263':
			case 'i263':
				err = GetComponentResource((Component)glob->self, codecInfoResourceType, kI263CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'VP30':
			case 'VP31':
				err = GetComponentResource((Component)glob->self, codecInfoResourceType, kVP3CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'FFVH':
			case 'HFYU':
				err = GetComponentResource((Component)glob->self, codecInfoResourceType, kHuffYUVCodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'MPEG':
			case 'mpg1':
			case 'mp1v':
				err = GetComponentResource((Component)glob->self, codecInfoResourceType, kMPEG1CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'MPG2':
			case 'mpg2':
			case 'mp2v':
				err = GetComponentResource((Component)glob->self, codecInfoResourceType, kMPEG2CodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'FPS1':
				err = GetComponentResource((Component)glob->self, codecInfoResourceType, kFRAPSCodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
			case 'SNOW':
				err = GetComponentResource((Component)glob->self, codecInfoResourceType, kSnowCodecInfoResID, (Handle *)&tempCodecInfo);
				break;
				
            default:	// should never happen but we have to handle the case
                err = GetComponentResource((Component)glob->self, codecInfoResourceType, kDivX4CodecInfoResID, (Handle *)&tempCodecInfo);
				
        }
        
        if (err == noErr) 
        {
            *info = **tempCodecInfo;
            
            DisposeHandle((Handle)tempCodecInfo);
        }
    }
	
    return err;
}

#define kSpoolChunkSize (16384)
#define kInfiniteDataSize (0x7fffffff)

static int FFusionGetBuffer(AVCodecContext *s, AVFrame *pic)
{
	FFusionGlobals glob = s->opaque;
	int ret = avcodec_default_get_buffer(s, pic);
	int i;
	
	if (ret >= 0) {
		for (i = 0; i < FFUSION_MAX_BUFFERS; i++) {
			if (!glob->buffers[i].used) {
				pic->opaque = &glob->buffers[i];
				glob->buffers[i].frame = pic;
				glob->buffers[i].used = true;
				glob->lastAllocatedBuffer = i;
				break;
			}
		}
	}
	
	return ret;
}

static void FFusionReleaseBuffer(AVCodecContext *s, AVFrame *pic)
{
//	FFusionGlobals glob = s->opaque;
	FFusionBuffer *buf = pic->opaque;
	
	buf->used = false;
	
	avcodec_default_release_buffer(s, pic);
}

//-----------------------------------------------------------------
// FFusionDecompress
//-----------------------------------------------------------------
// This function calls libavcodec to decompress one frame.
//-----------------------------------------------------------------

OSErr FFusionDecompress(AVCodecContext *context, UInt8 *dataPtr, ICMDataProcRecordPtr dataProc, long width, long height, AVFrame *picture, long length)
{
    OSErr err = noErr;
    int got_picture = false;
    int len = 0;
    long availableData = dataProc ? codecMinimumDataSize : kInfiniteDataSize;
	
    picture->data[0] = 0;
	
    while (!got_picture && length != 0) 
    {
        if (availableData < kSpoolChunkSize) 
        {
            // get some more source data
            
            err = InvokeICMDataUPP((Ptr *)&dataPtr, length, dataProc->dataRefCon, dataProc->dataProc);
			
            if (err == eofErr) err = noErr;
            if (err) return err;
			
            availableData = codecMinimumDataSize;
        }
		
        len = avcodec_decode_video(context, picture, &got_picture, dataPtr, length);
		
        if (len < 0)
        {            
            got_picture = 0;
            len = 1;
			Codecprintf(NULL, "Error while decoding frame\n");
            
            return noErr;
        }
        
        availableData -= len;
        dataPtr += len;
		length -= len;
    }
    return err;
}

static void SetupMultithreadedDecoding(AVCodecContext *s, enum CodecID codecID)
{
	int nthreads;
	size_t len = 4;
	
    // multithreading is only effective for mpeg1/2
    if (codecID != CODEC_ID_MPEG1VIDEO && codecID != CODEC_ID_MPEG2VIDEO) return;
    
	// one thread per usable CPU
	// (vmware or power saving may disable some CPUs)
	if (sysctlbyname("hw.activecpu", &nthreads, &len, NULL, 0) == -1) nthreads = 1;
	
	avcodec_thread_init(s, nthreads);
}
