/*	Copyright: 	© Copyright 2005 Apple Computer, Inc. All rights reserved.

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
			("Apple") in consideration of your agreement to the following terms, and your
			use, installation, modification or redistribution of this Apple software
			constitutes acceptance of these terms.  If you do not agree with these terms,
			please do not use, install, modify or redistribute this Apple software.

			In consideration of your agreement to abide by the following terms, and subject
			to these terms, Apple grants you a personal, non-exclusive license, under Apple’s
			copyrights in this original Apple software (the "Apple Software"), to use,
			reproduce, modify and redistribute the Apple Software, with or without
			modifications, in source and/or binary forms; provided that if you redistribute
			the Apple Software in its entirety and without modifications, you must retain
			this notice and the following text and disclaimers in all such redistributions of
			the Apple Software.  Neither the name, trademarks, service marks or logos of
			Apple Computer, Inc. may be used to endorse or promote products derived from the
			Apple Software without specific prior written permission from Apple.  Except as
			expressly stated in this notice, no other rights or licenses, express or implied,
			are granted by Apple herein, including but not limited to any patent rights that
			may be infringed by your derivative works or by other works in which the Apple
			Software may be incorporated.

			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
			WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
			WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
			PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
			COMBINATION WITH YOUR PRODUCTS.

			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
			CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
			GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
			ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
			OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
			(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
			ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*=============================================================================
	ACBaseCodec.cpp

=============================================================================*/

//=============================================================================
//	Includes
//=============================================================================

#include "ACBaseCodec.h"
#include <algorithm>
#include "CABundleLocker.h"

#if TARGET_OS_WIN32
	#include "CAWin32StringResources.h"
#endif

//=============================================================================
//	ACBaseCodec
//=============================================================================

ACBaseCodec::ACBaseCodec()
:
	ACCodec(),
	mIsInitialized(false),
	mInputFormatList(),
	mInputFormat(),
	mOutputFormatList(),
	mOutputFormat()
{
}

ACBaseCodec::~ACBaseCodec()
{
}

void	ACBaseCodec::GetPropertyInfo(AudioCodecPropertyID inPropertyID, UInt32& outPropertyDataSize, bool& outWritable)
{
	switch(inPropertyID)
	{
		case kAudioCodecPropertyNameCFString:
			outPropertyDataSize = sizeof(CFStringRef);
			outWritable = false;
			break;
		case kAudioCodecPropertyManufacturerCFString:
			outPropertyDataSize = sizeof(CFStringRef);
			outWritable = false;
			break;
		case kAudioCodecPropertyFormatCFString:
			outPropertyDataSize = sizeof(CFStringRef);
			outWritable = false;
			break;
		case kAudioCodecPropertyRequiresPacketDescription:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;
		case kAudioCodecPropertyMinimumNumberInputPackets :
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;
		case kAudioCodecPropertyMinimumNumberOutputPackets :
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;
		case kAudioCodecPropertyInputChannelLayout :
		case kAudioCodecPropertyOutputChannelLayout :
			// by default a codec doesn't support channel layouts.
			CODEC_THROW(kAudioCodecIllegalOperationError);
			break;
		case kAudioCodecPropertyAvailableInputChannelLayouts :
		case kAudioCodecPropertyAvailableOutputChannelLayouts :
			// by default a codec doesn't support channel layouts.
			CODEC_THROW(kAudioCodecIllegalOperationError);
			break;

		case kAudioCodecPropertyCurrentInputFormat:
			outPropertyDataSize = sizeof(AudioStreamBasicDescription);
			outWritable = true;
			break;
			
		case kAudioCodecPropertySupportedInputFormats:
		case kAudioCodecInputFormatsForOutputFormat:
			outPropertyDataSize = GetNumberSupportedInputFormats() * sizeof(AudioStreamBasicDescription);
			outWritable = false;
			break;
			
		case kAudioCodecPropertyCurrentOutputFormat:
			outPropertyDataSize = sizeof(AudioStreamBasicDescription);
			outWritable = true;
			break;
			
		case kAudioCodecPropertySupportedOutputFormats:
		case kAudioCodecOutputFormatsForInputFormat:
			outPropertyDataSize = GetNumberSupportedOutputFormats() * sizeof(AudioStreamBasicDescription);
			outWritable = false;
			break;
			
		case kAudioCodecPropertyMagicCookie:
			outPropertyDataSize = GetMagicCookieByteSize();
			outWritable = true;
			break;
			
		case kAudioCodecPropertyInputBufferSize:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;
			
		case kAudioCodecPropertyUsedInputBufferSize:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;
		
		case kAudioCodecPropertyIsInitialized:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;

		case kAudioCodecPropertyAvailableNumberChannels:
			outPropertyDataSize = sizeof(UInt32) * 2; // Mono, stereo
			outWritable = false;
			break;
			
 		case kAudioCodecPropertyPrimeMethod:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;

 		case kAudioCodecPropertyPrimeInfo:
			outPropertyDataSize = sizeof(AudioCodecPrimeInfo);
			outWritable = false;
			break;

 		case kAudioCodecUseRecommendedSampleRate:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;
			
 		case kAudioCodecOutputPrecedence:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;

 		case kAudioCodecDoesSampleRateConversion:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;
#ifdef kAudioCodecRequiresMagicCookie
 		case kAudioCodecRequiresMagicCookie:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;
#endif

		default:
			CODEC_THROW(kAudioCodecUnknownPropertyError);
			break;
			
	};
}

void	ACBaseCodec::GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData)
{
	UInt32 thePacketsToGet;
	
	switch(inPropertyID)
	{
		case kAudioCodecPropertyNameCFString:
		{
			if (ioPropertyDataSize != sizeof(CFStringRef)) CODEC_THROW(kAudioCodecBadPropertySizeError);
			
			CABundleLocker lock;
			CFStringRef name = CFCopyLocalizedStringFromTableInBundle(CFSTR("unknown codec"), CFSTR("CodecNames"), GetCodecBundle(), CFSTR(""));
			*(CFStringRef*)outPropertyData = name;
			break; 
		}
		case kAudioCodecPropertyManufacturerCFString:
		{
			if (ioPropertyDataSize != sizeof(CFStringRef)) CODEC_THROW(kAudioCodecBadPropertySizeError);
			
			CABundleLocker lock;
			CFStringRef name = CFCopyLocalizedStringFromTableInBundle(CFSTR("Apple Computer, Inc."), CFSTR("CodecNames"), GetCodecBundle(), CFSTR(""));
			*(CFStringRef*)outPropertyData = name;
			break; 
		}
        case kAudioCodecPropertyRequiresPacketDescription:
  			if(ioPropertyDataSize == sizeof(UInt32))
			{
                *reinterpret_cast<UInt32*>(outPropertyData) = 0; 
            }
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
            break;
		case kAudioCodecPropertyMinimumNumberInputPackets :
			if(ioPropertyDataSize != sizeof(UInt32)) CODEC_THROW(kAudioCodecBadPropertySizeError);
			*(UInt32*)outPropertyData = 1;
			break;
		case kAudioCodecPropertyMinimumNumberOutputPackets :
			if(ioPropertyDataSize != sizeof(UInt32)) CODEC_THROW(kAudioCodecBadPropertySizeError);
			*(UInt32*)outPropertyData = 1;
			break;
			
		case kAudioCodecPropertyInputChannelLayout :
		case kAudioCodecPropertyOutputChannelLayout :
			// by default a codec doesn't support channel layouts.
			CODEC_THROW(kAudioCodecIllegalOperationError);
			break;
		case kAudioCodecPropertyAvailableInputChannelLayouts :
		case kAudioCodecPropertyAvailableOutputChannelLayouts :
			// by default a codec doesn't support channel layouts.
			CODEC_THROW(kAudioCodecIllegalOperationError);
			break;
			
		case kAudioCodecPropertyCurrentInputFormat:
			if(ioPropertyDataSize == sizeof(AudioStreamBasicDescription))
			{
				GetCurrentInputFormat(*reinterpret_cast<AudioStreamBasicDescription*>(outPropertyData));
			}
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;
			
		case kAudioCodecPropertySupportedInputFormats:
		case kAudioCodecInputFormatsForOutputFormat:
			thePacketsToGet = ioPropertyDataSize / sizeof(AudioStreamBasicDescription);
			GetSupportedInputFormats(reinterpret_cast<AudioStreamBasicDescription*>(outPropertyData), thePacketsToGet);
			ioPropertyDataSize = thePacketsToGet * sizeof(AudioStreamBasicDescription);
			break;
			
		case kAudioCodecPropertyCurrentOutputFormat:
			if(ioPropertyDataSize == sizeof(AudioStreamBasicDescription))
			{
				GetCurrentOutputFormat(*reinterpret_cast<AudioStreamBasicDescription*>(outPropertyData));
			}
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;
			
		case kAudioCodecPropertySupportedOutputFormats:
		case kAudioCodecOutputFormatsForInputFormat:
			thePacketsToGet = ioPropertyDataSize / sizeof(AudioStreamBasicDescription);
			GetSupportedOutputFormats(reinterpret_cast<AudioStreamBasicDescription*>(outPropertyData), thePacketsToGet);
			ioPropertyDataSize = thePacketsToGet * sizeof(AudioStreamBasicDescription);
			break;
			
		case kAudioCodecPropertyMagicCookie:
			if(ioPropertyDataSize >= GetMagicCookieByteSize())
			{
				GetMagicCookie(outPropertyData, ioPropertyDataSize);
			}
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;
			
		case kAudioCodecPropertyInputBufferSize:
			if(ioPropertyDataSize == sizeof(UInt32))
			{
				*reinterpret_cast<UInt32*>(outPropertyData) = GetInputBufferByteSize();
			}
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;
			
		case kAudioCodecPropertyUsedInputBufferSize:
			if(ioPropertyDataSize == sizeof(UInt32))
			{
				*reinterpret_cast<UInt32*>(outPropertyData) = GetUsedInputBufferByteSize();
			}
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;
			
		case kAudioCodecPropertyIsInitialized:
			if(ioPropertyDataSize == sizeof(UInt32))
			{
				*reinterpret_cast<UInt32*>(outPropertyData) = IsInitialized() ? 1 : 0;
			}
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;
			
        case kAudioCodecPropertyAvailableNumberChannels:
  			if(ioPropertyDataSize == sizeof(UInt32) * 2)
			{
				(reinterpret_cast<UInt32*>(outPropertyData))[0] = 1;
				(reinterpret_cast<UInt32*>(outPropertyData))[1] = 2;
			}
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;

        case kAudioCodecPropertyPrimeMethod:
  			if(ioPropertyDataSize == sizeof(UInt32))
			{
				*reinterpret_cast<UInt32*>(outPropertyData) = (UInt32)kAudioCodecPrimeMethod_None;
			}
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;

		case kAudioCodecPropertyPrimeInfo:
  			if(ioPropertyDataSize == sizeof(AudioCodecPrimeInfo) )
			{
				(reinterpret_cast<AudioCodecPrimeInfo*>(outPropertyData))->leadingFrames = 0;
				(reinterpret_cast<AudioCodecPrimeInfo*>(outPropertyData))->trailingFrames = 0;
			}
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;

        case kAudioCodecUseRecommendedSampleRate:
  			if(ioPropertyDataSize == sizeof(UInt32))
			{
				*reinterpret_cast<UInt32*>(outPropertyData) = 0;
			}
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;

        case kAudioCodecOutputPrecedence:
  			if(ioPropertyDataSize == sizeof(UInt32))
			{
				*reinterpret_cast<UInt32*>(outPropertyData) = kAudioCodecOutputPrecedenceNone;
			}
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;

        case kAudioCodecDoesSampleRateConversion:
  			if(ioPropertyDataSize == sizeof(UInt32))
			{
				*reinterpret_cast<UInt32*>(outPropertyData) = 0;
			}
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;

#ifdef kAudioCodecRequiresMagicCookie
        case kAudioCodecRequiresMagicCookie:
  			if(ioPropertyDataSize == sizeof(UInt32))
			{
				*reinterpret_cast<UInt32*>(outPropertyData) = 0;
			}
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;
#endif
		default:
			CODEC_THROW(kAudioCodecUnknownPropertyError);
			break;
			
	};
}

void	ACBaseCodec::SetProperty(AudioCodecPropertyID inPropertyID, UInt32 inPropertyDataSize, const void* inPropertyData)
{
	switch(inPropertyID)
	{
		case kAudioCodecPropertyMinimumNumberInputPackets :
			CODEC_THROW(kAudioCodecIllegalOperationError);
			break;
		case kAudioCodecPropertyMinimumNumberOutputPackets :
			CODEC_THROW(kAudioCodecIllegalOperationError);
			break;

		case kAudioCodecPropertyInputChannelLayout :
		case kAudioCodecPropertyOutputChannelLayout :
			// by default a codec doesn't support channel layouts.
			CODEC_THROW(kAudioCodecIllegalOperationError);
			break;

		case kAudioCodecPropertyAvailableInputChannelLayouts :
		case kAudioCodecPropertyAvailableOutputChannelLayouts :
			// by default a codec doesn't support channel layouts.
			CODEC_THROW(kAudioCodecIllegalOperationError);
			break;

		case kAudioCodecPropertyCurrentInputFormat:
			if(inPropertyDataSize == sizeof(AudioStreamBasicDescription))
			{
				SetCurrentInputFormat(*reinterpret_cast<const AudioStreamBasicDescription*>(inPropertyData));
			}
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;
			
		case kAudioCodecPropertyCurrentOutputFormat:
			if(inPropertyDataSize == sizeof(AudioStreamBasicDescription))
			{
				SetCurrentOutputFormat(*reinterpret_cast<const AudioStreamBasicDescription*>(inPropertyData));
			}
			else
			{
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			break;
			
		case kAudioCodecPropertyMagicCookie:
			SetMagicCookie(inPropertyData, inPropertyDataSize);
			break;
			
		case kAudioCodecPropertyHasVariablePacketByteSizes:
		case kAudioCodecPropertyInputBufferSize:
		case kAudioCodecPropertyNameCFString:
		case kAudioCodecPropertyManufacturerCFString:
		case kAudioCodecPropertyFormatCFString:
		case kAudioCodecPropertySupportedInputFormats:
		case kAudioCodecPropertySupportedOutputFormats:
		case kAudioCodecPropertyUsedInputBufferSize:
		case kAudioCodecPropertyIsInitialized:
		case kAudioCodecPropertyAvailableNumberChannels:
		case kAudioCodecPropertyPrimeMethod:
		case kAudioCodecPropertyPrimeInfo:
		case kAudioCodecOutputFormatsForInputFormat:
		case kAudioCodecInputFormatsForOutputFormat:
		case kAudioCodecUseRecommendedSampleRate:
		case kAudioCodecOutputPrecedence:
		case kAudioCodecDoesSampleRateConversion:
#ifdef kAudioCodecRequiresMagicCookie
		case kAudioCodecRequiresMagicCookie:
#endif
		case kAudioCodecPropertyRequiresPacketDescription:
			CODEC_THROW(kAudioCodecIllegalOperationError);
			break;
			
		default:
			CODEC_THROW(kAudioCodecUnknownPropertyError);
			break;
			
	};
}

void	ACBaseCodec::Initialize(const AudioStreamBasicDescription* inInputFormat, const AudioStreamBasicDescription* inOutputFormat, const void* inMagicCookie, UInt32 inMagicCookieByteSize)
{
	mIsInitialized = true;
}

void	ACBaseCodec::Uninitialize()
{
	mIsInitialized = false;
}

void	ACBaseCodec::Reset()
{
}

UInt32	ACBaseCodec::GetNumberSupportedInputFormats() const
{
	return mInputFormatList.size();
}

void	ACBaseCodec::GetSupportedInputFormats(AudioStreamBasicDescription* outInputFormats, UInt32& ioNumberInputFormats) const
{
	UInt32 theNumberFormats = mInputFormatList.size();
	ioNumberInputFormats = (theNumberFormats < ioNumberInputFormats) ? theNumberFormats : ioNumberInputFormats;
	
	FormatList::const_iterator theIterator = mInputFormatList.begin();
	theNumberFormats = ioNumberInputFormats;
	while((theNumberFormats > 0) && (theIterator != mInputFormatList.end()))
	{
		*outInputFormats = *theIterator;
		
		++outInputFormats;
		--theNumberFormats;
		std::advance(theIterator, 1);
	}
}

void	ACBaseCodec::GetCurrentInputFormat(AudioStreamBasicDescription& outInputFormat)
{
	outInputFormat = mInputFormat;
}

void	ACBaseCodec::SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat)
{
	if(!mIsInitialized)
	{
		mInputFormat = inInputFormat;
	}
	else
	{
		CODEC_THROW(kAudioCodecStateError);
	}
}

UInt32	ACBaseCodec::GetNumberSupportedOutputFormats() const
{
	return mOutputFormatList.size();
}

void	ACBaseCodec::GetSupportedOutputFormats(AudioStreamBasicDescription* outOutputFormats, UInt32& ioNumberOutputFormats) const
{
	UInt32 theNumberFormats = mOutputFormatList.size();
	ioNumberOutputFormats = (theNumberFormats < ioNumberOutputFormats) ? theNumberFormats : ioNumberOutputFormats;
	
	FormatList::const_iterator theIterator = mOutputFormatList.begin();
	theNumberFormats = ioNumberOutputFormats;
	while((theNumberFormats > 0) && (theIterator != mOutputFormatList.end()))
	{
		*outOutputFormats = *theIterator;
		
		++outOutputFormats;
		--theNumberFormats;
		std::advance(theIterator, 1);
	}
}

void	ACBaseCodec::GetCurrentOutputFormat(AudioStreamBasicDescription& outOutputFormat)
{
	outOutputFormat = mOutputFormat;
}

void	ACBaseCodec::SetCurrentOutputFormat(const AudioStreamBasicDescription& inOutputFormat)
{
	if(!mIsInitialized)
	{
		mOutputFormat = inOutputFormat;
	}
	else
	{
		CODEC_THROW(kAudioCodecStateError);
	}
}

UInt32	ACBaseCodec::GetMagicCookieByteSize() const
{
	return 0;
}

void	ACBaseCodec::GetMagicCookie(void* outMagicCookieData, UInt32& ioMagicCookieDataByteSize) const
{
	ioMagicCookieDataByteSize = 0;
}

void	ACBaseCodec::SetMagicCookie(const void* outMagicCookieData, UInt32 inMagicCookieDataByteSize)
{
	if(mIsInitialized)
	{
		CODEC_THROW(kAudioCodecStateError);
	}
}

void	ACBaseCodec::AddInputFormat(const AudioStreamBasicDescription& inInputFormat)
{
	FormatList::iterator theIterator = std::find(mInputFormatList.begin(), mInputFormatList.end(), inInputFormat);
	if(theIterator == mInputFormatList.end())
	{
		theIterator = std::lower_bound(mInputFormatList.begin(), mInputFormatList.end(), inInputFormat);
		mInputFormatList.insert(theIterator, inInputFormat);
	}
}

void	ACBaseCodec::AddOutputFormat(const AudioStreamBasicDescription& inOutputFormat)
{
	FormatList::iterator theIterator = std::find(mOutputFormatList.begin(), mOutputFormatList.end(), inOutputFormat);
	if(theIterator == mOutputFormatList.end())
	{
		theIterator = std::lower_bound(mOutputFormatList.begin(), mOutputFormatList.end(), inOutputFormat);
		mOutputFormatList.insert(theIterator, inOutputFormat);
	}
}
