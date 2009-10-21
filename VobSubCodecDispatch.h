/*
 * VobSubCodecDispatch.h
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

#if !TARGET_CPU_68K
	ComponentSelectorOffset (8)
#else
	ComponentSelectorOffset (6)
#endif

	ComponentRangeCount (3)
	ComponentRangeShift (8)
	ComponentRangeMask	(FF)

	ComponentRangeBegin (0)
		StdComponentCall (GetMPWorkFunction)
		ComponentError	 (Unregister)
		StdComponentCall (Target)
		ComponentError   (Register)
		StdComponentCall (Version)
		StdComponentCall (CanDo)
		StdComponentCall (Close)
		StdComponentCall (Open)
	ComponentRangeEnd (0)
		
	ComponentRangeBegin (1)
		ComponentCall     (GetCodecInfo)
		ComponentDelegate (GetCompressionTime)
		ComponentDelegate (GetMaxCompressionSize)
		ComponentDelegate (PreCompress)
		ComponentDelegate (BandCompress)
		ComponentDelegate (PreDecompress)
		ComponentDelegate (BandDecompress)
		ComponentDelegate (Busy)
		ComponentDelegate (GetCompressedImageSize)
		ComponentDelegate (GetSimilarity)
		ComponentDelegate (TrimImage)
		ComponentDelegate (RequestSettings)
		ComponentDelegate (GetSettings)
		ComponentDelegate (SetSettings)
		ComponentDelegate (Flush)
		ComponentDelegate (SetTimeCode)
		ComponentDelegate (IsImageDescriptionEquivalent)
		ComponentDelegate (NewMemory)
		ComponentDelegate (DisposeMemory)
		ComponentDelegate (HitTestData)
		ComponentDelegate (NewImageBufferMemory)
		ComponentDelegate (ExtractAndCombineFields)
		ComponentDelegate (GetMaxCompressionSizeWithSources)
		ComponentDelegate (SetTimeBase)
		ComponentDelegate (SourceChanged)
		ComponentDelegate (FlushLastFrame)
		ComponentDelegate (GetSettingsAsText)
		ComponentDelegate (GetParameterListHandle)
		ComponentDelegate (GetParameterList)
		ComponentDelegate (CreateStandardParameterDialog)
		ComponentDelegate (IsStandardParameterDialogEvent)
		ComponentDelegate (DismissStandardParameterDialog)
		ComponentDelegate (StandardParameterDialogDoAction)
		ComponentDelegate (NewImageGWorld)
		ComponentDelegate (DisposeImageGWorld)
		ComponentDelegate (HitTestDataWithFlags)
		ComponentDelegate (ValidateParameters)
		ComponentDelegate (GetBaseMPWorkFunction)
		ComponentDelegate (0x0026)
		ComponentDelegate (0x0027)
		ComponentDelegate (RequestGammaLevel)
		ComponentCall     (GetSourceDataGammaLevel)
		ComponentDelegate (0x002A)
		ComponentDelegate (GetDecompressLatency)
		ComponentDelegate (MergeFloatingImageOntoWindow)
		ComponentDelegate (RemoveFloatingImage)
		ComponentDelegate (GetDITLForSize)
		ComponentDelegate (DITLInstall)
		ComponentDelegate (DITLEvent)
		ComponentDelegate (DITLItem)
		ComponentDelegate (DITLRemove)
		ComponentDelegate (DITLValidateInput)
        ComponentDelegate (0x0034)
        ComponentDelegate (0x0035)
        ComponentDelegate (GetPreferredChunkSizeAndAlignment)
		ComponentDelegate (PrepareToCompressFrames)
		ComponentDelegate (EncodeFrame)
		ComponentDelegate (CompleteFrame)
    	ComponentDelegate (BeginPass)
    	ComponentDelegate (EndPass)
		ComponentDelegate (ProcessBetweenPasses)
	ComponentRangeEnd (1)

	ComponentRangeUnused (2)

	ComponentRangeBegin (3)
		ComponentCall (Preflight)
		ComponentCall (Initialize)
		ComponentCall (BeginBand)
		ComponentCall (DrawBand)
		ComponentCall (EndBand)
		ComponentDelegate (QueueStarting)
		ComponentDelegate (QueueStopping)
		ComponentDelegate (DroppingFrame)
		ComponentDelegate (ScheduleFrame)
		ComponentDelegate (CancelTrigger)
		ComponentDelegate (0x020A)
		ComponentDelegate (0x020B)
		ComponentDelegate (0x020C)
		ComponentDelegate (0x020D)
		ComponentDelegate (0x020E)
		ComponentCall (DecodeBand)
	ComponentRangeEnd (3)
