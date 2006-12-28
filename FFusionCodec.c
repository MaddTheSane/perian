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
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//---------------------------------------------------------------------------
// Source Code
//---------------------------------------------------------------------------

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

#include "FFusionCodec.h"
#include "EI_Image.h"
#include "avcodec.h"
#include "postprocess.h"
#include "Codecprintf.h"

void inline swapFrame(AVFrame * *a, AVFrame * *b)
{
	AVFrame *t = *a;
	*a = *b;
	*b = t;
}

//---------------------------------------------------------------------------
// Types
//---------------------------------------------------------------------------

typedef struct
{
    pp_mode_t			*mode[PP_QUALITY_MAX+1];
    pp_context_t		*context;
    short			level;
	AVFrame			*destBuffer;
} PostProcParamRecord;

// 32 because that's ffmpeg's INTERNAL_BUFFER_SIZE
#define FFUSION_MAX_BUFFERS 32
typedef struct
{
	AVFrame		*frame;
	bool		used;
} FFusionBuffer;

typedef struct
{
    ComponentInstance		self;
    ComponentInstance		delegateComponent;
    ComponentInstance		target;
    ImageCodecMPDrawBandUPP 	drawBandUPP;
    Handle			pixelTypes;
    AVCodec			*avCodec;
    AVCodecContext	*avContext;
    AVFrame			*picture;
	AVFrame			*futureFrame;
	int				lastFrameNumber;
	uint64_t		lastFramePts;
    OSType			componentType;
    char			hasy420;
    char			alreadyDonePPPref;
    PostProcParamRecord		postProcParams;
	FILE			*fileLog;
	int				futureFrameAvailable;
	int				delayedFrames;
	AVFrame			lastDisplayedFrame;
	bool			quicktimeDoesReorder;		// QT will call DrawBand() in display order
	FFusionBuffer	buffers[FFUSION_MAX_BUFFERS];	// the buffers which the codec has retained
	int				lastAllocatedBuffer;		// the index of the buffer which was last allocated 
												// by the codec (and is the latest in decode order)
	uint8_t			*inputBuffer;
	unsigned int	inputBufSize;
} FFusionGlobalsRecord, *FFusionGlobals;

typedef struct
{
    long			width;
    long			height;
    long			depth;
    OSType			pixelFormat;
    long			bufferSize;
	int				decoded;
	long			frameNumber;
	int				useFuture;
	FFusionBuffer	*buffer;
} FFusionDecompressRecord;


//---------------------------------------------------------------------------
// Prototypes of private subroutines
//---------------------------------------------------------------------------

static OSErr FFusionDecompress(AVCodecContext *context, UInt8 *dataPtr, ICMDataProcRecordPtr dataProc, long width, long height, AVFrame *picture, long length, int useFirstFrameHack);
static void FastY420(UInt8 *baseAddr, AVFrame *picture);
static void SlowY420(UInt8 *baseAddr, long rowBump, long width, long height, AVFrame *picture);
static void BGR24toRGB24(UInt8 *baseAddr, long rowBump, long width, long height, AVFrame *picture);
static int FFusionGetBuffer(AVCodecContext *s, AVFrame *pic);
static void FFusionReleaseBuffer(AVCodecContext *s, AVFrame *pic);

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
	
    FourCCprintf("Opening component for type ", descout.componentSubType);
	
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
		glob->lastFrameNumber = 0;
		glob->lastFramePts = 0;
        glob->alreadyDonePPPref = 0;
        glob->componentType = descout.componentSubType;
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
                Codecprintf(glob->fileLog, "Use speedy y420 component\n");
                glob->hasy420 = 1;
            }
        }
        else
        {
            Codecprintf(glob->fileLog, "Use slow y420 component\n");
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
    }
    
    return err;
}

//-----------------------------------------------------------------
// Component Close Request - Required
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecClose(FFusionGlobals glob, ComponentInstance self)
{
    // Make sure to close the base component and dealocate our storage
    int i;
    
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
		
		if (glob->postProcParams.destBuffer)
		{
			av_free(glob->postProcParams.destBuffer);
		}
        
        if (glob->avCodec)
        {
            avcodec_close(glob->avContext);
        }
		
        if (glob->picture)
        {
            av_free(glob->picture);
        }
		
		if (glob->futureFrame)
		{
			av_free(glob->futureFrame);
		}
		
        if (glob->avContext)
        {
			if (glob->avContext->extradata)
				free(glob->avContext->extradata);
			
            av_free(glob->avContext);
        }
		
		if (glob->pixelTypes)
		{
			DisposeHandle(glob->pixelTypes);
		}
        
		if (glob->inputBuffer)
		{
			av_free(glob->inputBuffer);
		}
		
        for (i=0; i<=PP_QUALITY_MAX; i++)
        {
            if (glob->postProcParams.mode[i])
                pp_free_mode(glob->postProcParams.mode[i]);
        }
		
        if (glob->postProcParams.context)
            pp_free_context(glob->postProcParams.context);
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
	
    cap->decompressRecordSize = sizeof(FFusionDecompressRecord);
    cap->canAsync = true;
	
	// QT 7
	if(cap->recordSize > offsetof(ImageSubCodecDecompressCapabilities, baseCodecShouldCallDecodeBandForAllFrames))
	{
		cap->subCodecIsMultiBufferAware = true;
		cap->subCodecSupportsOutOfOrderDisplayTimes = true;
		cap->baseCodecShouldCallDecodeBandForAllFrames = true;
		cap->subCodecSupportsScheduledBackwardsPlaybackWithDifferenceFrames = true;
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
    int index, i;
    CodecCapabilities *capabilities = p->capabilities;
    long bitfield;
    char altivec = 0;
    Byte* myptr;
	long count = 0;
	Handle imgDescExt;
	
    // We first open libavcodec library and the codec corresponding
    // to the fourCC if it has not been done before
    
    if (!glob->avCodec)
    {
		initLib();
		
        switch (glob->componentType)
        {
            case 'MPG4':	// MS-MPEG4 v1
            case 'mpg4':
            case 'DIV1':
            case 'div1':
                glob->avCodec = avcodec_find_decoder(CODEC_ID_MSMPEG4V1);
				break;
				
            case 'MP42':	// MS-MPEG4 v2
            case 'mp42':
            case 'DIV2':
            case 'div2':
                glob->avCodec = avcodec_find_decoder(CODEC_ID_MSMPEG4V2);
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
                glob->avCodec = avcodec_find_decoder(CODEC_ID_MSMPEG4V3);
				break;
				
			case 'mp4v':	// MPEG4 part 2 in mov/mp4
				glob->quicktimeDoesReorder = true;
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
                glob->avCodec = avcodec_find_decoder(CODEC_ID_MPEG4);
				break;

			case 'avc1':	// H.264 in mov/mp4/mkv
				glob->quicktimeDoesReorder = true;
			case 'H264':	// H.264 in AVI
			case 'h264':
			case 'X264':
			case 'x264':
			case 'AVC1':
			case 'DAVC':
			case 'VSSH':
				glob->avCodec = avcodec_find_decoder(CODEC_ID_H264);
				break;

			case 'FLV1':
				glob->avCodec = avcodec_find_decoder(CODEC_ID_FLV1);
				break;

			case 'FSV1':
				glob->avCodec = avcodec_find_decoder(CODEC_ID_FLASHSV);
				break;

			case 'VP60':
			case 'VP61':
			case 'VP62':
				glob->avCodec = avcodec_find_decoder(CODEC_ID_VP6);
				break;

			case 'VP6F':
				glob->avCodec = avcodec_find_decoder(CODEC_ID_VP6F);
				break;

			case 'I263':
			case 'i263':
				glob->avCodec = avcodec_find_decoder(CODEC_ID_H263I);
				break;

			case 'VP30':
			case 'VP31':
				glob->avCodec = avcodec_find_decoder(CODEC_ID_VP3);
				break;

            default:
			Codecprintf(glob->fileLog, "Warning! Unknown codec type! Using MPEG4 by default.\n");
                
                glob->avCodec = avcodec_find_decoder(CODEC_ID_MPEG4);
        }
        
        // libavcodec version 0.4.6 and higher uses AVFrame instead of AVPicture
        // It is better to handle the frame with a pointer and to use built-in
        // allocation function
        
        glob->picture = avcodec_alloc_frame();
		glob->futureFrame = avcodec_alloc_frame();
        
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
			CountImageDescriptionExtensionType(p->imageDescription, 'avcC', &count);
			
			if (count >= 1) {
				imgDescExt = NewHandle(0);
				GetImageDescriptionExtension(p->imageDescription, &imgDescExt, 'avcC', 1);
				
				glob->avContext->extradata = calloc(1, GetHandleSize(imgDescExt) + FF_INPUT_BUFFER_PADDING_SIZE);
				memcpy(glob->avContext->extradata, *imgDescExt, GetHandleSize(imgDescExt));
				glob->avContext->extradata_size = GetHandleSize(imgDescExt);
				
				DisposeHandle(imgDescExt);
			}
		} else if (glob->componentType == 'mp4v') {
			CountImageDescriptionExtensionType(p->imageDescription, 'esds', &count);
			
			if (count >= 1) {
				imgDescExt = NewHandle(0);
				GetImageDescriptionExtension(p->imageDescription, &imgDescExt, 'esds', 1);
				
				ReadESDSDescExt(imgDescExt, &glob->avContext->extradata, &glob->avContext->extradata_size);
				
				DisposeHandle(imgDescExt);
			}
		}
		
		// some hooks into ffmpeg's buffer allocation to get frames in 
		// decode order without delay more easily
		glob->avContext->opaque = glob;
		glob->avContext->get_buffer = FFusionGetBuffer;
		glob->avContext->release_buffer = FFusionReleaseBuffer;
		
        // Finally we open the avcodec 
        
        if (avcodec_open(glob->avContext, glob->avCodec))
        {
            Codecprintf(glob->fileLog, "Error opening avcodec!\n");
            
            return -2;
        }
		
		// we allocate some space for copying the frame data since we need some padding at the end
		// for ffmpeg's optimized bitstream readers. Size doesn't really matter, it'll grow if need be
		glob->inputBuffer = av_malloc(64*1024);
		glob->inputBufSize = 64*1024;
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
    
    // Post-processing
	if (!glob->postProcParams.context)
	{
		// allocate a destination buffer so we don't mess with the decoded data
		glob->postProcParams.destBuffer = avcodec_alloc_frame();
		avcodec_default_get_buffer(glob->avContext, glob->postProcParams.destBuffer);
		
		glob->postProcParams.context = pp_get_context((**p->imageDescription).width, (**p->imageDescription).height, PP_FORMAT_420);
		
		for (i=0; i<=PP_QUALITY_MAX; i++) 
		{
			
			if (i <= 2) 
			{
				glob->postProcParams.mode[i] = pp_get_mode_by_name_and_quality("h1,v1,dr"/*"al:f"*/, i);
			} 
			else if (i <= 4) 
			{
				glob->postProcParams.mode[i] = pp_get_mode_by_name_and_quality("hb,vb,dr,"/*"al:f"*/, i);
			} 
			else 
			{
				glob->postProcParams.mode[i] = pp_get_mode_by_name_and_quality("hb,vb,dr,""hb:c,vb:c,dr:c,"/*"al:f"*/, i);
			}
			
			if (glob->postProcParams.mode[i] == NULL) 
			{
				Codecprintf(glob->fileLog, "Error getting PP filter %d!\n", i);
				
				return -1;
			}
			
			glob->postProcParams.level = 0;//GetPPUserPreference();
		}
	}
	capabilities->flags |= codecCanAsync | codecCanAsyncWhen;
	
    
    return noErr;
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
	
    //////
    IBNibRef 		nibRef;
    WindowRef 		window;
    OSStatus		err;
    CFBundleRef		bundleRef;
    EventHandlerUPP  	handlerUPP, handlerUPP2;
    EventTypeSpec    	eventType;
    ControlID		controlID;
    ControlRef		theControl;
    KeyMap		currentKeyMap;
    int			userPreference;
    ///////
	
    offsetH = (long)(p->dstRect.left - p->dstPixMap.bounds.left) * (long)(p->dstPixMap.pixelSize >> 3);
    offsetV = (long)(p->dstRect.top - p->dstPixMap.bounds.top) * (long)drp->rowBytes;
    
    myDrp->width = (**p->imageDescription).width;
    myDrp->height = (**p->imageDescription).height;
    myDrp->depth = (**p->imageDescription).depth;
    myDrp->bufferSize = p->bufferSize;			// bufferSize is the data size of the current frame
	
    myDrp->pixelFormat = p->dstPixMap.pixelFormat;
	myDrp->decoded = p->frameTime ? (0 != (p->frameTime->flags & icmFrameAlreadyDecoded)) : false;
	myDrp->frameNumber = p->frameNumber - 1;
	myDrp->buffer = NULL;
	
    if (p->conditionFlags & codecConditionFirstFrame)
    {
        GetKeys(currentKeyMap);
        
		/*        if ((currentKeyMap[1] & kOptionKeyModifier) && !glob->alreadyDonePPPref)
        {
            glob->alreadyDonePPPref = 1;
			
            bundleRef = CFBundleGetBundleWithIdentifier(CFSTR("net.aldorande.component.FFusion"));
			
            if (bundleRef == NULL)
                printf("Cannot get main bundle reference\n");
			
            err = CreateNibReferenceWithCFBundle(bundleRef, CFSTR("main"), &nibRef);
            
            if (err != noErr)
                printf("cannot create nib reference! %d\n", (int)err);
            
            if (nibRef != NULL)
            {
                err = CreateWindowFromNib(nibRef, CFSTR("PostProcessing"), &window);
                
                if (err != noErr)
                    printf("cannot create window!\n");
                
                DisposeNibReference(nibRef);
				
                if (window != NULL)
                {
                    controlID.signature = 'post';
                    controlID.id = 128;
                    
                    err = GetControlByID(window, &controlID, &theControl);
                    
                    if (err != noErr)
                    {
                        printf("Cannot get slider hint text control!\n");
                    }
					
                    userPreference = GetPPUserPreference();
                    ChangeHintText(userPreference, theControl);
                    
                    controlID.signature = 'post';
                    controlID.id = 129;
                    
                    err = GetControlByID(window, &controlID, &theControl);
                    
                    if (err != noErr)
                    {
                        printf("Cannot get slider control!\n");
                    }
					
                    SetControl32BitValue(theControl, userPreference);
					
                    ShowWindow(window);
                    
                    handlerUPP = NewEventHandlerUPP(HandlePPDialogWindowEvent);
                    
                    eventType.eventClass = kEventClassCommand;
                    eventType.eventKind  = kEventCommandProcess;
                    
                    InstallWindowEventHandler(window, handlerUPP, 1, &eventType, window, NULL);
                    
                    handlerUPP2 = NewEventHandlerUPP(HandlePPDialogControlEvent);
                    
                    eventType.eventClass = kEventClassControl;
                    eventType.eventKind = kEventControlTrack;
                    
                    InstallControlEventHandler(theControl, handlerUPP2, 1, &eventType, window, NULL);
                }
            }
        }*/
    }
    
    /*if ((glob->postProcParams.level >= 0) && (GetPPUserPreference() > 0)) 
    {
        if (p->conditionFlags & codecConditionCatchUpDiff) 
        {
            if (glob->postProcParams.level > 0) 
            {
                glob->postProcParams.level--;
                
                printf("PP=%i -\n", glob->postProcParams.level);
            }
            
            glob->postProcParams.goodness = 0;
        } 
        else 
        {
            if (++glob->postProcParams.goodness > 5) 
            {
                if (glob->postProcParams.level < PP_QUALITY_MAX + 1) 
                {
                    glob->postProcParams.level++;
                    
                    printf("PP=%i +\n", glob->postProcParams.level);
                }
                
                glob->postProcParams.goodness = 0;
            }
        }
    }*/
	
    return noErr;
}

pascal ComponentResult FFusionCodecDecodeBand(FFusionGlobals glob, ImageSubCodecDecompressRecord *drp, unsigned long flags)
{
	OSErr err = noErr;
	int useFirstFrameHack = 0;
	AVFrame tempFrame;
	
    FFusionDecompressRecord *myDrp = (FFusionDecompressRecord *)drp->userDecompressRecord;
	
	// QuickTime will drop H.264 frames when necessary if a sample dependency table exists
	// we don't want to flush buffers in that case.
	if(myDrp->frameNumber != glob->lastFrameNumber + 1 && !glob->quicktimeDoesReorder)
	{
		avcodec_flush_buffers(glob->avContext);
		glob->futureFrameAvailable = false;
		glob->lastFramePts = 0;
		if(glob->delayedFrames != 0)
		{
			//Hack to force display of first frame
			glob->avContext->flags |= CODEC_FLAG_LOW_DELAY;
			useFirstFrameHack = 1;
		}
	}
	glob->lastFrameNumber = myDrp->frameNumber;
	
	unsigned char *dataPtr = (unsigned char *)drp->codecData;
	ICMDataProcRecordPtr dataProc = drp->dataProcRecord.dataProc ? &drp->dataProcRecord : NULL;
	
	glob->inputBuffer = av_fast_realloc(glob->inputBuffer, &glob->inputBufSize, myDrp->bufferSize + FF_INPUT_BUFFER_PADDING_SIZE);
	if (glob->inputBuffer) {
		dataPtr = glob->inputBuffer;
		memcpy(dataPtr, drp->codecData, myDrp->bufferSize);
		memset(dataPtr + myDrp->bufferSize, 0, FF_INPUT_BUFFER_PADDING_SIZE);
	}
	
	avcodec_get_frame_defaults(&tempFrame);
	err = FFusionDecompress(glob->avContext, dataPtr, dataProc, myDrp->width, myDrp->height, &tempFrame, myDrp->bufferSize, useFirstFrameHack);
	myDrp->useFuture = false;
	
	if (glob->quicktimeDoesReorder) {
		myDrp->buffer = &glob->buffers[glob->lastAllocatedBuffer];
		myDrp->decoded = true;
		return err;
	}
	
	if(tempFrame.pts < glob->lastFramePts && glob->delayedFrames == 0)
	{
		//Badly framed...  No choice but to delay frames by 1
		glob->delayedFrames = 1;
		swapFrame(&glob->picture, &glob->futureFrame);
		memcpy(glob->picture, &tempFrame, sizeof(AVFrame));
	}
	else if(glob->delayedFrames == 1)
	{
		memcpy(glob->picture, &tempFrame, sizeof(AVFrame));
		if(tempFrame.pict_type == FF_I_TYPE)
		{
			glob->avContext->flags &= ~CODEC_FLAG_LOW_DELAY;
			avcodec_flush_buffers(glob->avContext);
			err = FFusionDecompress(glob->avContext, dataPtr, dataProc, myDrp->width, myDrp->height, glob->picture, myDrp->bufferSize, 1);
			glob->delayedFrames = 2;
		}
		else if(tempFrame.pict_type != FF_B_TYPE)
		{
			swapFrame(&glob->picture, &glob->futureFrame);
			glob->futureFrameAvailable = 1;
		}
		glob->lastFramePts = glob->picture->pts;
	}
	else
	{
		memcpy(glob->picture, &tempFrame, sizeof(AVFrame));
		glob->lastFramePts = glob->picture->pts;	
	}
	
	if(glob->picture->data[0] == NULL && glob->futureFrameAvailable)
	{
		myDrp->useFuture = true;
		glob->futureFrameAvailable = false;
		glob->lastFramePts = glob->futureFrame->pts;
	}
	else if(glob->picture->pict_type == FF_P_TYPE && glob->delayedFrames == 0 && err == noErr)
	{
		//Check to see if a B-Frame follows this
		unsigned char nullChars[8] = {0,0,0,0, 0,0,0,0};
		err = FFusionDecompress(glob->avContext, nullChars, NULL, myDrp->width, myDrp->height, glob->futureFrame, 8, 0);
		if(glob->futureFrame->data[0] != NULL)
		{
			//We found a B-frame in here
			AVFrame *tframe = glob->picture;
			glob->picture = glob->futureFrame;
			glob->futureFrame = tframe;
			
			glob->futureFrameAvailable = true;
			glob->lastFramePts = glob->picture->pts;
		}
	}
	myDrp->decoded = true;
	
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
	AVFrame *ppDestPic = glob->postProcParams.destBuffer;
	int i, j;
	
	if(!myDrp->decoded)
		err = FFusionCodecDecodeBand(glob, drp, 0);
	
	AVFrame *picture;
	
	if (glob->quicktimeDoesReorder) {
		if (myDrp->buffer)
			picture = myDrp->buffer->frame;
		else
			picture = &glob->lastDisplayedFrame;
	} else {
		if(myDrp->useFuture)
			picture = glob->futureFrame;
		else
			picture = glob->picture;
	}
	
	if(picture->data[0] == 0)
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
		
		// only do post processing if it's a new frame
		if (glob->postProcParams.level > 0)
		{
			pp_postprocess(picture->data, picture->linesize, 
						   ppDestPic->data, ppDestPic->linesize, 
						   myDrp->width, myDrp->height, 
						   picture->qscale_table, picture->qstride, 
						   glob->postProcParams.mode[glob->postProcParams.level],
						   glob->postProcParams.context, picture->pict_type);
			picture = ppDestPic;
		}
	}
	
	if (myDrp->pixelFormat == 'y420' && glob->avContext->pix_fmt == PIX_FMT_YUV420P)
	{
		FastY420((UInt8 *)drp->baseAddr, picture);
	}
	else if (myDrp->pixelFormat == k2vuyPixelFormat && glob->avContext->pix_fmt == PIX_FMT_YUV420P)
	{
		SlowY420((UInt8 *)drp->baseAddr, drp->rowBytes, myDrp->width, myDrp->height, picture);
	}
	else if (myDrp->pixelFormat == k24RGBPixelFormat && glob->avContext->pix_fmt == PIX_FMT_BGR24)
	{
		BGR24toRGB24((UInt8 *)drp->baseAddr, drp->rowBytes, myDrp->width, myDrp->height, picture);
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
	FFusionGlobals glob = s->opaque;
	FFusionBuffer *buf = pic->opaque;
	
	buf->used = false;
	
	avcodec_default_release_buffer(s, pic);
}

//-----------------------------------------------------------------
// FFusionDecompress
//-----------------------------------------------------------------
// This function calls libavcodec to decompress one frame.
//-----------------------------------------------------------------

OSErr FFusionDecompress(AVCodecContext *context, UInt8 *dataPtr, ICMDataProcRecordPtr dataProc, long width, long height, AVFrame *picture, long length, int useFirstFrameHack)
{
    OSErr err = noErr;
    int got_picture = false;
    int len = 0;
    long availableData = dataProc ? codecMinimumDataSize : kInfiniteDataSize;
	
    picture->data[0] = 0;
	
	if(useFirstFrameHack)
	{
		//Hack to first display of first frame when not using low delay
		context->flags |= CODEC_FLAG_LOW_DELAY;
	}
	
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
		
		if(useFirstFrameHack)
		{
			//clean up after hack
			context->flags &= ~CODEC_FLAG_LOW_DELAY;
			avcodec_flush_buffers(context);
			len = avcodec_decode_video(context, picture, &got_picture, dataPtr, length);
		}
        
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

//-----------------------------------------------------------------
// FastY420
//-----------------------------------------------------------------
// Returns y420 data directly to QuickTime which then converts
// in RGB for display
//-----------------------------------------------------------------

static void FastY420(UInt8 *baseAddr, AVFrame *picture)
{
    PlanarPixmapInfoYUV420 *planar;
	
	/*From Docs: PixMap baseAddr points to a big-endian PlanarPixmapInfoYUV420 struct; see ImageCodec.i. */
    planar = (PlanarPixmapInfoYUV420 *) baseAddr;
    
    // if ya can't set da poiners, set da offsets
    planar->componentInfoY.offset = EndianU32_NtoB(picture->data[0] - baseAddr);
    planar->componentInfoCb.offset =  EndianU32_NtoB(picture->data[1] - baseAddr);
    planar->componentInfoCr.offset =  EndianU32_NtoB(picture->data[2] - baseAddr);
    
    // for the 16/32 add look at EDGE in mpegvideo.c
    planar->componentInfoY.rowBytes = EndianU32_NtoB(picture->linesize[0]);
    planar->componentInfoCb.rowBytes = EndianU32_NtoB(picture->linesize[1]);
    planar->componentInfoCr.rowBytes = EndianU32_NtoB(picture->linesize[2]);
}

//-----------------------------------------------------------------
// FFusionSlowDecompress
//-----------------------------------------------------------------
// We have to return 2yuv values because
// QT version has no built-in y420 component.
// Since we do the conversion ourselves it is not really optimized....
// The function should never be called since many people now
// have a decent OS/QT version.
//-----------------------------------------------------------------

static void SlowY420(UInt8 *baseAddr, long rowBump, long width, long height, AVFrame *picture)
{
    unsigned int i, j;
    char *yuvPtr;
    char *py,*pu,*pv;
	
    // now let's do some yuv420/vuy2 conversion
    
    yuvPtr = (char *)baseAddr;
    py = (char *)picture->data[0];
    pu = (char *)picture->data[1];
    pv = (char *)picture->data[2];
	
    for(i = 0 ;  i < height; i++)
    {
        for(j = 0; j < width; j+= 2)
        {
            yuvPtr[2*j] = pu[j>>1];
            yuvPtr[2*j+1] = py[j];
            yuvPtr[2*j+2] = pv[j>>1];
            yuvPtr[2*j+3] = py[j+1];
        }
        
        yuvPtr += rowBump;
        py += picture->linesize[0];
        
        if (i & 1)
        {
            pu += picture->linesize[1];
            pv += picture->linesize[2];
        }
    }
}

static void BGR24toRGB24(UInt8 *baseAddr, long rowBump, long width, long height, AVFrame *picture)
{
	unsigned int i, j;
	UInt8 *srcPtr = picture->data[0];

	for (i = 0; i < height; ++i)
	{
		for (j = 0; j < width * 3; j += 3)
		{
			baseAddr[j] = srcPtr[j+2];
			baseAddr[j+1] = srcPtr[j+1];
			baseAddr[j+2] = srcPtr[j];
		}
		baseAddr += rowBump;
		srcPtr += picture->linesize[0];
	}
}

int GetPPUserPreference()
{
    CFIndex 	userPref;
    Boolean 	keyExists;
    int		ppUserValue = 0;
    
    userPref = CFPreferencesGetAppIntegerValue(CFSTR("postProcessingLevel"), CFSTR("net.aldorande.component.FFusion"), &keyExists);
    
    if (keyExists)
    {
        ppUserValue = (int)userPref;
    }
    else
    {
        Codecprintf(NULL, "Key does not exists\n");
        
        ppUserValue = 0;
    }
    
    return ppUserValue;
}

void SetPPUserPreference(int value)
{
    int	    sameValue = value;
    Boolean syncOK;
	
    CFPreferencesSetAppValue(CFSTR("postProcessingLevel"), CFNumberCreate(NULL, kCFNumberIntType, &sameValue), CFSTR("net.aldorande.component.FFusion"));
    
    syncOK = CFPreferencesAppSynchronize(CFSTR("net.aldorande.component.FFusion"));
    
    if (!syncOK)
    {
        Codecprintf(NULL, "Error writing user preference!\n");
    }
}

pascal OSStatus HandlePPDialogWindowEvent(EventHandlerCallRef  nextHandler, EventRef theEvent, void* userData)
{
    UInt32	whatHappened;
    HICommand 	commandStruct;
    UInt32 	theCommandID;
    OSErr	theErr = eventNotHandledErr;
    OSErr	resErr;
    WindowRef	window = (WindowRef)userData;
    SInt32	value = 0;
    ControlRef	slider;
    ControlID	controlID;
    
    whatHappened = GetEventKind(theEvent);
	
    switch (whatHappened)
    {
        case kEventCommandProcess:
            GetEventParameter(theEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &commandStruct);
			
            theCommandID = commandStruct.commandID;
            
            if (theCommandID == kHICommandOK)
            {
                controlID.id = 129;
                controlID.signature = 'post';
                resErr = GetControlByID(window, &controlID, &slider);
                
                if (resErr == noErr)
                {
                    value = GetControl32BitValue(slider);
                }
                else
                {
                    Codecprintf(NULL, "Unable to get post-processing control value!\n");
                }
                
                SetPPUserPreference(value);
				
                DisposeWindow(window);
                
                theErr = noErr;
            }
				
				if (theCommandID == kHICommandCancel)
				{                
					DisposeWindow(window);
					
					theErr = noErr;
				}
				
				break;
    }
    
    return theErr;
}

pascal OSStatus HandlePPDialogControlEvent(EventHandlerCallRef nextHandler, EventRef theEvent, void* userData)
{
    WindowRef	window = (WindowRef)userData;
    ControlID	controlID;
    ControlRef	slider, staticText;
    OSErr	resErr;
    SInt32	value;
    OSErr	result;
    
    result = CallNextEventHandler(nextHandler, theEvent);
    
    if (result == noErr)
    {        
        controlID.id = 128;
        controlID.signature = 'post';
        resErr = GetControlByID(window, &controlID, &staticText);
        
        controlID.id = 129;
        controlID.signature = 'post';
        resErr = GetControlByID(window, &controlID, &slider);
        
        value = GetControl32BitValue(slider);
        
        ChangeHintText(value, staticText);
    }
    
    return result;
}

void ChangeHintText(int value, ControlRef staticTextField)
{
    CFStringRef myCFSTR, myIndexCFSTR;
    OSErr	resErr;
    CFBundleRef	bundleRef;
    
    bundleRef = CFBundleGetBundleWithIdentifier(CFSTR("net.aldorande.component.FFusion"));
    
    if (bundleRef == NULL)
    {
        Codecprintf(NULL, "Error getting current bundle!\n");
    }
    
    if (myIndexCFSTR = CFStringCreateWithFormatAndArguments(NULL, NULL, CFSTR("%d"), (va_list)&value))
    {
        myCFSTR = CFBundleCopyLocalizedString(bundleRef, myIndexCFSTR, NULL, CFSTR("PostProcessing"));
    }
    else
    {
        myCFSTR = CFBundleCopyLocalizedString(bundleRef, CFSTR("0"), NULL, CFSTR("PostProcessing"));
    }
	
    //resErr = SetControlData(staticTextField, kControlEntireControl, kControlStaticTextTextTag, CFStringGetLength(myCFSTR), CFStringGetCStringPtr(myCFSTR, CFStringGetSystemEncoding()));
    
    //this works only under 10.2
    resErr = SetControlData(staticTextField, kControlEntireControl, kControlStaticTextCFStringTag, CFStringGetLength(myCFSTR), &myCFSTR);
	
    if (resErr != noErr)
        Codecprintf(NULL, "Could not change control title! (%d) \n", (int)resErr);
    
    Draw1Control(staticTextField);
}
