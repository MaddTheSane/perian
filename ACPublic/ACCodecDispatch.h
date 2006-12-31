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
	ACCodecDispatch.h

=============================================================================*/
#if !defined(__ACCodecDispatch_h__)
#define __ACCodecDispatch_h__

//=============================================================================
//	Includes
//=============================================================================

#include "ACCodec.h"
#include "ACCodecDispatchTypes.h"

//=============================================================================
//	ACCodecDispatch
//
//	A function template that implements a component dispatch routine for an
//	AudioCodec in terms of an instance of a subclass of ACCodec.
//	It is best used when called from the primary codec dispatcher like so:
//	
//	extern "C"
//	ComponentResult	FooCodecDispatch(ComponentParameters* inParameters, FooCodec* inThis)
//	{
//		return	ACCodecDispatch(inParameters, inThis);
//	}
//
//	The reason this exists is to better encapuslate all the necessary code
//	to do the dispatching without having to figure out what the C++ mangled
//	name for the entry point to put in the exported symbols file.
//=============================================================================

template <class CodecClass>
ComponentResult ACCodecDispatch(ComponentParameters* inParameters, CodecClass* inThis)
{
	ComponentResult	theError = kAudioCodecNoError;
	
	try
	{
		switch (inParameters->what)
		{
			//	these selectors don't use the object pointer
			
			case kComponentOpenSelect:
				{
					CodecClass*	theCodec = new CodecClass();
					SetComponentInstanceStorage(((AudioCodecOpenGluePB*)inParameters)->inCodec, (Handle)theCodec);
				}
				break;
	
			case kComponentCloseSelect:
				delete inThis;
				break;
			
			case kComponentCanDoSelect:
				switch (*((SInt16*)&inParameters->params[1]))
				{
					case kComponentOpenSelect:
					case kComponentCloseSelect:
					case kComponentCanDoSelect:
					case kComponentRegisterSelect:
					case kComponentVersionSelect:
					case kAudioCodecGetPropertyInfoSelect:
					case kAudioCodecGetPropertySelect:
					case kAudioCodecSetPropertySelect:
					case kAudioCodecInitializeSelect:
					case kAudioCodecAppendInputDataSelect:
					case kAudioCodecProduceOutputDataSelect:
					case kAudioCodecResetSelect:
						theError = 1;
						break;
				};
				break;
				
			default:
				//	these selectors use the object pointer
				if(inThis != NULL)
				{
					switch (inParameters->what)
					{
						case kComponentRegisterSelect:
							if(!inThis->Register())
							{
								theError = componentDontRegister;
							}
							break;
				
						case kComponentVersionSelect:
							theError = inThis->GetVersion();
							break;
						
						case kAudioCodecGetPropertyInfoSelect:
							{
								AudioCodecGetPropertyInfoGluePB* thePB = (AudioCodecGetPropertyInfoGluePB*)inParameters;
								UInt32 theSize = 0;
								bool isWritable = false;
								
								inThis->GetPropertyInfo(thePB->inPropertyID, theSize, isWritable);
								if(thePB->outSize != NULL)
								{
									*(thePB->outSize) = theSize;
								}
								if(thePB->outWritable != NULL)
								{
									*(thePB->outWritable) = isWritable ? 1 : 0;
								}
							}
							break;
				
						case kAudioCodecGetPropertySelect:
							{
								AudioCodecGetPropertyGluePB* thePB = (AudioCodecGetPropertyGluePB*)inParameters;
								
								if((thePB->ioPropertyDataSize != NULL) && (thePB->outPropertyData != NULL))
								{
									inThis->GetProperty(thePB->inPropertyID, *(thePB->ioPropertyDataSize), thePB->outPropertyData);
								}
								else
								{
									theError = paramErr;
								}
							}
							break;
				
						case kAudioCodecSetPropertySelect:
							{
								AudioCodecSetPropertyGluePB* thePB = (AudioCodecSetPropertyGluePB*)inParameters;
								
								if(thePB->inPropertyData != NULL)
								{
									inThis->SetProperty(thePB->inPropertyID, thePB->inPropertyDataSize, thePB->inPropertyData);
								}
								else
								{
									theError = paramErr;
								}
							}
							break;
				
						case kAudioCodecInitializeSelect:
							{
								AudioCodecInitializeGluePB* thePB = (AudioCodecInitializeGluePB*)inParameters;
								
								inThis->Initialize(thePB->inInputFormat, thePB->inOutputFormat, thePB->inMagicCookie, thePB->inMagicCookieByteSize);
							}
							break;
				
						case kAudioCodecUninitializeSelect:
							{
								//AudioCodecUninitializeGluePB* thePB = (AudioCodecUninitializeGluePB*)inParameters;
								
								inThis->Uninitialize();
							}
							break;
				
						case kAudioCodecAppendInputDataSelect:
							{
								AudioCodecAppendInputDataGluePB* thePB = (AudioCodecAppendInputDataGluePB*)inParameters;
								
								if((thePB->inInputData != NULL) && (thePB->ioInputDataByteSize != NULL))
								{
									if(thePB->ioNumberPackets != NULL)
									{
										inThis->AppendInputData(thePB->inInputData, *(thePB->ioInputDataByteSize), *(thePB->ioNumberPackets), thePB->inPacketDescription);
									}
									else
									{
										UInt32 theNumberPackets = 0;
										inThis->AppendInputData(thePB->inInputData, *(thePB->ioInputDataByteSize), theNumberPackets, thePB->inPacketDescription);
									}
								}
								else
								{
									theError = paramErr;
								}
							}
							break;
				
						case kAudioCodecProduceOutputDataSelect:
							{
								AudioCodecProduceOutputPacketsGluePB* thePB = (AudioCodecProduceOutputPacketsGluePB*)inParameters;
								
								if((thePB->outOutputData != NULL) && (thePB->ioOutputDataByteSize != NULL) && (thePB->ioNumberPackets != NULL) && (thePB->outStatus != NULL))
								{
									*(thePB->outStatus) = inThis->ProduceOutputPackets(thePB->outOutputData, *(thePB->ioOutputDataByteSize), *(thePB->ioNumberPackets), thePB->outPacketDescription);
                                                                        if(kAudioCodecProduceOutputPacketFailure == *(thePB->outStatus))
                                                                        {
                                                                            theError = paramErr;
                                                                        }
                                                                }
								else
								{
									theError = paramErr;
								}
							}
							break;
				
						case kAudioCodecResetSelect:
							{
								//AudioCodecResetGluePB* thePB = (AudioCodecResetGluePB*)inParameters;
								
								inThis->Reset();
							}
							break;
				
						default:
							theError = badComponentSelector;
							break;
					};
				}
				else
				{
					theError = paramErr;
				}
				break;
		};
	}
	catch(ComponentResult inErrorCode)
	{
		theError = inErrorCode;
	}
	catch(...)
	{
		theError = kAudioCodecUnspecifiedError;
	}
	
	return theError;
}

#endif
