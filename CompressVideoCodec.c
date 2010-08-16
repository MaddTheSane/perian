/*
 * CompressVideoCodec.c
 * Created by Graham Booker on 8/14/10.
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
#include "avcodec.h"
#include "CompressCodecUtils.h"

typedef struct {
	ComponentInstance	self;
	ComponentInstance	delegateComponent;
	ComponentInstance	actualCodec;
	int					strippedHeaderSize;
	uint8_t				*strippedHeader;
	int					userDecompressRecordSize;
	OSType				originalFourCC;
} CompressCodecGlobalRecord, *CompressCodecGlobals;

typedef struct {
	void				*userDecompressRecord;
	Ptr					codecData;
} CompressDecompressRecord;

// Setup required for ComponentDispatchHelper.c
#define IMAGECODEC_BASENAME() 		CompressCodec
#define IMAGECODEC_GLOBALS() 		CompressCodecGlobals storage

#define CALLCOMPONENT_BASENAME()	IMAGECODEC_BASENAME()
#define	CALLCOMPONENT_GLOBALS()		IMAGECODEC_GLOBALS()

#define COMPONENT_UPP_PREFIX()		uppImageCodec
#define COMPONENT_DISPATCH_FILE		"CompressVideoCodecDispatch.h"
#define COMPONENT_SELECT_PREFIX()  	kImageCodec

#define	GET_DELEGATE_COMPONENT()	(storage->delegateComponent)

#include <CoreServices/Components.k.h>
#include <QuickTime/ImageCodec.k.h>
#include <QuickTime/ComponentDispatchHelper.c>

ComponentResult CompressCodecOpen(CompressCodecGlobals glob, ComponentInstance self)
{
	ComponentResult err;
	
	glob = (CompressCodecGlobals)NewPtrClear(sizeof(CompressCodecGlobalRecord));
	if(err = MemError()) goto bail;
	
	SetComponentInstanceStorage(self, (Handle)glob);
	glob->self = self;
	
	err = OpenADefaultComponent(decompressorComponentType, kBaseCodecType, &glob->delegateComponent);
	if (err) goto bail;
	ComponentSetTarget(glob->delegateComponent, self);

    ComponentDescription desc;
    GetComponentInfo((Component)self, &desc, 0, 0, 0);
	OSType original = originalStreamFourCC(desc.componentSubType);
	if(original != 0)
	{
		glob->originalFourCC = original;
		memset(&desc, 0, sizeof(ComponentDescription));
		desc.componentType = decompressorComponentType;
		desc.componentSubType = original;
		Component component = FindNextComponent(0, &desc);
		if(component)
			glob->actualCodec = OpenComponent(component);
	}
		
bail:
	return err;
}

ComponentResult CompressCodecClose(CompressCodecGlobals glob, ComponentInstance self)
{
	if(glob) {
		if(glob->actualCodec)
			CloseComponent(glob->actualCodec);
		if(glob->strippedHeader)
			av_freep(&glob->strippedHeader);
		DisposePtr((Ptr)glob);
	}
	
	return noErr;
}

ComponentResult CompressCodecVersion(CompressCodecGlobals glob)
{
	if(glob->actualCodec)
		return CallComponentVersion(glob->actualCodec);
	return kCompressCodecVersion;
}

ComponentResult CompressCodecTarget(CompressCodecGlobals glob, ComponentInstance target)
{
	if(glob->actualCodec)
		return CallComponentTarget(glob->actualCodec, target);
	return noErr;
}

ComponentResult CompressCodecGetMPWorkFunction(CompressCodecGlobals glob, ComponentMPWorkFunctionUPP *workFunction, void **refCon)
{
	if(glob->actualCodec)
		return CallComponentGetMPWorkFunction(glob->actualCodec, workFunction, refCon);
	return noErr;
}

ComponentResult CompressCodecInitialize(CompressCodecGlobals glob, ImageSubCodecDecompressCapabilities *cap)
{
	ComponentResult err = noErr;
	cap->decompressRecordSize = sizeof(CompressDecompressRecord);
	if(glob->actualCodec)
	{
		err = ImageCodecInitialize(glob->actualCodec, cap);
		if(err == noErr)
			glob->userDecompressRecordSize = cap->decompressRecordSize;
	}
	return err;
}

ComponentResult CompressCodecPreflight(CompressCodecGlobals glob, CodecDecompressParams *p)
{
	ImageDescriptionHandle imageDesc = p->imageDescription;
	
	if(isImageDescriptionExtensionPresent(imageDesc, kCompressionSettingsExtension))
	{
		Handle header = NewHandle(0);
		GetImageDescriptionExtension(imageDesc, &header, kCompressionSettingsExtension, 1);
		int size = GetHandleSize(header);
		if(size > 0)
		{
			glob->strippedHeaderSize = size;
			glob->strippedHeader = malloc(size);
			memcpy(glob->strippedHeader, *header, size);
		}
		DisposeHandle(header);
	}
	
	OSType myType = (*imageDesc)->cType;
	(*imageDesc)->cType = glob->originalFourCC;
	Ptr oldData = p->data;
	long oldBufferSize = p->bufferSize;
	if(glob->strippedHeaderSize)
	{
		long newSize = oldBufferSize + glob->strippedHeaderSize;
		Ptr newData = NewPtrClear(newSize);
		memcpy(newData, glob->strippedHeader, glob->strippedHeaderSize);
		memcpy(newData + glob->strippedHeaderSize, oldData, oldBufferSize);
		p->data = newData;
		p->bufferSize += glob->strippedHeaderSize;
	}
	ComponentResult err = ImageCodecPreflight(glob->actualCodec, p);
	(*imageDesc)->cType = myType;
	if(glob->strippedHeaderSize)
	{
		DisposePtr(p->data);
		p->data = oldData;
		p->bufferSize = oldBufferSize;
	}
	
	return noErr;
}

ComponentResult CompressCodecBeginBand(CompressCodecGlobals glob, CodecDecompressParams *p, ImageSubCodecDecompressRecord *drp, long flags)
{
	CompressDecompressRecord *myDrp = (CompressDecompressRecord *)drp->userDecompressRecord;
	myDrp->userDecompressRecord = malloc(glob->userDecompressRecordSize);
	
	long oldBufferSize = p->bufferSize;
	long newBufferSize = oldBufferSize + glob->strippedHeaderSize;
	Ptr oldCodecData = drp->codecData;
	Ptr codecData = NewPtrClear(newBufferSize);
	
	memcpy(codecData, glob->strippedHeader, glob->strippedHeaderSize);
	memcpy(codecData + glob->strippedHeaderSize, oldCodecData, p->bufferSize);
	myDrp->codecData = codecData;
	
	drp->userDecompressRecord = myDrp->userDecompressRecord;
	drp->codecData = codecData;
	p->bufferSize = newBufferSize;
	
	ComponentResult err = ImageCodecBeginBand(glob->actualCodec, p, drp, flags);
	
	drp->userDecompressRecord = myDrp;
	drp->codecData = oldCodecData;
	p->bufferSize = oldBufferSize;
	
	return err;
}

ComponentResult CompressCodecDecodeBand(CompressCodecGlobals glob, ImageSubCodecDecompressRecord *drp, unsigned long flags)
{
	CompressDecompressRecord *myDrp = (CompressDecompressRecord *)drp->userDecompressRecord;
	Ptr oldCodecData = drp->codecData;
	
	drp->userDecompressRecord = myDrp->userDecompressRecord;
	drp->codecData = myDrp->codecData;
	
	ComponentResult err = ImageCodecDecodeBand(glob->actualCodec, drp, flags);
	
	drp->userDecompressRecord = myDrp;
	drp->codecData = oldCodecData;
	
	return err;
}

ComponentResult CompressCodecDrawBand(CompressCodecGlobals glob, ImageSubCodecDecompressRecord *drp)
{
	CompressDecompressRecord *myDrp = (CompressDecompressRecord *)drp->userDecompressRecord;
	Ptr oldCodecData = drp->codecData;
	
	drp->userDecompressRecord = myDrp->userDecompressRecord;
	drp->codecData = myDrp->codecData;
	
	ComponentResult err = ImageCodecDrawBand(glob->actualCodec, drp);
	
	drp->userDecompressRecord = myDrp;
	drp->codecData = oldCodecData;
	
	return err;
}

ComponentResult CompressCodecEndBand(CompressCodecGlobals glob, ImageSubCodecDecompressRecord *drp, OSErr result, long flags)
{
	CompressDecompressRecord *myDrp = (CompressDecompressRecord *)drp->userDecompressRecord;
	Ptr oldCodecData = drp->codecData;
	
	drp->userDecompressRecord = myDrp->userDecompressRecord;
	drp->codecData = myDrp->codecData;
	
	ComponentResult err = ImageCodecEndBand(glob->actualCodec, drp, result, flags);
	av_freep(&myDrp->userDecompressRecord);
	av_freep(&myDrp->codecData);
	
	drp->userDecompressRecord = myDrp;
	drp->codecData = oldCodecData;
	
	return err;
}

ComponentResult CompressCodecGetCodecInfo(CompressCodecGlobals glob, CodecInfo *info)
{
	if(glob->actualCodec)
		return ImageCodecGetCodecInfo(glob->actualCodec, info);
	return noErr;
}

ComponentResult CompressCodecGetSourceDataGammaLevel(CompressCodecGlobals glob, Fixed *sourceDataGammaLevel)
{
	if(glob->actualCodec)
		return ImageCodecGetSourceDataGammaLevel(glob->actualCodec, sourceDataGammaLevel);
	return noErr;
}