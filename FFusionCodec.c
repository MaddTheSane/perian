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
#include <sys/sysctl.h>
#include <pthread.h>

#include "PerianResourceIDs.h"
#include "avcodec.h"
#include "Codecprintf.h"
#include "ColorConversions.h"
#include "bitstream_info.h"
#include "FrameBuffer.h"
#include "CommonUtils.h"
#include "CodecIDs.h"
#include "FFmpegUtils.h"

//---------------------------------------------------------------------------
// Types
//---------------------------------------------------------------------------

// 64 because that's 2 * ffmpeg's INTERNAL_BUFFER_SIZE and QT sometimes uses more than 32
#define FFUSION_MAX_BUFFERS 64

#define kNumPixelFormatsSupportedFFusion 1

typedef struct
{
	AVFrame		*frame;
	short		retainCount;
	short		ffmpegUsing;
	long		frameNumber;
	AVFrame		returnedFrame;
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

struct per_frame_decode_stats
{
	unsigned		begin_calls;
	unsigned		decode_calls;
	unsigned		draw_calls;
	unsigned		end_calls;
};

struct decode_stats
{
	struct per_frame_decode_stats type[4];
	int		max_frames_begun;
	int		max_frames_decoded;
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
	FILE			*fileLog;
	AVFrame			lastDisplayedFrame;
	FFusionPacked	packedType;
	FFusionBuffer	buffers[FFUSION_MAX_BUFFERS];	// the buffers which the codec has retained
	int				lastAllocatedBuffer;		// the index of the buffer which was last allocated 
												// by the codec (and is the latest in decode order)	
	int				shouldUseReturnedFrame;
	struct begin_glob	begin;
	FFusionData		data;
	struct decode_glob	decode;
	struct decode_stats stats;
	ColorConversionFuncs colorConv;
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

static OSErr FFusionDecompress(FFusionGlobals glob, AVCodecContext *context, UInt8 *dataPtr, long width, long height, AVFrame *picture, long length);
static int FFusionGetBuffer(AVCodecContext *s, AVFrame *pic);
static void FFusionReleaseBuffer(AVCodecContext *s, AVFrame *pic);
static void releaseBuffer(AVCodecContext *s, FFusionBuffer *buf);
static FFusionBuffer *retainBuffer(FFusionGlobals glob, FFusionBuffer *buf);

int GetPPUserPreference();
void SetPPUserPreference(int value);
pascal OSStatus HandlePPDialogWindowEvent(EventHandlerCallRef  nextHandler, EventRef theEvent, void* userData);
pascal OSStatus HandlePPDialogControlEvent(EventHandlerCallRef  nextHandler, EventRef theEvent, void* userData);
void ChangeHintText(int value, ControlRef staticTextField);

extern CFMutableStringRef CopyHomeDirectory();

#define FFusionDebugPrint(x...) if (glob->fileLog) Codecprintf(glob->fileLog, x);
#define not(x) ((x) ? "" : "not ")

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

static void *launchUpdateChecker(void *args)
{
	FSRef *ref = (FSRef *)args;
    LSOpenFSRef(ref, NULL);
	free(ref);
	return NULL;
}

Boolean FFusionAlreadyRanUpdateCheck = 0;

void FFusionRunUpdateCheck()
{
	if (FFusionAlreadyRanUpdateCheck) return;

    CFDateRef lastRunDate = CopyPreferencesValueTyped(CFSTR("NextRunDate"), CFDateGetTypeID());
    CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
    
	FFusionAlreadyRanUpdateCheck = 1;
	
	if (lastRunDate) {
		Boolean exit = CFDateGetAbsoluteTime(lastRunDate) > now;
		CFRelease(lastRunDate);
		if (exit) return;
	}
    
    //Two places to check, home dir and /
    
    CFMutableStringRef location = CopyHomeDirectory();
    CFStringAppend(location, CFSTR("/Library/PreferencePanes/Perian.prefPane/Contents/Resources/PerianUpdateChecker.app"));
    
    char fileRep[1024];
    FSRef *updateCheckRef = malloc(sizeof(FSRef));
    Boolean doCheck = FALSE;
    
    if(CFStringGetFileSystemRepresentation(location, fileRep, 1024))
        if(FSPathMakeRef((UInt8 *)fileRep, updateCheckRef, NULL) == noErr)
            doCheck = TRUE;
    
    CFRelease(location);
    if(doCheck == FALSE)
    {
        CFStringRef absLocation = CFSTR("/Library/PreferencePanes/Perian.prefPane/Contents/Resources/PerianUpdateChecker.app");
        if(CFStringGetFileSystemRepresentation(absLocation, fileRep, 1024))
            if(FSPathMakeRef((UInt8 *)fileRep, updateCheckRef, NULL) != noErr)
                goto err;  //We have failed
    }
	pthread_t thread;
	pthread_create(&thread, NULL, launchUpdateChecker, updateCheckRef);
	
	return;
err:
	free(updateCheckRef);
}

static void RecomputeMaxCounts(FFusionGlobals glob)
{
	int i;
	unsigned begun = 0, decoded = 0, ended = 0, drawn = 0;
	
	for (i = 0; i < 4; i++) {
		struct per_frame_decode_stats *f = &glob->stats.type[i];
		
		begun += f->begin_calls;
		decoded += f->decode_calls;
		drawn += f->draw_calls;
		ended += f->end_calls;
	}
	
	signed begin_diff = begun - ended, decode_diff = decoded - drawn;
	
	if (abs(begin_diff) > glob->stats.max_frames_begun) glob->stats.max_frames_begun = begin_diff;
	if (abs(decode_diff) > glob->stats.max_frames_decoded) glob->stats.max_frames_decoded = decode_diff;
}

static void DumpFrameDropStats(FFusionGlobals glob)
{
	static const char types[4] = {'?', 'I', 'P', 'B'};
	int i;
	
	if (!glob->fileLog || glob->decode.lastFrame == 0) return;
	
	Codecprintf(glob->fileLog, "%p frame drop stats\nType\t| BeginBand\t| DecodeBand\t| DrawBand\t| dropped before decode\t| dropped before draw\n", glob);
	
	for (i = 0; i < 4; i++) {
		struct per_frame_decode_stats *f = &glob->stats.type[i];
				
		Codecprintf(glob->fileLog, "%c\t| %d\t\t| %d\t\t| %d\t\t| %d/%f%%\t\t| %d/%f%%\n", types[i], f->begin_calls, f->decode_calls, f->draw_calls,
					f->begin_calls - f->decode_calls,(f->begin_calls > f->decode_calls) ? ((float)(f->begin_calls - f->decode_calls)/(float)f->begin_calls) * 100. : 0.,
					f->decode_calls - f->draw_calls,(f->decode_calls > f->draw_calls) ? ((float)(f->decode_calls - f->draw_calls)/(float)f->decode_calls) * 100. : 0.);
	}
}

static enum PixelFormat FindPixFmtFromVideo(AVCodec *codec, AVCodecContext *avctx, Ptr data, int bufferSize)
{
    AVCodecContext tmpContext;
    AVFrame tmpFrame;
    int got_picture;
    enum PixelFormat pix_fmt;
    
    avcodec_get_context_defaults2(&tmpContext, CODEC_TYPE_VIDEO);
	avcodec_get_frame_defaults(&tmpFrame);
    tmpContext.width = avctx->width;
    tmpContext.height = avctx->height;
	tmpContext.flags = avctx->flags;
	tmpContext.bits_per_coded_sample = avctx->bits_per_coded_sample;
    tmpContext.codec_tag = avctx->codec_tag;
	tmpContext.codec_id  = avctx->codec_id;
	tmpContext.extradata = avctx->extradata;
	tmpContext.extradata_size = avctx->extradata_size;
	
    avcodec_open(&tmpContext, codec);
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = (UInt8*)data;
	pkt.size = bufferSize;
    avcodec_decode_video2(&tmpContext, &tmpFrame, &got_picture, &pkt);
    pix_fmt = tmpContext.pix_fmt;
    avcodec_close(&tmpContext);
    
    return pix_fmt;
}

static void SetupMultithreadedDecoding(AVCodecContext *s, enum CodecID codecID)
{
	int nthreads = 1;
	size_t len = 4;
	
    // multithreading is only effective for mpeg1/2 and h.264 with slices
    if (codecID != CODEC_ID_MPEG1VIDEO && codecID != CODEC_ID_MPEG2VIDEO && codecID != CODEC_ID_H264) return;
    
	// two threads on multicore, otherwise 1
	if (sysctlbyname("hw.activecpu", &nthreads, &len, NULL, 0) == -1) nthreads = 1;
	else nthreads = FFMIN(nthreads, 2);
	
	avcodec_thread_init(s, nthreads);
}

static void SetSkipLoopFilter(FFusionGlobals glob, AVCodecContext *avctx)
{
	Boolean keyExists = FALSE;
	int loopFilterSkip = CFPreferencesGetAppIntegerValue(CFSTR("SkipLoopFilter"), PERIAN_PREF_DOMAIN, &keyExists);
	if(keyExists)
	{
		enum AVDiscard loopFilterValue = AVDISCARD_DEFAULT;
		switch (loopFilterSkip) {
			case 1:
				loopFilterValue = AVDISCARD_NONREF;
				break;
			case 2:
				loopFilterValue = AVDISCARD_BIDIR;
				break;
			case 3:
				loopFilterValue = AVDISCARD_NONKEY;
				break;
			case 4:
				loopFilterValue = AVDISCARD_ALL;
				break;
			default:
				break;
		}
		avctx->skip_loop_filter = loopFilterValue;
		FFusionDebugPrint("%p Preflight set skip loop filter to %d", glob, loopFilterValue);
	}
}

// A list of codec types (mostly official Apple codecs) which always have DTS info.
// This is the wrong way to do it, instead we should check for a ctts atom directly.
// This way causes files to play frames out of order if we guess wrong. Doesn't seem
// possible to do it right, though.
FFusionPacked DefaultPackedTypeForCodec(OSType codec)
{
	switch (codec) {
		case kMPEG1VisualCodecType:
		case kMPEG2VisualCodecType:
		case 'hdv1':
		case kMPEG4VisualCodecType:
		case kH264CodecType:
			return PACKED_QUICKTIME_KNOWS_ORDER;
		default:
			return PACKED_ALL_IN_FIRST_FRAME;
	}
}

static void swapFrame(AVFrame * *a, AVFrame * *b)
{
	AVFrame *t = *a;
	*a = *b;
	*b = t;
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
		CFStringRef pathToLogFile = CopyPreferencesValueTyped(CFSTR("DebugLogFile"), CFStringGetTypeID());
		char path[PATH_MAX];
        SetComponentInstanceStorage(self, (Handle)glob);

        glob->self = self;
        glob->target = self;
        glob->drawBandUPP = NULL;
        glob->pixelTypes = NewHandleClear((kNumPixelFormatsSupportedFFusion+1) * sizeof(OSType));
        glob->avCodec = 0;
        glob->componentType = descout.componentSubType;
		glob->data.frames = NULL;
		glob->begin.parser = NULL;
		if (pathToLogFile) {
			CFStringGetCString(pathToLogFile, path, PATH_MAX, kCFStringEncodingUTF8);
			CFRelease(pathToLogFile);
			glob->fileLog = fopen(path, "a");
		}
		glob->shouldUseReturnedFrame = 0;
		
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
		
		// we allocate some space for copying the frame data since we need some padding at the end
		// for ffmpeg's optimized bitstream readers. Size doesn't really matter, it'll grow if need be
		FFusionDataSetup(&(glob->data), 256, 1024*1024);
        FFusionRunUpdateCheck();
    }
    
    FFusionDebugPrint("%p opened for '%s'\n", glob, FourCCString(glob->componentType));
    return err;
}

//-----------------------------------------------------------------
// Component Close Request - Required
//-----------------------------------------------------------------

pascal ComponentResult FFusionCodecClose(FFusionGlobals glob, ComponentInstance self)
{
    FFusionDebugPrint("%p closed.\n", glob);
	DumpFrameDropStats(glob);

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
				
        if (glob->avContext)
        {
			if (glob->avContext->extradata)
				free(glob->avContext->extradata);
						
			if (glob->avContext->codec) avcodec_close(glob->avContext);
            av_free(glob->avContext);
        }
		
		if (glob->begin.parser)
		{
			ffusionParserFree(glob->begin.parser);
		}
		
		if (glob->pixelTypes)
		{
			DisposeHandle(glob->pixelTypes);
		}
		
		FFusionDataFree(&(glob->data));
        
		if(glob->fileLog)
			fclose(glob->fileLog);
		
        memset(glob, 0, sizeof(FFusionGlobalsRecord));
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
	Boolean doExperimentalFlags = CFPreferencesGetAppBooleanValue(CFSTR("ExperimentalQTFlags"), PERIAN_PREF_DOMAIN, NULL);
	
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
		cap->baseCodecShouldCallDecodeBandForAllFrames = !doExperimentalFlags;
		cap->subCodecSupportsScheduledBackwardsPlaybackWithDifferenceFrames = true;
		cap->subCodecSupportsDrawInDecodeOrder = doExperimentalFlags;
		cap->subCodecSupportsDecodeSmoothing = true; 
	}
	
    return noErr;
}

static inline int shouldDecode(FFusionGlobals glob, enum CodecID codecID)
{
	FFusionDecodeAbilities decode = FFUSION_PREFER_DECODE;
	if (glob->componentType == 'avc1')
		decode = ffusionIsParsedVideoDecodable(glob->begin.parser);
	if(decode > FFUSION_CANNOT_DECODE && 
	   (codecID == CODEC_ID_H264 || codecID == CODEC_ID_MPEG4) && CFPreferencesGetAppBooleanValue(CFSTR("PreferAppleCodecs"), PERIAN_PREF_DOMAIN, NULL))
		decode = FFUSION_PREFER_NOT_DECODE;
	if(decode > FFUSION_CANNOT_DECODE)
		if(IsForcedDecodeEnabled())
			decode = FFUSION_PREFER_DECODE;
	return decode > FFUSION_PREFER_NOT_DECODE;
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
	long count = 0;
	Handle imgDescExt;
	OSErr err = noErr;
	
    // We first open libavcodec library and the codec corresponding
    // to the fourCC if it has not been done before
    
	FFusionDebugPrint("%p Preflight called.\n", glob);
	FFusionDebugPrint("%p Frame dropping is %senabled\n", glob, not(IsFrameDroppingEnabled()));
	
    if (!glob->avCodec)
    {
		FFInitFFmpeg();
		initFFusionParsers();

		OSType componentType = glob->componentType;
		enum CodecID codecID = getCodecID(componentType);
		
		glob->packedType = DefaultPackedTypeForCodec(componentType);
		
		if(componentType == 'VP30' || componentType == 'VP31')
			glob->shouldUseReturnedFrame = TRUE;

		if(codecID == CODEC_ID_NONE)
		{
			Codecprintf(glob->fileLog, "Warning! Unknown codec type! Using MPEG4 by default.\n");
			codecID = CODEC_ID_MPEG4;
		}
		
		glob->avCodec = avcodec_find_decoder(codecID);
//		if(glob->packedType != PACKED_QUICKTIME_KNOWS_ORDER)
			glob->begin.parser = ffusionParserInit(codecID);
                
		if ((codecID == CODEC_ID_MPEG4 || codecID == CODEC_ID_H264) && !glob->begin.parser)
			Codecprintf(glob->fileLog, "This is a parseable format, but we couldn't open a parser!\n");
		
        // we do the same for the AVCodecContext since all context values are
        // correctly initialized when calling the alloc function
        
        glob->avContext = avcodec_alloc_context2(CODEC_TYPE_VIDEO);
		
		// Use low delay
		glob->avContext->flags |= CODEC_FLAG_LOW_DELAY;
		
        // Image size is mandatory for DivX-like codecs
        
        glob->avContext->width = (**p->imageDescription).width;
        glob->avContext->height = (**p->imageDescription).height;
		glob->avContext->bits_per_coded_sample = (**p->imageDescription).depth;
		
        // We also pass the FourCC since it allows the H263 hybrid decoder
        // to make the difference between the various flavours of DivX
        glob->avContext->codec_tag = Endian32_Swap(glob->componentType);
		glob->avContext->codec_id  = codecID;
        
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
		
		FFusionDebugPrint("%p preflighted for %dx%d '%s'. (%d bytes of extradata)\n", glob, (**p->imageDescription).width, (**p->imageDescription).height, FourCCString(glob->componentType), glob->avContext->extradata_size);
        
		if(glob->avContext->extradata_size != 0 && glob->begin.parser != NULL)
			ffusionParseExtraData(glob->begin.parser, glob->avContext->extradata, glob->avContext->extradata_size);
		
		if (glob->fileLog)
			ffusionLogDebugInfo(glob->begin.parser, glob->fileLog);
		
		if (!shouldDecode(glob, codecID))
			err = featureUnsupported;
		
		// some hooks into ffmpeg's buffer allocation to get frames in 
		// decode order without delay more easily
		glob->avContext->opaque = glob;
		glob->avContext->get_buffer = FFusionGetBuffer;
		glob->avContext->release_buffer = FFusionReleaseBuffer;
		
		// multi-slice decoding
		SetupMultithreadedDecoding(glob->avContext, codecID);
		
		// deblock skipping for h264
		SetSkipLoopFilter(glob, glob->avContext);
		
		//fast flag
		if (CFPreferencesGetAppBooleanValue(CFSTR("UseFastDecode"), PERIAN_PREF_DOMAIN, NULL))
			glob->avContext->flags2 |= CODEC_FLAG2_FAST;
		
        // Finally we open the avcodec 
        
        if (avcodec_open(glob->avContext, glob->avCodec))
        {
            Codecprintf(glob->fileLog, "Error opening avcodec!\n");
            
			err = paramErr;
        }
		
        // codec was opened, but didn't give us its pixfmt
		// we have to decode the first frame to find out one
		else if (glob->avContext->pix_fmt == PIX_FMT_NONE && p->bufferSize && p->data)
            glob->avContext->pix_fmt = FindPixFmtFromVideo(glob->avCodec, glob->avContext, p->data, p->bufferSize);
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
    
    pos = *((OSType **)glob->pixelTypes);
    
    index = 0;
	
	if (!err) {
		OSType qtPixFmt = ColorConversionDstForPixFmt(glob->avContext->pix_fmt);
		
		/*
		 an error here means either
		 1) a color converter for this format isn't implemented
		 2) we know QT doesn't like this format and will give us argb 32bit instead
		 
		 in the case of 2 we have to special-case bail right here, since errors
		 in BeginBand are ignored
		 */
		if (qtPixFmt)
			pos[index++] = qtPixFmt;
		else
			err = featureUnsupported;
	}
	
    p->wantedDestinationPixelTypes = (OSType **)glob->pixelTypes;
    
    // Specify the number of pixels the image must be extended in width and height if
    // the component cannot accommodate the image at its given width and height
    // It is not the case here
    
    capabilities->extendWidth = 0;
    capabilities->extendHeight = 0;
    
	capabilities->flags |= codecCanAsync | codecCanAsyncWhen;
	
	FFusionDebugPrint("%p Preflight requesting colorspace '%s'. (error %d)\n", glob, FourCCString(pos[0]), err);
	
    return err;
}

static int qtTypeForFrameInfo(int original, int fftype, int skippable)
{
	if(fftype == FF_I_TYPE)
	{
		if(!skippable)
			return kCodecFrameTypeKey;
	}
	else if(skippable && IsFrameDroppingEnabled())
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
    FFusionDecompressRecord *myDrp = (FFusionDecompressRecord *)drp->userDecompressRecord;
	int redisplayFirstFrame = 0;
	int type = 0;
	int skippable = 0;
	
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

    myDrp->width = (**p->imageDescription).width;
    myDrp->height = (**p->imageDescription).height;
    myDrp->depth = (**p->imageDescription).depth;
	
    myDrp->pixelFormat = p->dstPixMap.pixelFormat;
	myDrp->decoded = p->frameTime ? (0 != (p->frameTime->flags & icmFrameAlreadyDecoded)) : false;
	myDrp->frameData = NULL;
	myDrp->buffer = NULL;
	
	FFusionDebugPrint("%p BeginBand #%d. (%sdecoded, packed %d)\n", glob, p->frameNumber, not(myDrp->decoded), glob->packedType);
	
	if (!glob->avContext) {
		Codecprintf(glob->fileLog, "Perian: QT tried to call BeginBand without preflighting!\n");
		return internalComponentErr;
	}
	
	if (p->frameNumber == 0 && myDrp->pixelFormat != ColorConversionDstForPixFmt(glob->avContext->pix_fmt)) {
		Codecprintf(glob->fileLog, "QT gave us unwanted pixelFormat %s (%08x), this will not work\n", FourCCString(myDrp->pixelFormat), myDrp->pixelFormat);
	}
	
	if(myDrp->decoded)
	{
		int i;
		myDrp->frameNumber = p->frameNumber;
		for (i = 0; i < FFUSION_MAX_BUFFERS; i++) {
			if (glob->buffers[i].retainCount && glob->buffers[i].frameNumber == myDrp->frameNumber) {
				myDrp->buffer = retainBuffer(glob, &glob->buffers[i]);
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
				return noErr;
			}
		}
		else
		{
			/* Reset context, safe marking in such a case */
			glob->begin.lastFrameType = FF_I_TYPE;
			FFusionDataReadUnparsed(&(glob->data));
			glob->begin.lastPFrameData = NULL;
			redisplayFirstFrame = 1;
		}
	}
	
	if(glob->begin.parser != NULL)
	{
		int parsedBufSize = 0;
		uint8_t *buffer = (uint8_t *)drp->codecData;
		int bufferSize = p->bufferSize;
		int skipped = 0;
		
		if(glob->data.unparsedFrames.dataSize != 0)
		{
			buffer = glob->data.unparsedFrames.buffer;
			bufferSize = glob->data.unparsedFrames.dataSize;
		}
		
		if(ffusionParse(glob->begin.parser, buffer, bufferSize, &parsedBufSize, &type, &skippable, &skipped) == 0 && (!skipped || glob->begin.futureType))
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
				Codecprintf(glob->fileLog, "parse failed frame %d with size %d\n", p->frameNumber, bufferSize);
				if(glob->data.unparsedFrames.dataSize != 0)
					Codecprintf(glob->fileLog, ", parser had extra data\n");				
			}
		}
		else if(glob->packedType != PACKED_QUICKTIME_KNOWS_ORDER)
		{
			if(type == FF_B_TYPE && glob->packedType == PACKED_ALL_IN_FIRST_FRAME && glob->begin.futureType == 0)
				/* Badly framed.  We hit a B frame after it was supposed to be displayed, switch to delaying by a frame */
				glob->packedType = PACKED_DELAY_BY_ONE_FRAME;
			else if(glob->packedType == PACKED_DELAY_BY_ONE_FRAME && parsedBufSize < bufferSize - 16)
				/* Seems to be switching back to packed in one frame; switch back*/
				glob->packedType = PACKED_ALL_IN_FIRST_FRAME;
			
			myDrp->frameData = FFusionDataAppend(&(glob->data), buffer, parsedBufSize, type);
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
				int success = ffusionParse(glob->begin.parser, buffer, bufferSize, &parsedBufSize, &type, &skippable, &skipped);
				if(success && type == FF_B_TYPE)
				{
					/* A B frame follows us, so setup the P frame for the future and set dependencies */
					glob->begin.futureType = oldType;
					if(glob->begin.lastPFrameData != NULL)
						/* Mark the next P or I frame, predictive decoding */
						glob->begin.lastPFrameData->nextFrame = myDrp->frameData;
					glob->begin.lastPFrameData = myDrp->frameData;
					myDrp->frameData = FFusionDataAppend(&(glob->data), buffer, parsedBufSize, type);
					myDrp->frameData->prereqFrame = glob->begin.lastPFrameData;
					buffer += parsedBufSize;
					bufferSize -= parsedBufSize;
				}
				if(bufferSize > 0)
					FFusionDataSetUnparsed(&(glob->data), buffer, bufferSize);
				else
					FFusionDataReadUnparsed(&(glob->data));
			}
			else
				FFusionDataReadUnparsed(&(glob->data));
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
	
	glob->stats.type[drp->frameType].begin_calls++;
	RecomputeMaxCounts(glob);
	FFusionDebugPrint("%p BeginBand: frame #%d type %d. (%sskippable)\n", glob, myDrp->frameNumber, type, not(skippable));
	
    return noErr;
}

static OSErr PrereqDecompress(FFusionGlobals glob, FrameData *prereq, AVCodecContext *context, long width, long height, AVFrame *picture)
{
	FFusionDebugPrint("%p prereq-decompressing frame #%d.\n", glob, prereq->frameNumber);
	
	FrameData *preprereq = FrameDataCheckPrereq(prereq);
	if(preprereq)
		PrereqDecompress(glob, preprereq, context, width, height, picture);
	
	unsigned char *dataPtr = (unsigned char *)prereq->buffer;
	int dataSize = prereq->dataSize;
	
	OSErr err = FFusionDecompress(glob, context, dataPtr, width, height, picture, dataSize);
	
	return err;
}

pascal ComponentResult FFusionCodecDecodeBand(FFusionGlobals glob, ImageSubCodecDecompressRecord *drp, unsigned long flags)
{
	OSErr err = noErr;
	AVFrame tempFrame;

    FFusionDecompressRecord *myDrp = (FFusionDecompressRecord *)drp->userDecompressRecord;
	
	glob->stats.type[drp->frameType].decode_calls++;
	RecomputeMaxCounts(glob);
	FFusionDebugPrint("%p DecodeBand #%d qtType %d. (packed %d)\n", glob, myDrp->frameNumber, drp->frameType, glob->packedType);
	
	// QuickTime will drop H.264 frames when necessary if a sample dependency table exists
	// we don't want to flush buffers in that case.
	if(glob->packedType != PACKED_QUICKTIME_KNOWS_ORDER && myDrp->frameNumber != glob->decode.lastFrame + 1)
	{
		/* Skipped some frames in here */
		FFusionDebugPrint("%p - frames skipped.\n", glob);
		if(drp->frameType == kCodecFrameTypeKey || myDrp->GOPStartFrameNumber > glob->decode.lastFrame || myDrp->frameNumber < glob->decode.lastFrame)
		{
			/* If this is a key frame or the P frame before us is after the last frame (skip ahead), or we are before the last decoded frame (skip back) *
			 * then we are in a whole new GOP */
			avcodec_flush_buffers(glob->avContext);
		}
	}
	
	if(myDrp->frameData && myDrp->frameData->decoded && glob->decode.futureBuffer != NULL)
	{
		myDrp->buffer = retainBuffer(glob, glob->decode.futureBuffer);
		myDrp->decoded = true;
#if 0	/* Need to make sure this frame's data is not eradicated during the decompress */
		FrameData *nextFrame = myDrp->frameData->nextFrame;
		if(nextFrame != NULL)
		{
			FFusionDecompress(glob, glob->avContext, nextFrame->buffer, NULL, myDrp->width, myDrp->height, &tempFrame, nextFrame->dataSize);
			if(tempFrame.data[0] != NULL)
			{
				glob->decode.futureBuffer = (FFusionBuffer *)tempFrame.opaque;
				nextFrame->decoded = TRUE;
			}
		}
		else
#endif
			glob->decode.futureBuffer = NULL;
		FFusionDataMarkRead(myDrp->frameData);
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
			PrereqDecompress(glob, prereq, glob->avContext, myDrp->width, myDrp->height, &tempFrame);
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
		dataPtr = FFusionCreateEntireDataBuffer(&(glob->data), (uint8_t *)drp->codecData, dataSize);
	}
		
	err = FFusionDecompress(glob, glob->avContext, dataPtr, myDrp->width, myDrp->height, &tempFrame, dataSize);
			
	if (glob->packedType == PACKED_QUICKTIME_KNOWS_ORDER) {
		myDrp->buffer = &glob->buffers[glob->lastAllocatedBuffer];
		myDrp->buffer->frameNumber = myDrp->frameNumber;
		retainBuffer(glob, myDrp->buffer);
		myDrp->buffer->returnedFrame = tempFrame;
		myDrp->decoded = true;
		glob->decode.lastFrame = myDrp->frameNumber;
		return err;
	}
	if(tempFrame.data[0] == NULL)
		myDrp->buffer = NULL;
	else
		myDrp->buffer = retainBuffer(glob, (FFusionBuffer *)tempFrame.opaque);
	
	if(tempFrame.pict_type == FF_I_TYPE)
		/* Wipe memory of past P frames */
		glob->decode.futureBuffer = NULL;
	glob->decode.lastFrame = myDrp->frameNumber;
	myDrp->decoded = true;
	if (myDrp->buffer) myDrp->buffer->returnedFrame = tempFrame;
	
	FFusionDataMarkRead(frameData);
	
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
	AVFrame *picture;
	
	glob->stats.type[drp->frameType].draw_calls++;
	RecomputeMaxCounts(glob);
	FFusionDebugPrint("%p DrawBand #%d. (%sdecoded)\n", glob, myDrp->frameNumber, not(myDrp->decoded));
	
	if(!myDrp->decoded) {
		err = FFusionCodecDecodeBand(glob, drp, 0);

		if (err) goto err;
	}
	
	if (myDrp->buffer)
	{
		picture = myDrp->buffer->frame;
	}
	else
		picture = &glob->lastDisplayedFrame;
	
	if(!picture || picture->data[0] == 0)
	{
		if(glob->shouldUseReturnedFrame && myDrp->buffer &&
		   myDrp->buffer->returnedFrame.data[0])
			//Some decoders (vp3) keep their internal buffers in an unusable state
			picture = &myDrp->buffer->returnedFrame;
		else if(glob->lastDisplayedFrame.data[0] != NULL)
			//Display last frame
			picture = &glob->lastDisplayedFrame;
		else {
			//Display black (no frame decoded yet)

			if (!glob->colorConv.clear) {
				err = ColorConversionFindFor(&glob->colorConv, glob->avContext->pix_fmt, NULL, myDrp->pixelFormat);
				if (err) goto err;
			}
			
			glob->colorConv.clear((UInt8*)drp->baseAddr, drp->rowBytes, myDrp->width, myDrp->height);
			return noErr;
		}
	}
	else
	{
		if (myDrp->buffer)
			glob->lastDisplayedFrame = *picture;
	}
	
	if (!glob->colorConv.convert) {
		err = ColorConversionFindFor(&glob->colorConv, glob->avContext->pix_fmt, picture, myDrp->pixelFormat);
		if (err) goto err;
	}
	
	glob->colorConv.convert(picture, (UInt8*)drp->baseAddr, drp->rowBytes, myDrp->width, myDrp->height);
	
err:
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
	FFusionDecompressRecord *myDrp = (FFusionDecompressRecord *)drp->userDecompressRecord;
	glob->stats.type[drp->frameType].end_calls++;
	FFusionBuffer *buf = myDrp->buffer;
	if(buf && buf->frame)
		releaseBuffer(glob->avContext, buf);
	
	FFusionDebugPrint("%p EndBand #%d.\n", glob, myDrp->frameNumber);
	
    return noErr;
}

// Gamma curve value for FFusion video.
ComponentResult FFusionCodecGetSourceDataGammaLevel(FFusionGlobals glob, Fixed *sourceDataGammaLevel)
{
	enum AVColorTransferCharacteristic color_trc = AVCOL_TRC_UNSPECIFIED;
	float gamma;
	
	if (glob->avContext)
		color_trc = glob->avContext->color_trc;
	
	switch (color_trc) {
		case AVCOL_TRC_GAMMA28:
			gamma = 2.8;
			break;
		case AVCOL_TRC_GAMMA22:
			gamma = 2.2;
			break;
		default: // absolutely everything in the world will reach here
			gamma = 1/.45; // and GAMMA22 is probably a typo for this anyway
	}
	
	*sourceDataGammaLevel = FloatToFixed(gamma);
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
	return getPerianCodecInfo(glob->self, glob->componentType, info);
}

static int FFusionGetBuffer(AVCodecContext *s, AVFrame *pic)
{
	FFusionGlobals glob = s->opaque;
	int ret = avcodec_default_get_buffer(s, pic);
	int i;
	
	if (ret >= 0) {
		for (i = 0; i < FFUSION_MAX_BUFFERS; i++) {
			if (!glob->buffers[i].retainCount) {
//				FFusionDebugPrint("%p Starting Buffer %p.\n", glob, &glob->buffers[i]);
				pic->opaque = &glob->buffers[i];
				glob->buffers[i].frame = pic;
				glob->buffers[i].retainCount = 1;
				glob->buffers[i].ffmpegUsing = 1;
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
	
	if(buf->ffmpegUsing)
	{
		buf->ffmpegUsing = 0;
		releaseBuffer(s, buf);
	}
}

static FFusionBuffer *retainBuffer(FFusionGlobals glob, FFusionBuffer *buf)
{
	buf->retainCount++;
//	FFusionDebugPrint("%p Retained Buffer %p #%d to %d.\n", glob, buf, buf->frameNumber, buf->retainCount);
	return buf;
}

static void releaseBuffer(AVCodecContext *s, FFusionBuffer *buf)
{
	buf->retainCount--;
//	FFusionGlobals glob = (FFusionGlobals)s->opaque;
//	FFusionDebugPrint("%p Released Buffer %p #%d to %d.\n", glob, buf, buf->frameNumber, buf->retainCount);
	if(!buf->retainCount && !buf->ffmpegUsing)
	{
		buf->returnedFrame.data[0] = NULL;
		avcodec_default_release_buffer(s, buf->frame);
	}
}

//-----------------------------------------------------------------
// FFusionDecompress
//-----------------------------------------------------------------
// This function calls libavcodec to decompress one frame.
//-----------------------------------------------------------------

OSErr FFusionDecompress(FFusionGlobals glob, AVCodecContext *context, UInt8 *dataPtr, long width, long height, AVFrame *picture, long length)
{
    OSErr err = noErr;
    int got_picture = false;
    int len = 0;
	
	FFusionDebugPrint("%p Decompress %d bytes.\n", glob, length);
    avcodec_get_frame_defaults(picture);
	
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = dataPtr;
	pkt.size = length;
	len = avcodec_decode_video2(context, picture, &got_picture, &pkt);
	
	if (len < 0)
	{            
		Codecprintf(glob->fileLog, "Error while decoding frame\n");
	} 
	
	return err;
}
