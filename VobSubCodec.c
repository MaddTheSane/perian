/*
 * VobSubCodec.h
 * Created by David Conrad on 3/4/06.
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

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>
#include "PerianResourceIDs.h"
#include "Codecprintf.h"
#include "CommonUtils.h"
#include <zlib.h>
#include "avcodec.h"
#include "intreadwrite.h"

// Data structures
typedef struct	{
	ComponentInstance       self;
	ComponentInstance       delegateComponent;
	ComponentInstance       target;
	OSType**                wantedDestinationPixelTypeH;
	ImageCodecMPDrawBandUPP drawBandUPP;
	
	UInt32                  palette[16];
	
	int                     compressed;
	uint8_t                 *codecData;
	unsigned int            bufferSize;
	
	AVCodec                 *avCodec;
	AVCodecContext          *avContext;
	AVSubtitle              subtitle;
} VobSubCodecGlobalsRecord, *VobSubCodecGlobals;

typedef struct {
	long		width;
	long		height;
	long		bufferSize;
	char		decoded;
} VobSubDecompressRecord;

typedef struct {
	// color format is 32-bit ARGB
	UInt32  pixelColor[16];
	UInt32  duration;
} PacketControlData;

// Setup required for ComponentDispatchHelper.c
#define IMAGECODEC_BASENAME() 		VobSubCodec
#define IMAGECODEC_GLOBALS() 		VobSubCodecGlobals storage

#define CALLCOMPONENT_BASENAME()	IMAGECODEC_BASENAME()
#define	CALLCOMPONENT_GLOBALS()		IMAGECODEC_GLOBALS()

#define COMPONENT_UPP_PREFIX()		uppImageCodec
#define COMPONENT_DISPATCH_FILE		"VobSubCodecDispatch.h"
#define COMPONENT_SELECT_PREFIX()  	kImageCodec

#define	GET_DELEGATE_COMPONENT()	(storage->delegateComponent)

#include <CoreServices/Components.k.h>
#include <QuickTime/ImageCodec.k.h>
#include <QuickTime/ComponentDispatchHelper.c>

#define kNumPixelFormatsSupportedVobSub 1

// dest must be at least as large as src
int ExtractVobSubPacket(UInt8 *dest, UInt8 *framedSrc, int srcSize, int *usedSrcBytes, int index);
static ComponentResult ReadPacketControls(UInt8 *packet, UInt32 palette[16], PacketControlData *controlDataOut);
extern void initLib();
extern void init_FFmpeg();

ComponentResult VobSubCodecOpen(VobSubCodecGlobals glob, ComponentInstance self)
{
	ComponentResult err;
	// Allocate memory for our globals, set them up and inform the component manager that we've done so
	glob = (VobSubCodecGlobals)NewPtrClear(sizeof(VobSubCodecGlobalsRecord));
	if (err = MemError()) goto bail;

	SetComponentInstanceStorage(self, (Handle)glob);

	glob->self = self;
	glob->target = self;
	glob->wantedDestinationPixelTypeH = (OSType **)NewHandleClear((kNumPixelFormatsSupportedVobSub+1) * sizeof(OSType));
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

ComponentResult VobSubCodecClose(VobSubCodecGlobals glob, ComponentInstance self)
{
	int i;
	
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
		if (glob->codecData) {
			av_freep(&glob->codecData);
		}
		if (glob->avCodec) {
			avcodec_close(glob->avContext);
		}
		if (glob->avContext) {
			av_freep(&glob->avContext);
		}
		if (glob->subtitle.rects) {
			for (i = 0; i < glob->subtitle.num_rects; i++) {
				av_freep(&glob->subtitle.rects[i]->pict.data[0]);
				av_freep(&glob->subtitle.rects[i]->pict.data[1]);
				av_freep(&glob->subtitle.rects[i]);
			}
			av_freep(&glob->subtitle.rects);
		}

		DisposePtr((Ptr)glob);
	}

	return noErr;
}

ComponentResult VobSubCodecVersion(VobSubCodecGlobals glob)
{	
	return kVobSubCodecVersion;
}

ComponentResult VobSubCodecTarget(VobSubCodecGlobals glob, ComponentInstance target)
{
	glob->target = target;
	return noErr;
}

ComponentResult VobSubCodecGetMPWorkFunction(VobSubCodecGlobals glob, ComponentMPWorkFunctionUPP *workFunction, void **refCon)
{
	if (glob->drawBandUPP == NULL)
		glob->drawBandUPP = NewImageCodecMPDrawBandUPP((ImageCodecMPDrawBandProcPtr)VobSubCodecDrawBand);
	
	return ImageCodecGetBaseMPWorkFunction(glob->delegateComponent, workFunction, refCon, glob->drawBandUPP, glob);
}

ComponentResult VobSubCodecInitialize(VobSubCodecGlobals glob, ImageSubCodecDecompressCapabilities *cap)
{
	cap->decompressRecordSize = sizeof(VobSubDecompressRecord);
	cap->canAsync = true;
	cap->baseCodecShouldCallDecodeBandForAllFrames = true;
	
	if(cap->recordSize > offsetof(ImageSubCodecDecompressCapabilities, baseCodecShouldCallDecodeBandForAllFrames)) 
		cap->subCodecIsMultiBufferAware = true;

	return noErr;
}

static ComponentResult SetupColorPalette(VobSubCodecGlobals glob, ImageDescriptionHandle imageDescription) {
	OSErr err = noErr;
	Handle descExtension = NewHandle(0);
	
	err = GetImageDescriptionExtension(imageDescription, &descExtension, kVobSubIdxExtension, 1);
	if (err) goto bail;
	
	char *string = (char *) *descExtension;
	
	char *palette = strnstr(string, "palette:", GetHandleSize(descExtension));
	
	if (palette != NULL) {
		sscanf(palette, "palette: %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx", 
			   &glob->palette[ 0], &glob->palette[ 1], &glob->palette[ 2], &glob->palette[ 3], 
			   &glob->palette[ 4], &glob->palette[ 5], &glob->palette[ 6], &glob->palette[ 7], 
			   &glob->palette[ 8], &glob->palette[ 9], &glob->palette[10], &glob->palette[11], 
			   &glob->palette[12], &glob->palette[13], &glob->palette[14], &glob->palette[15]);
	}
	
bail:
	DisposeHandle(descExtension);
	return err;
}

ComponentResult VobSubCodecPreflight(VobSubCodecGlobals glob, CodecDecompressParams *p)
{
	CodecCapabilities *capabilities = p->capabilities;
	OSTypePtr         formats = *glob->wantedDestinationPixelTypeH;

	// Specify the minimum image band height supported by the component
	// bandInc specifies a common factor of supported image band heights
	capabilities->bandMin = (**p->imageDescription).height;
	capabilities->bandInc = capabilities->bandMin;

	// Indicate the wanted destination using the wantedDestinationPixelTypeH previously set up
	capabilities->wantedPixelSize  = 0; 	
	
	// we want 4:4:4:4 ARGB
	*formats++ = k32ARGBPixelFormat;
	
	p->wantedDestinationPixelTypes = glob->wantedDestinationPixelTypeH;

	// Specify the number of pixels the image must be extended in width and height if
	// the component cannot accommodate the image at its given width and height
	capabilities->extendWidth = 0;
	capabilities->extendHeight = 0;
	
	// get the color palette info from the image description
	SetupColorPalette(glob, p->imageDescription);

	if (isImageDescriptionExtensionPresent(p->imageDescription, kMKVCompressionExtension))
		glob->compressed = 1;
	
	if (!glob->avCodec) {
		init_FFmpeg();
		
		glob->avCodec = avcodec_find_decoder(CODEC_ID_DVD_SUBTITLE);
		glob->avContext = avcodec_alloc_context();
		
		if (avcodec_open(glob->avContext, glob->avCodec)) {
			Codecprintf(NULL, "Error opening DVD subtitle decoder\n");
			return codecErr;
		}
	}
	
	return noErr;
}

ComponentResult VobSubCodecBeginBand(VobSubCodecGlobals glob, CodecDecompressParams *p, ImageSubCodecDecompressRecord *drp, long flags)
{
	VobSubDecompressRecord *myDrp = (VobSubDecompressRecord *)drp->userDecompressRecord;

	// Let base codec know that all our frames are keyframes
	drp->frameType = kCodecFrameTypeKey;

	myDrp->width = (**p->imageDescription).width;
	myDrp->height = (**p->imageDescription).height;
	myDrp->bufferSize = p->bufferSize;
	myDrp->decoded = p->frameTime ? (0 != (p->frameTime->flags & icmFrameAlreadyDecoded)) : false;
	
	return noErr;
}

void DecompressZlib(VobSubCodecGlobals glob, uint8_t *data, long *bufferSize)
{
	ComponentResult err = noErr;
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	err = inflateInit(&strm);
	if (err != Z_OK) return;
	
	strm.avail_in = *bufferSize;
	strm.next_in = data;
	
	// first, get the size of the decompressed data
	strm.avail_out = 2;
	strm.next_out = glob->codecData;
	
	err = inflate(&strm, Z_SYNC_FLUSH);
	if (err < Z_OK) goto bail;
	if (strm.avail_out != 0) goto bail;
	
	// reallocate our buffer to be big enough to store the decompressed packet
	*bufferSize = AV_RB16(glob->codecData);
	glob->codecData = fast_realloc_with_padding(glob->codecData, &glob->bufferSize, *bufferSize);
	
	// then decompress the rest of it
	strm.avail_out = glob->bufferSize - 2;
	strm.next_out = glob->codecData + 2;
	
	inflate(&strm, Z_SYNC_FLUSH);
bail:
	inflateEnd(&strm);
}

ComponentResult VobSubCodecDecodeBand(VobSubCodecGlobals glob, ImageSubCodecDecompressRecord *drp, unsigned long flags)
{
	VobSubDecompressRecord *myDrp = (VobSubDecompressRecord *)drp->userDecompressRecord;
	UInt8 *data = (UInt8 *) drp->codecData;
	int ret, got_sub;
	
	if(myDrp->bufferSize < 4)
	{
		myDrp->decoded = true;
		return noErr;
	}
	
	if (glob->codecData == NULL) {
		glob->codecData = av_malloc(myDrp->bufferSize + 2);
		glob->bufferSize = myDrp->bufferSize + 2;
	}
	
	// make sure we have enough space to store the packet
	glob->codecData = fast_realloc_with_padding(glob->codecData, &glob->bufferSize, myDrp->bufferSize + 2);
	
	if (glob->compressed) {
		DecompressZlib(glob, data, &myDrp->bufferSize);
		
	// the header of a spu PS packet starts 0x000001bd
	// if it's raw spu data, the 1st 2 bytes are the length of the data
	} else if (data[0] + data[1] == 0) {
		// remove the MPEG framing
		myDrp->bufferSize = ExtractVobSubPacket(glob->codecData, data, myDrp->bufferSize, NULL, -1);
	} else {
		memcpy(glob->codecData, drp->codecData, myDrp->bufferSize);
	}
	
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = glob->codecData;
	pkt.size = myDrp->bufferSize;
	ret = avcodec_decode_subtitle2(glob->avContext, &glob->subtitle, &got_sub, &pkt);
	
	if (ret < 0 || !got_sub) {
		Codecprintf(NULL, "Error decoding DVD subtitle %d / %ld\n", ret, myDrp->bufferSize);
		return codecErr;
	}
	
	myDrp->decoded = true;
	
	return noErr;
}

ComponentResult VobSubCodecDrawBand(VobSubCodecGlobals glob, ImageSubCodecDecompressRecord *drp)
{
	OSErr err = noErr;
	VobSubDecompressRecord *myDrp = (VobSubDecompressRecord *)drp->userDecompressRecord;
	PacketControlData controlData;
	uint8_t *data = glob->codecData;
	unsigned int i, j, x, y;
	int usePalette = 0;
	
	if(!myDrp->decoded)
		err = VobSubCodecDecodeBand(glob, drp, 0);
	if (err) return err;
	
	// clear the buffer to pure transparent
	memset(drp->baseAddr, 0, myDrp->height * drp->rowBytes);
	if(myDrp->bufferSize < 4)
		return noErr;
	
	err = ReadPacketControls(data, glob->palette, &controlData);
	if (err == noErr)
		usePalette = true;
	
	for (i = 0; i < glob->subtitle.num_rects; i++) {
		AVSubtitleRect *rect = glob->subtitle.rects[i];
		uint8_t *line = (uint8_t *)drp->baseAddr + drp->rowBytes * rect->y + rect->x*4;
		uint8_t *sub = rect->pict.data[0];
		unsigned int w = FFMIN(rect->w, myDrp->width  - rect->x);
		unsigned int h = FFMIN(rect->h, myDrp->height - rect->y);
		uint32_t *palette = (uint32_t *)rect->pict.data[1];
		
		if (usePalette) {
			for (j = 0; j < 4; j++)
				palette[j] = EndianU32_BtoN(controlData.pixelColor[j]);
		}
		
		for (y = 0; y < h; y++) {
			uint32_t *pixel = (uint32_t *) line;
			
			for (x = 0; x < w; x++)
				pixel[x] = palette[sub[x]];
			
			line += drp->rowBytes;
			sub += rect->pict.linesize[0];
		}
	}
	
	return err;
}

ComponentResult VobSubCodecEndBand(VobSubCodecGlobals glob, ImageSubCodecDecompressRecord *drp, OSErr result, long flags)
{
	return noErr;
}

// Gamma curve value for VobSub.
// Not specified, so assume it's the same as the video on the DVD.
// modern PAL uses NTSC-ish gamma, so don't even bother guessing it.
ComponentResult VobSubCodecGetSourceDataGammaLevel(VobSubCodecGlobals glob, Fixed *sourceDataGammaLevel)
{
	*sourceDataGammaLevel = FloatToFixed(1/.45); // == ~2.2
	return noErr;
}

ComponentResult VobSubCodecGetCodecInfo(VobSubCodecGlobals glob, CodecInfo *info)
{
	OSErr err = noErr;
	if (info == NULL) {
		err = paramErr;
	} else {
		CodecInfo **tempCodecInfo;

		err = GetComponentResource((Component)glob->self, codecInfoResourceType, kVobSubCodecResourceID, (Handle *)&tempCodecInfo);
		if (err == noErr) {
			*info = **tempCodecInfo;
			DisposeHandle((Handle)tempCodecInfo);
		}
	}

	return err;
}

int ExtractVobSubPacket(UInt8 *dest, UInt8 *framedSrc, int srcSize, int *usedSrcBytes, int index) {
	int copiedBytes = 0;
	UInt8 *currentPacket = framedSrc;
	int packetSize = INT_MAX;
	
	while (currentPacket - framedSrc < srcSize && copiedBytes < packetSize) {
		// 3-byte start code: 0x00 00 01
		if (currentPacket[0] + currentPacket[1] != 0 || currentPacket[2] != 1) {
			Codecprintf(NULL, "VobSub Codec: !! Unknown header: %02x %02x %02x\n", currentPacket[0], currentPacket[1], currentPacket[2]);
			return copiedBytes;
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
				int packetIndex = currentPacket[header_data_length + 9] & 0x1f;
				if(index == -1)
					index = packetIndex;
				if(index == packetIndex)
				{
					int blockSize = packet_length - 1 - (header_data_length + 3);
					memcpy(&dest[copiedBytes], 
						   // header's 9 bytes + extension, we don't want 1st byte of packet
						   &currentPacket[9 + header_data_length + 1], 
						   // we don't want the 1-byte stream ID, or the header
						   blockSize);
					copiedBytes += blockSize;

					if(packetSize == INT_MAX)
					{
						packetSize = dest[0] << 8 | dest[1];
					}
				}
				currentPacket += packet_length + 6;
				break;
				
			default:
				// unknown packet, probably video, return for now
				Codecprintf(NULL, "VobSubCodec - Unknown packet type %x, aborting\n", (int)currentPacket[3]);
				return copiedBytes;
		} // switch (currentPacket[3])
	} // while (currentPacket - framedSrc < srcSize)
	if(usedSrcBytes != NULL)
		*usedSrcBytes = currentPacket - framedSrc;
	
	return copiedBytes;
}

static ComponentResult ReadPacketControls(UInt8 *packet, UInt32 palette[16], PacketControlData *controlDataOut) {
	// to set whether the key sequences 0x03 - 0x06 have been seen
	UInt16 controlSeqSeen = 0;
	int i = 0;
	Boolean loop = TRUE;
	int controlOffset = (packet[2] << 8) + packet[3] + 4;
	uint8_t *controlSeq = packet + controlOffset;
	
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
				controlDataOut->pixelColor[3] += palette[controlSeq[i+1] >> 4 ];
				controlDataOut->pixelColor[2] += palette[controlSeq[i+1] & 0xf];
				controlDataOut->pixelColor[1] += palette[controlSeq[i+2] >> 4 ];
				controlDataOut->pixelColor[0] += palette[controlSeq[i+2] & 0xf];
				
				i += 3;
				controlSeqSeen |= 0x0f;
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
				controlSeqSeen |= 0xf0;
				break;
				
			case 0x05:
				// coordinates of image, ffmpeg takes care of this
				i += 7;
				break;
				
			case 0x06:
				// offset of the first graphic line, and second, ffmpeg takes care of this
				i += 5;
				break;
				
			case 0xff:
				// end of control sequence
				loop = FALSE;
				break;
				
			default:
				Codecprintf(NULL, " !! Unknown control sequence 0x%02x  aborting (offset %x)\n", controlSeq[i], i);
				loop = FALSE;
				break;
		}
	}
	
	// force fully transparent to transparent black; needed? for graphicsModePreBlackAlpha
	for (i = 0; i < 4; i++) {
		if ((controlDataOut->pixelColor[i] & 0xff000000) == 0)
			controlDataOut->pixelColor[i] = 0;
	}
	
	if (controlSeqSeen != 0xff)
		return -1;
	return noErr;
}
