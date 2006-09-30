/*
 *  MkvExportPrivate.cpp
 *
 *    MkvExportPrivate.cpp - C++ code for interfacing with libmatroska to export.
 *
 *
 *  Copyright (c) 2006  David Conrad
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; 
 *  version 2.1 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <QuickTime/QuickTime.h>
#include <iostream>

#include <ebml/c/libebml_t.h>
#include <ebml/IOCallback.h>
#include <ebml/EbmlHead.h>
#include <ebml/EbmlSubHead.h>
#include <ebml/EbmlVoid.h>
#include <ebml/EbmlVersion.h>
#include "DataHandlerCallback.h"

#include <matroska/KaxConfig.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxInfo.h>
#include <matroska/KaxInfoData.h>
#include <matroska/KaxVersion.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxTrackVideo.h>
#include <matroska/KaxTrackAudio.h>

#include "MatroskaExport.h"
#include "MkvExportPrivate.h"
#include "MatroskaCodecIDs.h"

using namespace std;
using namespace libmatroska;

ComponentResult WriteHeader(MatroskaExportGlobals glob)
{
	EbmlHead fileHead;

	EDocType & myDocType = GetChild<EDocType>(fileHead);
	*static_cast<EbmlString *>(&myDocType) = "matroska";

	EDocTypeVersion & myDocTypeVer = GetChild<EDocTypeVersion>(fileHead);
	*(static_cast<EbmlUInteger *>(&myDocTypeVer)) = MATROSKA_VERSION;

	EDocTypeReadVersion & myDocTypeReadVer = GetChild<EDocTypeReadVersion>(fileHead);
	*(static_cast<EbmlUInteger *>(&myDocTypeReadVer)) = 1;
	
	fileHead.Render(*glob->ioHandler, false);
	
	glob->segment = new KaxSegment();
	
	// size is unknown and will always be, we can render it right away
	glob->segmentSize = glob->segment->WriteHead(*glob->ioHandler, 5, false);
	
	// reserve 300 octets for the meta seek for the important elements
	glob->metaSeekPlaceholder = new EbmlVoid();
	glob->metaSeekPlaceholder->SetSize(300);
	glob->metaSeekPlaceholder->Render(*glob->ioHandler, false);
	glob->metaSeek = new KaxSeekHead();
	glob->cues = new KaxCues();
	
	KaxInfo & myInfos = GetChild<KaxInfo>(*glob->segment);
	
	KaxTimecodeScale & timeScale = GetChild<KaxTimecodeScale>(myInfos);
	if (glob->timecodeScale)
		*(static_cast<EbmlUInteger *>(&timeScale)) = glob->timecodeScale;
	else
		glob->timecodeScale = 1000000;	// default scale: 1 milisecond
	
	KaxDuration & segDuration = GetChild<KaxDuration>(myInfos);
	*(static_cast<EbmlFloat *>(&segDuration)) = 
		Float64(WideToSInt64(glob->duration.value)) * 1e9 / 
		(glob->duration.scale * UInt64(timeScale));
	
	UTFstring muxingApp, writingApp;
	muxingApp.SetUTF8(string("libebml ") + EbmlCodeVersion + " & libmatroska " + KaxCodeVersion);
	writingApp.SetUTF8(string("Perian 0.5"));
	*((EbmlUnicodeString *)&GetChild<KaxMuxingApp>(myInfos)) = muxingApp;
	*((EbmlUnicodeString *)&GetChild<KaxWritingApp>(myInfos)) = writingApp;
	
	GetChild<KaxDateUTC>(myInfos).SetEpochDate(time(NULL));
	
	// segment UID is 128 random bits
	uint8 segUID[128 / 8];
	srandom(time(NULL));
	for (int i = 0; i < 128 / 8; i++)
		segUID[i] = random();
	KaxSegmentUID &kaxSegUID = GetChild<KaxSegmentUID>(myInfos);
	kaxSegUID.CopyBuffer(segUID, 128 / 8);
	
	glob->actSegmentSize += myInfos.Render(*glob->ioHandler);
	glob->metaSeek->IndexThis(myInfos, *glob->segment);
	
	return noErr;
}

ComponentResult GetTrackInfoCopy(MatroskaExportGlobals glob, KaxTracks &trackInfo, 
								 KaxTrackEntry &track, OutputTrack &outputTrack)
{
	ComponentResult err = noErr;
	Media theMedia = GetTrackMedia(outputTrack.theTrack);
	SampleDescriptionHandle desc = (SampleDescriptionHandle) NewHandle(0);
	OSType mediaType;
	
	GetMediaSampleDescription(theMedia, 1, desc);
	GetMediaHandlerDescription(theMedia, &mediaType, NULL, NULL);
	
	KaxTrackType & trackType = GetChild<KaxTrackType>(track);
	KaxCodecID & codecID = GetChild<KaxCodecID>(track);
	
	// native codec id is preferred; look it up
	for (int j = 0; j < sizeof(kMatroskaCodecIDs) / sizeof(MatroskaQT_Codec); j++) {
		if ((*desc)->dataFormat == kMatroskaCodecIDs[j].cType) {
			*static_cast<EbmlString *>(&codecID) = kMatroskaCodecIDs[j].mkvID;
		}
	}
	
	if (mediaType == VideoMediaType) {
		ImageDescriptionHandle imgDesc = (ImageDescriptionHandle) desc;
		KaxTrackVideo & trackVideo = GetChild<KaxTrackVideo>(track);
		*(static_cast<EbmlUInteger *>(&trackType)) = track_video;
		
		// no native codec id; use vfw compatibility mode if we have a strf extension
		// otherwise QT compatibility mode; though we should have an option to force vfw
		// since support for QT compatibility isn't as good
		if (codecID.GetSize() == 0) {
			long count;
			CountImageDescriptionExtensionType(imgDesc, 'strf', &count);
			
			if (count > 0) {
				Handle imgDescHandle = NewHandle(0);
				GetImageDescriptionExtension(imgDesc, &imgDescHandle, 'strf', 1);
				
				*static_cast<EbmlString *>(&codecID) = MKV_V_MS;
				KaxCodecPrivate & codecPrivate = GetChild<KaxCodecPrivate>(track);
				codecPrivate.CopyBuffer((binary *) *imgDescHandle, GetHandleSize(imgDescHandle));
				
				DisposeHandle(imgDescHandle);
			} else {
				return -1;
			}
		}
		
		KaxVideoPixelWidth & pxlWidth = GetChild<KaxVideoPixelWidth>(trackVideo);
		KaxVideoPixelHeight & pxlHeight = GetChild<KaxVideoPixelHeight>(trackVideo);
		//KaxVideoDisplayWidth & dispWidth = GetChild<KaxVideoDisplayWidth>(trackVideo);
		//KaxVideoDisplayHeight & dispHeight = GetChild<KaxVideoDisplayHeight>(trackVideo);
		*(static_cast<EbmlUInteger *>(&pxlWidth)) = (*imgDesc)->width;
		*(static_cast<EbmlUInteger *>(&pxlHeight)) = (*imgDesc)->height;
		
		track.EnableLacing(false);
		
	} else if (mediaType == SoundMediaType) {
		SoundDescriptionHandle sndDesc = (SoundDescriptionHandle) desc;
		KaxTrackAudio & trackAudio = GetChild<KaxTrackAudio>(track);
		*(static_cast<EbmlUInteger *>(&trackType)) = track_audio;
		
		AudioStreamBasicDescription asbd = {0};
		err = QTSoundDescriptionGetProperty(sndDesc, kQTPropertyClass_SoundDescription,
											kQTSoundDescriptionPropertyID_AudioStreamBasicDescription,
											sizeof(asbd), &asbd, NULL);
		
		KaxAudioSamplingFreq & sampleFreq = GetChild<KaxAudioSamplingFreq>(trackAudio);
		*(static_cast<EbmlFloat *>(&sampleFreq)) = asbd.mSampleRate;
		
		// only matters for uncompressed formats; it's set to 0 for compressed
		if (asbd.mBitsPerChannel) {
			KaxAudioBitDepth & bitDepth = GetChild<KaxAudioBitDepth>(trackAudio);
			*(static_cast<EbmlUInteger *>(&bitDepth)) = asbd.mBitsPerChannel;
		}
		
		KaxAudioChannels & channels = GetChild<KaxAudioChannels>(trackAudio);
		*(static_cast<EbmlUInteger *>(&channels)) = asbd.mChannelsPerFrame;
		
		track.EnableLacing(true);
		
	} else {
		// we couldn't handle the track; remove it
		trackInfo.Remove(trackInfo.ListSize() - 1);
	}
	
	DisposeHandle((Handle) desc);
	return err;
}

ComponentResult GetTrackInfoEncode(MatroskaExportGlobals glob, KaxTrackEntry &track, 
								   OutputTrack &outputTrack)
{	
	ComponentResult err = noErr;
	MovieExportGetDataParams gdp = {0};
	gdp.recordSize = sizeof(MovieExportGetDataParams);
	gdp.trackID = outputTrack.trackID;
	gdp.sourceTimeScale = outputTrack.sourceTimeScale;
	
	err = InvokeMovieExportGetDataUPP(outputTrack.refCon, &gdp, outputTrack.getDataProc);
	if (err) return err;
	
	
	KaxTrackType & trackType = GetChild<KaxTrackType>(track);
	KaxCodecID & codecID = GetChild<KaxCodecID>(track);
	
	// native codec id is preferred; look it up
	for (int j = 0; j < sizeof(kMatroskaCodecIDs) / sizeof(MatroskaQT_Codec); j++) {
		if ((*gdp.desc)->dataFormat == kMatroskaCodecIDs[j].cType) {
			*static_cast<EbmlString *>(&codecID) = kMatroskaCodecIDs[j].mkvID;
		}
	}
	
	if (gdp.descType == VideoMediaType) {
		ImageDescriptionHandle imgDesc = (ImageDescriptionHandle) gdp.desc;
		*(static_cast<EbmlUInteger *>(&trackType)) = track_video;
		
		if (codecID.GetSize() == 0) {
			long count;
			// no native codec id; use vfw compatibility mode if we have a strf extension
			CountImageDescriptionExtensionType(imgDesc, 'strf', &count);
			if (count > 0) {
				Handle imgDescHandle = NewHandle(0);
				GetImageDescriptionExtension(imgDesc, &imgDescHandle, 'strf', 1);
				
				*static_cast<EbmlString *>(&codecID) = MKV_V_MS;
				KaxCodecPrivate & codecPrivate = GetChild<KaxCodecPrivate>(track);
				codecPrivate.CopyBuffer((binary *) *imgDescHandle, GetHandleSize(imgDescHandle));
				
				DisposeHandle(imgDescHandle);
			} else {
				return -1;
			}
		}
		
		KaxVideoPixelWidth & pxlWidth = GetChild<KaxVideoPixelWidth>(track);
		KaxVideoPixelHeight & pxlHeight = GetChild<KaxVideoPixelHeight>(track);
		//KaxVideoDisplayWidth & dispWidth = GetChild<KaxVideoDisplayWidth>(track);
		//KaxVideoDisplayHeight & dispHeight = GetChild<KaxVideoDisplayHeight>(track);
		*(static_cast<EbmlUInteger *>(&pxlWidth)) = (*imgDesc)->width;
		*(static_cast<EbmlUInteger *>(&pxlHeight)) = (*imgDesc)->height;
		
		track.EnableLacing(false);
		
	} else if (gdp.descType == SoundMediaType) {
		*(static_cast<EbmlUInteger *>(&trackType)) = track_audio;
		
		
	} else {
		printf("Unhandled media type %4.4s\n", &gdp.descType);
	}
	return err;
}

ComponentResult WriteTrackInfo(MatroskaExportGlobals glob)
{
	ComponentResult err = noErr;
	KaxTracks & trackInfo = GetChild<KaxTracks>(*glob->segment);
	KaxTrackEntry *track = NULL;
	
	for (int i = 0; i < glob->outputTracks->size(); i++) {
		if (track == NULL)
			track = &GetChild<KaxTrackEntry>(trackInfo);
		else
			track = &GetNextChild<KaxTrackEntry>(trackInfo, *track);
		
		track->SetGlobalTimecodeScale(glob->timecodeScale);
		
		KaxTrackNumber & trackNum = GetChild<KaxTrackNumber>(*track);
		*(static_cast<EbmlUInteger *>(&trackNum)) = i + 1;
		
		KaxTrackUID & trackUID = GetChild<KaxTrackUID>(*track);
		*(static_cast<EbmlUInteger *>(&trackUID)) = random();
		
		if (glob->outputTracks->at(i).theTrack)
			GetTrackInfoCopy(glob, trackInfo, *track, glob->outputTracks->at(i));
		else
			GetTrackInfoEncode(glob, *track, glob->outputTracks->at(i));
	}
	
	glob->actSegmentSize += trackInfo.Render(*glob->ioHandler, false);
	glob->metaSeek->IndexThis(trackInfo, *glob->segment);
	
	return err;
}

ComponentResult StartNewCluster(MatroskaExportGlobals glob, TimeValue64 atTime)
{
	if (glob->currentCluster) {
		glob->segmentSize += glob->currentCluster->Render(*glob->ioHandler, *glob->cues, false);
		glob->currentCluster->ReleaseFrames();
		glob->metaSeek->IndexThis(*glob->currentCluster, *glob->segment);
		delete glob->currentCluster;
	}
	glob->currentCluster = new KaxCluster();
	glob->currentCluster->SetParent(*glob->segment);
	glob->currentCluster->SetPreviousTimecode(atTime * glob->timecodeScale, glob->timecodeScale);
	glob->currentCluster->EnableChecksum();
	return noErr;
}

ComponentResult FinishClusters(MatroskaExportGlobals glob)
{
	if (glob->currentCluster) {
		glob->segmentSize += glob->currentCluster->Render(*glob->ioHandler, *glob->cues, false);
		glob->currentCluster->ReleaseFrames();
		glob->metaSeek->IndexThis(*glob->currentCluster, *glob->segment);
		delete glob->currentCluster;
		glob->currentCluster = NULL;
	}
	return noErr;
}

ComponentResult WriteFrame(MatroskaExportGlobals glob)
{
	ComponentResult err = noErr;
	Media theMedia;
	int trackNum = 0;
	TimeValue64 decodeTime = glob->outputTracks->at(0).currentTime;
	TimeValue64 decodeDuration, displayOffset;
	MediaSampleFlags sampleFlags;
	ByteCount sampleDataSize;
	
	// find the track with the next earliest sample
	for (int i = 1; i < glob->outputTracks->size(); i++) {
		if (glob->outputTracks->at(i).currentTime < decodeTime && 
			glob->outputTracks->at(i).currentTime >= 0) {
			
			trackNum = i;
			decodeTime = glob->outputTracks->at(i).currentTime;
		}
	}
	
	theMedia = GetTrackMedia(glob->outputTracks->at(trackNum).theTrack);
	
	// update the track's current time
	GetMediaNextInterestingDecodeTime(theMedia, nextTimeMediaSample, decodeTime, fixed1,
									  &glob->outputTracks->at(trackNum).currentTime, NULL);
	
	// get the sample size
	err = GetMediaSample2(theMedia, NULL, 0, &sampleDataSize, decodeTime, 
						  NULL, NULL, NULL, NULL, NULL, 1, NULL, NULL);
	if (err) return err;
	
	// make sure our buffer is large enough
	if (sampleDataSize > GetPtrSize(glob->sampleBuffer))
		SetPtrSize(glob->sampleBuffer, sampleDataSize);
	
	// and actually fetch the sample
	err = GetMediaSample2(theMedia, (UInt8 *) glob->sampleBuffer, GetPtrSize(glob->sampleBuffer),
						  NULL, decodeTime, NULL, &decodeDuration, &displayOffset, 
						  NULL, NULL, 1, NULL, &sampleFlags);
	
	DataBuffer data((binary *)glob->sampleBuffer, sampleDataSize);
	
	// TODO: import with references to correct frames
	//glob->currentCluster->AddFrame(*glob->outputTracks->at(trackNum).kaxTrack, 
								   
	
	return err;
}

ComponentResult FinishFile(MatroskaExportGlobals glob)
{
	// write the seek head where it goes
	glob->actSegmentSize += glob->metaSeekPlaceholder->ReplaceWith(*glob->metaSeek, *glob->ioHandler, false);
	
	if (glob->segment->ForceSize(glob->segmentSize - glob->segment->HeadSize() + glob->actSegmentSize))
		glob->segment->OverwriteHead(*glob->ioHandler);
	
	return noErr;
}


