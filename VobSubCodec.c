
#if __MACH__
    #include <Carbon/Carbon.h>
    #include <QuickTime/QuickTime.h>
#else
    #include <ConditionalMacros.h>
    #include <Endian.h>
    #include <ImageCodec.h>
#endif

#include "VobSubCodec.h"

// Constants
const UInt8 kNumPixelFormatsSupportedVobSub = 1;

// Data structures
typedef struct	{
	ComponentInstance		self;
	ComponentInstance		delegateComponent;
	ComponentInstance		target;
	OSType**				wantedDestinationPixelTypeH;
	ImageCodecMPDrawBandUPP drawBandUPP;
    
    UInt32                  palette[16];
    
} VobSubCodecGlobalsRecord, *VobSubCodecGlobals;

typedef struct {
	long		width;
	long		height;
	long		depth;
} VobSubDecompressRecord;

typedef struct {
    // color format is 32-bit ARGB
    UInt32  pixelColor[4];
    UInt16  startCol;
    UInt16  endCol;
    UInt16  startLine;
    UInt16  endLine;
    UInt16  firstScanPos;
    UInt16  secondScanPos;
} PacketControlData;

#if defined(NDEBUG)
#define dprintf(...) {}
#else
#define dprintf printf
#endif

// Setup required for ComponentDispatchHelper.c
#define IMAGECODEC_BASENAME() 		VobSubCodec
#define IMAGECODEC_GLOBALS() 		VobSubCodecGlobals storage

#define CALLCOMPONENT_BASENAME()	IMAGECODEC_BASENAME()
#define	CALLCOMPONENT_GLOBALS()		IMAGECODEC_GLOBALS()

#define COMPONENT_UPP_PREFIX()		uppImageCodec
#define COMPONENT_DISPATCH_FILE		"VobSubCodecDispatch.h"
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

ComponentResult SetupColorPalette(VobSubCodecGlobals glob, ImageDescriptionHandle imageDescription);
ComponentResult DecompressPacket(VobSubCodecGlobals glob, ImageSubCodecDecompressRecord *drp);
ComponentResult ProcessControlSequence(UInt8 *controlSeq, UInt32 palette[16], 
                                       PacketControlData *controlDataOut);
// sets data pointer to the next byte after the line decoded
void DecodeLine(UInt8 **data, UInt32 *framePtr, PacketControlData controlData);
void PrintPix(UInt16 word, UInt32 **frame, UInt32 palette[4]);

void ExtractData(UInt8 *framedSrc, UInt8 *dest, int srcSize, int destSize);

/* -- This Image Decompressor User the Base Image Decompressor Component --
	The base image decompressor is an Apple-supplied component
	that makes it easier for developers to create new decompressors.
	The base image decompressor does most of the housekeeping and
	interface functions required for a QuickTime decompressor component,
	including scheduling for asynchronous decompression.
*/

// Component Open Request - Required
pascal ComponentResult VobSubCodecOpen(VobSubCodecGlobals glob, ComponentInstance self)
{
	ComponentResult err;
	// Allocate memory for our globals, set them up and inform the component manager that we've done so
	glob = (VobSubCodecGlobals)NewPtrClear(sizeof(VobSubCodecGlobalsRecord));
	if (err = MemError()) goto bail;

	SetComponentInstanceStorage(self, (Handle)glob);

	glob->self = self;
	glob->target = self;
	glob->wantedDestinationPixelTypeH = (OSType **)NewHandle(sizeof(OSType) * (kNumPixelFormatsSupportedVobSub + 1));
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
pascal ComponentResult VobSubCodecClose(VobSubCodecGlobals glob, ComponentInstance self)
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

		DisposePtr((Ptr)glob);
	}

	return noErr;
}

// Component Version Request - Required
pascal ComponentResult VobSubCodecVersion(VobSubCodecGlobals glob)
{
#pragma unused(glob)
	
    return kVobSubCodecVersion;
}

// Component Target Request
// 		Allows another component to "target" you i.e., you call another component whenever
// you would call yourself (as a result of your component being used by another component)
pascal ComponentResult VobSubCodecTarget(VobSubCodecGlobals glob, ComponentInstance target)
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
pascal ComponentResult VobSubCodecGetMPWorkFunction(VobSubCodecGlobals glob, ComponentMPWorkFunctionUPP *workFunction, void **refCon)
{
	if (glob->drawBandUPP == NULL)
		glob->drawBandUPP = NewImageCodecMPDrawBandUPP((ImageCodecMPDrawBandProcPtr)VobSubCodecDrawBand);
	
	return ImageCodecGetBaseMPWorkFunction(glob->delegateComponent, workFunction, refCon, glob->drawBandUPP, glob);
}

#pragma mark-

// ImageCodecInitialize
//		The first function call that your image decompressor component receives from the base image
// decompressor is always a call to ImageCodecInitialize . In response to this call, your image decompressor
// component returns an ImageSubCodecDecompressCapabilities structure that specifies its capabilities.
pascal ComponentResult VobSubCodecInitialize(VobSubCodecGlobals glob, ImageSubCodecDecompressCapabilities *cap)
{
#pragma unused(glob)
	
	// Secifies the size of the ImageSubCodecDecompressRecord structure
	// and say we can support asyncronous decompression
	// With the help of the base image decompressor, any image decompressor
	// that uses only interrupt-safe calls for decompression operations can
	// support asynchronous decompression.
	cap->decompressRecordSize = sizeof(VobSubDecompressRecord);
	cap->canAsync = true;
	
	if(cap->recordSize > offsetof(ImageSubCodecDecompressCapabilities, baseCodecShouldCallDecodeBandForAllFrames)) 
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
pascal ComponentResult VobSubCodecPreflight(VobSubCodecGlobals glob, CodecDecompressParams *p)
{
	CodecCapabilities *capabilities = p->capabilities;
	OSTypePtr         formats = *glob->wantedDestinationPixelTypeH;
	UInt8             depth = (**p->imageDescription).depth;

	// Fill in formats for wantedDestinationPixelTypeH
	// Terminate with an OSType value 0  - see IceFloe #7
	// http://developer.apple.com/quicktime/icefloe/dispatch007.html
	
    if (depth == 24) depth = 32;
	
    *formats++	= depth;
	*formats++	= 0;
	
	// Specify the minimum image band height supported by the component
	// bandInc specifies a common factor of supported image band heights - 
	// if your component supports only image bands that are an even
    // multiple of some number of pixels high report this common factor in bandInc
	capabilities->bandMin = (**p->imageDescription).height;
	capabilities->bandInc = capabilities->bandMin;

	// Indicate the wanted destination using the wantedDestinationPixelTypeH previously set up
	capabilities->wantedPixelSize  = 0; 	
    
    // we want 4:4:4:4 ARGB cause that's what the pixel colors are stored as
    **glob->wantedDestinationPixelTypeH = k32ARGBPixelFormat;
    
	p->wantedDestinationPixelTypes = glob->wantedDestinationPixelTypeH;

	// Specify the number of pixels the image must be extended in width and height if
	// the component cannot accommodate the image at its given width and height
	capabilities->extendWidth = 0;
	capabilities->extendHeight = 0;
	
	// get the color palette info from the image description
    SetupColorPalette(glob, p->imageDescription);

	return noErr;
}

// ImageCodecBeginBand
// 		The ImageCodecBeginBand function allows your image decompressor component to save information about
// a band before decompressing it. This function is never called at interrupt time. The base image decompressor
// preserves any changes your component makes to any of the fields in the ImageSubCodecDecompressRecord
// or CodecDecompressParams structures. If your component supports asynchronous scheduled decompression, it
// may receive more than one ImageCodecBeginBand call before receiving an ImageCodecDrawBand call.
pascal ComponentResult VobSubCodecBeginBand(VobSubCodecGlobals glob, CodecDecompressParams *p, ImageSubCodecDecompressRecord *drp, long flags)
{
	//long offsetH, offsetV;
	VobSubDecompressRecord *myDrp = (VobSubDecompressRecord *)drp->userDecompressRecord;

#if 0
	offsetH = (long)(p->dstRect.left - p->dstPixMap.bounds.left) * (long)(p->dstPixMap.pixelSize >> 3);
	offsetV = (long)(p->dstRect.top - p->dstPixMap.bounds.top) * (long)drp->rowBytes;

	drp->baseAddr = p->dstPixMap.baseAddr + offsetH + offsetV;
#endif
	
	// Let base codec know that all our frames are key frames (a.k.a., sync samples)
	// This allows the base codec to perform frame dropping on our behalf if needed 
    drp->frameType = kCodecFrameTypeKey;

	myDrp->width = (**p->imageDescription).width;
	myDrp->height = (**p->imageDescription).height;
	myDrp->depth = (**p->imageDescription).depth;
    
    // remove the framing information if needed
    UInt8 *data = (UInt8 *) drp->codecData;
    // the header of a spu PS packet starts 0x000001bd
    // if it's raw spu data, the 1st 2 bytes are the length of the data
    if (data[0] + data[1] == 0) {
        dprintf(" Extacting spu data from PS packets %d\n");
        
        data = (UInt8 *) NewPtr(p->bufferSize);
        memcpy((void *) data, (void *) drp->codecData, GetPtrSize((Ptr) data));
        
        ExtractData(data, (UInt8 *) drp->codecData, GetPtrSize((Ptr) data), 
                    p->bufferSize);
        
        DisposePtr((Ptr) data);
    }
    
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
pascal ComponentResult VobSubCodecDrawBand(VobSubCodecGlobals glob, ImageSubCodecDecompressRecord *drp)
{
	OSErr err = noErr;
	VobSubDecompressRecord *myDrp = (VobSubDecompressRecord *)drp->userDecompressRecord;
	//unsigned char *dataPtr = (unsigned char *)drp->codecData;
	//ICMDataProcRecordPtr dataProc = drp->dataProcRecord.dataProc ? &drp->dataProcRecord : NULL;
    
	// clear the buffer to pure transparent
	memset(drp->baseAddr, 0, myDrp->height * drp->rowBytes);
	
    DecompressPacket(glob, drp);
    
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
pascal ComponentResult VobSubCodecEndBand(VobSubCodecGlobals glob, ImageSubCodecDecompressRecord *drp, OSErr result, long flags)
{
#pragma unused(glob, drp,result, flags)
	
	return noErr;
}

// ImageCodecQueueStarting
// 		If your component supports asynchronous scheduled decompression, the base image decompressor calls your image decompressor component's
// ImageCodecQueueStarting function before decompressing the frames in the queue. Your component is not required to implement this function.
// It can implement the function if it needs to perform any tasks at this time, such as locking data structures.
// The base image decompressor never calls the ImageCodecQueueStarting function at interrupt time.
pascal ComponentResult VobSubCodecQueueStarting(VobSubCodecGlobals glob)
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
pascal ComponentResult VobSubCodecQueueStopping(VobSubCodecGlobals glob)
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
pascal ComponentResult VobSubCodecGetCompressedImageSize(VobSubCodecGlobals glob, ImageDescriptionHandle desc, Ptr data, long dataSize, ICMDataProcRecordPtr dataProc, long *size)
{
#pragma	unused(glob,desc,dataSize,dataProc)
	
	if (size == NULL) 
		return paramErr;

//    *size = EndianU32_BtoN(framePtr->frameSize);
        
	return unimpErr;
}

// ImageCodecGetCodecInfo
//		Your component receives the ImageCodecGetCodecInfo request whenever an application calls the Image Compression Manager's GetCodecInfo function.
// Your component should return a formatted compressor information structure defining its capabilities.
// Both compressors and decompressors may receive this request.
pascal ComponentResult VobSubCodecGetCodecInfo(VobSubCodecGlobals glob, CodecInfo *info)
{
	OSErr err = noErr;
	if (info == NULL) {
		err = paramErr;
	} else {
		CodecInfo **tempCodecInfo;

		err = GetComponentResource((Component)glob->self, codecInfoResourceType, kVobSubCodecResource, (Handle *)&tempCodecInfo);
		if (err == noErr) {
			*info = **tempCodecInfo;
			DisposeHandle((Handle)tempCodecInfo);
		}
	}

	return err;
}

#pragma mark-

//****** RLE Decompression Routines ******

// Someday you may find yourself on an architecture where misaligned reads
// are ultra-expensive. On that day you can cheerfully adjust these macros to compensate.

#define Get32(x)		(*(long*)(x))
#define GetU32(x)		(*(unsigned long*)(x))
#define Set32(x,y)		(*(long*)(x)) = ((long)(y))

#define Get16(x)		(*(short*)(x))
#define GetU16(x)		(*(unsigned short*)(x))
#define Set16(x,y)		(*(short*)(x)) = ((short)(y))


#define kSpoolChunkSize (16384)
#define kInfiniteDataSize (0x7fffffff)


ComponentResult SetupColorPalette(VobSubCodecGlobals glob, ImageDescriptionHandle imageDescription) {
    OSErr err = noErr;
    
    Handle descExtension = NewHandle(0);
    
    err = GetImageDescriptionExtension(imageDescription, &descExtension, kIDXExtension, 1);
    
    char *string = (char *) *descExtension;
    
    char *palette = strstr(string, "palette:");
    
    if (palette != NULL) {
        sscanf(palette, "palette: %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx", 
               &glob->palette[0], &glob->palette[1], &glob->palette[2], &glob->palette[3], 
               &glob->palette[4], &glob->palette[5], &glob->palette[6], &glob->palette[7], 
               &glob->palette[8], &glob->palette[9], &glob->palette[10], &glob->palette[11], 
               &glob->palette[12], &glob->palette[13], &glob->palette[14], &glob->palette[15]);
    }

    return err;
}

void ExtractData(UInt8 *framedSrc, UInt8 *dest, int srcSize, int destSize) {
    int copiedBytes = 0;
    UInt8 *currentPacket = framedSrc;
    
    while (currentPacket - framedSrc < srcSize) {
        // 3-byte start code: 0x00 00 01
        if (currentPacket[0] + currentPacket[1] != 0 || currentPacket[2] != 1) {
            dprintf("VobSub Codec: !! Unknown header: %02x %02x %02x\n", 
                    currentPacket[0], currentPacket[1], currentPacket[2]);
            return;
        }
        
        int packet_length;
        
        switch (currentPacket[3]) {
            case 0xba:
                // discard PS packets; nothing in them we're interested in
                // here, packet_length is the additional stuffing
                packet_length = currentPacket[13] & 0x7;
                
                currentPacket += 14 + packet_length;
                break;
                
            case 0xbe:
            case 0xbf:
                // skip padding and navigation data 
                // (navigation shouldn't be present anyway)
                packet_length = currentPacket[4];
                packet_length <<= 8;
                packet_length += currentPacket[5];
                
                currentPacket += 6 + packet_length;
                break;
                
            case 0xbd:
                // a private stream packet, contains subtitle data
                packet_length = currentPacket[4];
                packet_length <<= 8;
                packet_length += currentPacket[5];
                
                int header_data_length = currentPacket[8];
                
                if (copiedBytes + packet_length - 4 - header_data_length > destSize) {
                    printf(" ExtractData() -  buffer passed not large enough for subtitle frame\n");
                    return;
                }
                    
                memcpy(&dest[copiedBytes], 
                       // header's 9 bytes + extension, we don't want 1st byte of packet
                       &currentPacket[9 + header_data_length + 1], 
                       // we don't want the 1-byte stream ID, or the header
                       packet_length - 1 - (header_data_length + 3));
                
                copiedBytes += packet_length - 1 - (header_data_length + 3);
                currentPacket += packet_length + 6;
                break;
                
            default:
                // unknown packet, probably video, return for now
                printf("VobSubCodec - Unknown packet type %x, aborting\n", (int)currentPacket[3]);
                return;
                break;
        } // switch (currentPacket[3])
    } // while (currentPacket - framedSrc < srcSize)
}

ComponentResult DecompressPacket(VobSubCodecGlobals glob, ImageSubCodecDecompressRecord *drp) {
    OSErr err = err;
    PacketControlData controlData;
    
    UInt8 *data = (UInt8 *) drp->codecData;
    // 2nd 2 bytes gives size of data. 1st 2 bytes of control header repeats this, so we skip it
    int controlSequenceOffset = (data[2] << 8) + data[3] + 4;
    
    err = ProcessControlSequence(&data[controlSequenceOffset], glob->palette, &controlData);
    
    if (err == noErr) {
        int i;
        UInt8 *firstScanData;
        UInt8 *secondScanData;
        firstScanData = &(data[controlData.firstScanPos]);
        secondScanData = &(data[controlData.secondScanPos]);
        
        Ptr currentLine = drp->baseAddr + controlData.startLine * drp->rowBytes;
        
        for (i = 0; i < controlData.endLine - controlData.startLine + 1; i += 2) {
            //memset(currentLine, 0, drp->rowBytes);
            DecodeLine(&firstScanData, (UInt32 *)currentLine, controlData);
            currentLine += drp->rowBytes;
            
            //dprintf("line ptr = %p\n", currentLine);
            //memset(currentLine, 0, drp->rowBytes);
            DecodeLine(&secondScanData, (UInt32 *)currentLine, controlData);
            currentLine += drp->rowBytes;
        }
    }
    return err;
}

ComponentResult ProcessControlSequence(UInt8 *controlSeq, UInt32 palette[16],
                                       PacketControlData *controlDataOut) {
    // to set whether the key sequences 0x03 - 0x06 have been seen
    UInt16 controlSeqSeen = 0;
    int i = 0;
    Boolean loop = TRUE;
    
    memset(controlDataOut, 0, sizeof(PacketControlData));
    
    while (loop) {
        switch (controlSeq[i]) {
            case 0x00:
                // subpicture identifier, we don't care
                i++;
                break;
                
            case 0x01:
                // start displaying, we don't care
                i++;
                break;
                
            case 0x03:
                // palette info
                controlDataOut->pixelColor[3] += palette[(controlSeq[i + 1] & 0xf0) >> 4];
                controlDataOut->pixelColor[2] += palette[(controlSeq[i + 1] & 0x0f)];
                controlDataOut->pixelColor[1] += palette[(controlSeq[i + 2] & 0xf0) >> 4];
                controlDataOut->pixelColor[0] += palette[(controlSeq[i + 2] & 0x0f)];
                
                printf("palette: %06lx %06lx %06lx %06lx\n", controlDataOut->pixelColor[0], 
                       controlDataOut->pixelColor[1], controlDataOut->pixelColor[2], 
                       controlDataOut->pixelColor[3]);
                
                i += 3;
                controlSeqSeen |= 0x000f;
                break;
                
            case 0x04:
                // alpha info
                controlDataOut->pixelColor[3] += (controlSeq[i + 1] & 0xf0) << 20;
                controlDataOut->pixelColor[2] += (controlSeq[i + 1] & 0x0f) << 24;
                controlDataOut->pixelColor[1] += (controlSeq[i + 2] & 0xf0) << 20;
                controlDataOut->pixelColor[0] += (controlSeq[i + 2] & 0x0f) << 24;
                
                // double the nibble
                controlDataOut->pixelColor[3] += (controlSeq[i + 1] & 0xf0) << 24;
                controlDataOut->pixelColor[2] += (controlSeq[i + 1] & 0x0f) << 28;
                controlDataOut->pixelColor[1] += (controlSeq[i + 2] & 0xf0) << 24;
                controlDataOut->pixelColor[0] += (controlSeq[i + 2] & 0x0f) << 28;
                
                i += 3;
                controlSeqSeen |= 0x00f0;
                break;
                
            case 0x05:
                // coordinates of image
                controlDataOut->startCol   = controlSeq[i + 1] << 4;
                controlDataOut->startCol  += controlSeq[i + 2] & 0xf0;
                controlDataOut->endCol    = (controlSeq[i + 2] & 0x0f) << 8;
                controlDataOut->endCol    += controlSeq[i + 3];
                controlDataOut->startLine  = controlSeq[i + 4] << 4;
                controlDataOut->startLine += controlSeq[i + 5] & 0xf0;
                controlDataOut->endLine   = (controlSeq[i + 5] & 0x0f) << 8;
                controlDataOut->endLine   += controlSeq[i + 6];
                
                i += 7;
                controlSeqSeen |= 0x0f00;
                break;
                
            case 0x06:
                // offset of the first graphic line, and second
                controlDataOut->firstScanPos   = controlSeq[i + 1] << 8;
                controlDataOut->firstScanPos  += controlSeq[i + 2];
                controlDataOut->secondScanPos  = controlSeq[i + 3] << 8;
                controlDataOut->secondScanPos += controlSeq[i + 4];
                
                i += 5;
                controlSeqSeen |= 0xf000;
                break;
                
            case 0xff:
                // end of control sequence
                loop = FALSE;
                break;
                
            default:
                dprintf(" !! Unknown control sequence 0x%02x  aborting (offset %x)\n", controlSeq[i], i);
                loop = FALSE;
                break;
        }
    }
    
    if (controlSeqSeen != 0xffff)
        return -1;
    else
        return noErr;
}

void DecodeLine(UInt8 **data, UInt32 *framePtr, PacketControlData controlData) {
    Boolean endline = FALSE;
    UInt8 nibbleMask = 0xf0;
    
    framePtr += controlData.startCol;
    
    while (!endline) {
        switch (**data & nibbleMask) {
            case 0x0:
                if (nibbleMask == 0xf0) {
                    // byte aligned
                    if ((**data & 0x0f) > 0x03) {
                        // 3-nibble word
                        PrintPix((**data << 4) + (((*data)[1] & 0xf0) >> 4), &framePtr, 
                                 controlData.pixelColor);
                        *data += 1;
                        nibbleMask = 0x0f;
                        
                    } else {
                        // 4-nibble word
                        PrintPix(GetU16(*data), &framePtr, controlData.pixelColor);
                        
                        // 0x00-- is a newline
                        if ((**data & 0x0f) == 0) {
                            endline = TRUE;
                        }
                        *data += 2;
                    }
                    
                } else {
                    // not byte aligned
                    if (((*data)[1] & 0xf0) > 0x30) {
                        // 3-nibble word with 1st nibble equal to zero
                        PrintPix((*data)[1], &framePtr, controlData.pixelColor);
                        *data += 2;
                        nibbleMask = 0xf0;
                        
                    } else {
                        // 4-nibble word, not byte-aligned, 1st nibble = 0
                        PrintPix(((*data)[1] << 4) + (((*data)[2] & 0xf0) >> 4), &framePtr, 
                                 controlData.pixelColor);
                        
                        // 0x00-- is a newline
                        if (((*data)[1] & 0xf0) == 0) {
                            endline = TRUE;
                            // byte align for the start of the next line
                            *data += 1;
                        }
                        *data += 2;
                    }
                }
                break;
                
            case 0x1:
            case 0x2:
            case 0x3:
                // 2-nibble word, not byte aligned
                PrintPix(((**data & 0x0f) << 4) + (((*data)[1] & 0xf0) >> 4), &framePtr, controlData.pixelColor);
                *data += 1;
                break;
                
            case 0x10:
            case 0x20:
            case 0x30:
                // 2-nibble word, byte aligned
                PrintPix(**data, &framePtr, controlData.pixelColor);
                *data += 1;
                break;
                
            default:
                // 1-nibble word
                if (nibbleMask == 0xf0) {
                    PrintPix((**data & 0xf0) >> 4, &framePtr, controlData.pixelColor);
                    nibbleMask = 0x0f;
                } else {
                    PrintPix(**data & 0x0f, &framePtr, controlData.pixelColor);
                    nibbleMask = 0xf0;
                    *data += 1;
                }
                break;
        }
    }
}

void PrintPix(UInt16 word, UInt32 **frame, UInt32 palette[4]) {
    int i;
    for (i = 0; i < word >> 2; i++) {
        (**frame) = palette[word & 0x3];
        *frame += 1;
    }
    if (word == 0) {
    }
}