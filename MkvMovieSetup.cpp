/*
 *  MkvMovieSetup.cpp
 *
 *    MkvMovieSetup.cpp - Functions to create the structure of a Matroska file as a QuickTime movie.
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

#include "MatroskaCodecIDs.h"
#include "MkvImportPrivate.h"
#include "MkvMovieSetup.h"
#include "SubImport.h"
#include <QuickTime/QuickTime.h>
#include <AudioToolbox/AudioToolbox.h>

#include <ebml/EbmlHead.h>
#include <ebml/EbmlSubHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlVersion.h>
#include <ebml/EbmlVoid.h>

#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxChapters.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxClusterData.h>
#include <matroska/KaxInfo.h>
#include <matroska/KaxInfoData.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxTrackEntryData.h>
#include <matroska/KaxTrackAudio.h>
#include <matroska/KaxTrackVideo.h>
#include <matroska/KaxContentEncoding.h>

using namespace std;
using namespace libmatroska;

ComponentResult MkvCreateVideoTrack(MkvTrackPtr mkvTrack, KaxTrackEntry *tr_entry, 
									Movie theMovie, Handle dataRef, OSType dataRefType);
ComponentResult MkvCreateAudioTrack(MkvTrackPtr mkvTrack, KaxTrackEntry *tr_entry, 
									Movie theMovie, Handle dataRef, OSType dataRefType);
ComponentResult MkvCreateSubtitleTrack(MkvTrackPtr mkvTrack, KaxTrackEntry *tracks, 
									   Movie theMovie, Handle dataRef, OSType dataRefType);
int GetAACProfile(KaxTrackEntry *tr_entry);
UInt32 GetDefaultChannelLayout(AudioStreamBasicDescription &asbd);
void FinishSampleDescription(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc);
typedef struct PixelAspectRatio {UInt32 hSpacing, vSpacing;} PixelAspectRatio;

ComponentResult ReadSegmentInfo(KaxInfo *segmentInfo, MkvSegment *segment, Movie theMovie)
{
	QTMetaDataRef movieMetaData;
	OSStatus err;
	KaxSegmentUID *uid = FindChild<KaxSegmentUID>(*segmentInfo);
	KaxDuration *duration = FindChild<KaxDuration>(*segmentInfo);
	KaxTimecodeScale *timecodeScale = FindChild<KaxTimecodeScale>(*segmentInfo);
	KaxTitle *title = FindChild<KaxTitle>(*segmentInfo);
	KaxWritingApp *writingApp = FindChild<KaxWritingApp>(*segmentInfo);
	
	if (uid)
		memcpy(segment->UID, uid->GetBuffer(), uid->GetSize());
	else
		return invalidAtomErr;
	
	if (duration)
		segment->duration = Float64(*duration);
	
	if (timecodeScale)
		segment->timecodeScale = UInt64(*timecodeScale);
	else
		segment->timecodeScale = TIMECODE_SCALE_DEFAULT;
	
	err = QTCopyMovieMetaData(theMovie, &movieMetaData);
	if (err == noErr) {
		OSType key;
		if (title) {
			key = kQTMetaDataCommonKeyDisplayName;
			QTMetaDataAddItem(movieMetaData, 
							  kQTMetaDataStorageFormatQuickTime, kQTMetaDataKeyFormatCommon, 
							  (UInt8 *)&key, sizeof(key), 
							  (UInt8 *)UTFstring(*title).GetUTF8().c_str(), 
							  UTFstring(*title).GetUTF8().size(), 
							  kQTMetaDataTypeUTF8, NULL);
		}
		if (writingApp) {
			key = kQTMetaDataCommonKeySoftware;
			QTMetaDataAddItem(movieMetaData, 
							  kQTMetaDataStorageFormatQuickTime, kQTMetaDataKeyFormatCommon, 
							  (UInt8 *)&key, sizeof(key), 
							  (UInt8 *)UTFstring(*writingApp).GetUTF8().c_str(), 
							  UTFstring(*writingApp).GetUTF8().size(), 
							  kQTMetaDataTypeUTF8, NULL);
		}
		QTMetaDataRelease(movieMetaData);
	}
	
	return noErr;
}

void AddChapterAtom(KaxChapterAtom *atom, KaxPrivate *priv, MkvSegment *segment)
{
	KaxChapterAtom *subChapter = FindChild<KaxChapterAtom>(*atom);
	
	// since QuickTime only supports linear chapter tracks (no nesting), only add chapter leaves
	if (subChapter && subChapter->GetSize() > 0) {
		while (subChapter && subChapter->GetSize() > 0) {
			AddChapterAtom(subChapter, priv, segment);
			subChapter = FindNextChild(*atom, *subChapter);
		}
	} else {
		// add the chapter to the track if it has no children
		KaxChapterTimeStart *startTime = FindChild<KaxChapterTimeStart>(*atom);
		if (priv->orderedChapters) {
			KaxChapterTimeEnd *endTime = FindChild<KaxChapterTimeEnd>(*atom);
			KaxChapterSegmentUID *segmentUID = FindChild<KaxChapterSegmentUID>(*atom);
			MkvOrderedChapter edit;
			if (segmentUID == NULL)
				memcpy(edit.segmentUID, segment->UID, 16);
			else
				memcpy(edit.segmentUID, segmentUID->GetBuffer(), segmentUID->GetSize());
			edit.segmentStart = UInt64(*startTime);
			edit.segmentEnd = UInt64(*endTime);
			priv->editList.push_back(edit);
		}
		
		KaxChapterDisplay *chapDisplay = FindChild<KaxChapterDisplay>(*atom);
		KaxChapterString *chapString = FindChild<KaxChapterString>(*chapDisplay);
		MediaHandler mh = GetMediaHandler(priv->chapterMedia);
		
		Rect bounds = {0, 0, 0, 0};
		TimeValue inserted;
		OSErr err = TextMediaAddTextSample(mh, 
										   const_cast<Ptr>(UTFstring(*chapString).GetUTF8().c_str()), 
										   UTFstring(*chapString).GetUTF8().size(), 
										   0, 0, 0, NULL, NULL, 
										   teCenter, &bounds, dfClipToTextBox, 
										   0, 0, 0, NULL, 1, &inserted);
		if (err)
			printf("MatroskaQT: Error adding text sample %d\n", err);
		else {
			TimeValue start = UInt64(*startTime) / segment->timecodeScale;
			if (priv->orderedChapters) {
				// the start time in ordered chapters refers to the start time in the segment
				// since we're constructing one movie from all segments, we need the overall time point
				start = 0;
				for (int i = 0; i < priv->editList.size() - 1; i++)
					start += (priv->editList[i].segmentEnd - priv->editList[i].segmentStart) / 
						segment->timecodeScale;
			} else {
				// if we have multiple segments in a file, chapters from the second should come 
				// after chapters from the first, so add that offset
				for (int i = 0; &priv->segments[i] != segment; i++)
					start += TimeValue(priv->segments[i].duration);
			}
			InsertMediaIntoTrack(priv->chapterTrack, start, inserted, 1, fixed1);
		}
	}
}

ComponentResult SetupChapters(KaxChapters *chapters, KaxPrivate *priv, MkvSegment *segment)
{
	vector<KaxChapterAtom> chapterAtoms;
	KaxChapterAtom *chapterAtom;
	KaxEditionEntry *edition = FindChild<KaxEditionEntry>(*chapters);
	KaxEditionFlagOrdered *ordered = FindChild<KaxEditionFlagOrdered>(*edition);
	
	if (priv->orderedChapters)
		// Ordered Chapters need be setup only once
		return noErr;
	
	if (priv->chapterTrack == NULL) {
		priv->chapterTrack = NewMovieTrack(priv->theMovie, 0, 0, kNoVolume);
		if (priv->chapterTrack == NULL) {
			printf("MatroskaQT: Error creating chapter track %d\n", GetMoviesError());
			return invalidTrack;
		}
		
		// we use a handle data reference here because I don't see any way to add textual 
		// sample references (TextMediaAddTextSample() will behave the same as AddSample()
		// in that it modifies the original file if that's the data reference of the media)
		Handle dataRef = NULL;
		Handle dataRefData = NewHandle(0);
		PtrToHand(&dataRefData, &dataRef, sizeof(Handle));
		priv->chapterMedia = NewTrackMedia(priv->chapterTrack, TextMediaType, 
										   GetMovieTimeScale(priv->theMovie), 
										   dataRef, HandleDataHandlerSubType);
		if (priv->chapterMedia == NULL) {
			printf("MatroskaQT: Error creating chapter media %d\n", GetMoviesError());
			return invalidMedia;
		}
		
		// Name the chapter track "Chapters" for easy distinguishing
		QTMetaDataRef trackMetaData;
		OSErr err = QTCopyTrackMetaData(priv->chapterTrack, &trackMetaData);
		if (err == noErr) {
			OSType key = kUserDataName;
			string chapterName("Chapters");
			QTMetaDataAddItem(trackMetaData, kQTMetaDataStorageFormatUserData,
							  kQTMetaDataKeyFormatUserData, (UInt8 *)&key, sizeof(key),
							  (UInt8 *)chapterName.c_str(), chapterName.size(),
							  kQTMetaDataTypeUTF8, NULL);
			QTMetaDataRelease(trackMetaData);
		}	
	}
	
	// Ordered chapters are very different from normal chapters; they act as an edit list
	// we'll need to save the start time, end time and segment UID so we can constuct the movie properly
	if (ordered && uint8(*ordered)) 
		priv->orderedChapters = true;
	
	BeginMediaEdits(priv->chapterMedia);
	
	// tell the text media handler the upcoming text samples are
	// encoded in Unicode with a byte order mark (BOM)
	MediaHandler mediaHandler = GetMediaHandler(priv->chapterMedia);
	SInt32 dataPtr = kTextEncodingUnicodeDefault;
	TextMediaSetTextSampleData(mediaHandler, &dataPtr, kTXNTextEncodingAttribute);
	
	chapterAtom = FindChild<KaxChapterAtom>(*edition);
	while (chapterAtom && chapterAtom->GetSize() > 0) {
		AddChapterAtom(chapterAtom, priv, segment);
		chapterAtom = FindNextChild<KaxChapterAtom>(*edition, *chapterAtom);
	}
	
	EndMediaEdits(priv->chapterMedia);
	SetTrackEnabled(priv->chapterTrack, false);
	return noErr;
}

ComponentResult MkvSetupTracks(KaxTracks *tracks, MkvSegment &segment, 
							   Movie theMovie, Handle dataRef, OSType dataRefType)
{
	ComponentResult err = noErr;
	int i;
	Track firstVideoTrack = NULL;
	Track firstAudioTrack = NULL;
	Track firstSubtitleTrack = NULL;
	
	for (i = 0; i < tracks->ListSize(); i++) {
		MkvTrackRecord track = {0};
		MkvTrackPtr mkvTrack = &track;
		mkvTrack->timecodeScale = segment.timecodeScale;
		mkvTrack->lastBlock = (MkvBlock *) NewPtrClear(sizeof(MkvBlock));
		
		KaxTrackEntry *tr_entry = static_cast<KaxTrackEntry *>((*tracks)[i]);
		
		KaxTrackNumber *tr_number = FindChild<KaxTrackNumber>(*tr_entry);
		if (tr_number)
			mkvTrack->trackNumber = uint16(*tr_number);
		
		KaxTrackType *tr_type = FindChild<KaxTrackType>(*tr_entry);
		
		KaxTrackTimecodeScale *timecodeScale = FindChild<KaxTrackTimecodeScale>(*tr_entry);
		if (timecodeScale != NULL && Float64(*timecodeScale) > 1.000000001 && 
			Float64(*timecodeScale) < 0.999999999) {
			dprintf("MatroskaQT: Track %hu has a TimecodeScale of %lf, implement it\n",
					mkvTrack->trackNumber, Float64(*timecodeScale));
		}
		
		if (tr_type) {
			mkvTrack->trackType = uint8(*tr_type);
			switch (uint8(*tr_type)) {
				case track_video:
					err = MkvCreateVideoTrack(mkvTrack, tr_entry, theMovie, dataRef, dataRefType);
					if (err) 
						printf("MatroskaQT: Error creating video track %d\n", err);
					
					// set video tracks as alternates
					if (firstVideoTrack != NULL)
						SetTrackAlternate(firstVideoTrack, mkvTrack->theTrack);
					else
						firstVideoTrack = mkvTrack->theTrack;
					break;
					
				case track_audio:
					err = MkvCreateAudioTrack(mkvTrack, tr_entry, theMovie, dataRef, dataRefType);
					if (err)
						printf("MatroskaQT: Error creating audio track %d\n", err);
					
					// set audio tracks as alternates
					if (firstAudioTrack != NULL)
						SetTrackAlternate(firstAudioTrack, mkvTrack->theTrack);
					else
						firstAudioTrack = mkvTrack->theTrack;
					break;
					
				case track_subtitle:
					err = MkvCreateSubtitleTrack(mkvTrack, tr_entry, theMovie, dataRef, dataRefType);
					if (err)
						printf("MatroskaQT: Error creating subtitle track %d\n", err);
						
					// set subtitle tracks as alternates
					if (firstSubtitleTrack != NULL)
						SetTrackAlternate(firstSubtitleTrack, mkvTrack->theTrack);
					else
						firstSubtitleTrack = mkvTrack->theTrack;
					break;
					
				case track_complex:
				case track_logo:
				case track_buttons:
				case track_control:
					// not likely to be implemented soon
					break;
			}
			if (mkvTrack->sampleHdl) {
				err = QTSampleTableCreateMutable(kCFAllocatorDefault,
												 GetMovieTimeScale(theMovie),
												 NULL,
												 &mkvTrack->sampleTable);
				if (err) {
					printf("MatroskaQT: Error creating sample table %d\n", err);
					break;
				}
				err = QTSampleTableAddSampleDescription(mkvTrack->sampleTable,
														mkvTrack->sampleHdl, 
														0, 
														&mkvTrack->qtSampleDesc);
				if (err) {
					printf("MatroskaQT: Error adding sample description %d\n", err);
					break;
				}
			}
			if (mkvTrack->theMedia)
				SetMediaLanguage(mkvTrack->theMedia, GetTrackLanguage(tr_entry));
				
			if (mkvTrack->theTrack) {
				KaxTrackName *trackName = FindChild<KaxTrackName>(*tr_entry);
				
				if (trackName) {
					QTMetaDataRef trackMetaData;
					err = QTCopyTrackMetaData(mkvTrack->theTrack, &trackMetaData);
					
					if (err == noErr) {
						OSType key = 'name';
						// QuickTime differentiates between the title of a track and its name
						// so we set both
						QTMetaDataAddItem(trackMetaData,
										  kQTMetaDataStorageFormatQuickTime,
										  kQTMetaDataKeyFormatCommon,
										  (UInt8 *)&key, sizeof(key),
										  (UInt8 *)UTFstring(*trackName).GetUTF8().c_str(),
										  UTFstring(*trackName).GetUTF8().size(),
										  kQTMetaDataTypeUTF8, NULL);
						
						QTMetaDataAddItem(trackMetaData,
										  kQTMetaDataStorageFormatUserData,
										  kQTMetaDataKeyFormatUserData,
										  (UInt8 *)&key, sizeof(key),
										  (UInt8 *)UTFstring(*trackName).GetUTF8().c_str(),
										  UTFstring(*trackName).GetUTF8().size(),
										  kQTMetaDataTypeUTF8, NULL);
						
						QTMetaDataRelease(trackMetaData);
					} // if (err == noErr)
				} // if (trackName)
			} // if (mkvTrack->theTrack)
		} // if (tr_type)
		segment.tracks.push_back(track);
	}
	
	return err;
}

ComponentResult MkvCreateVideoTrack(MkvTrackPtr mkvTrack, KaxTrackEntry *tr_entry, 
									Movie theMovie, Handle dataRef, OSType dataRefType)
{
	ComponentResult err = noErr;
	ImageDescriptionHandle imgDesc;
	PixelAspectRatio **pasp = (PixelAspectRatio**)NewHandle(sizeof(PixelAspectRatio));
	Fixed width, height;
	
	KaxTrackVideo *tr_video = FindChild<KaxTrackVideo>(*tr_entry);
	if (tr_video == NULL) 
		return -1;
	**pasp = (PixelAspectRatio){1,1};

	KaxVideoDisplayWidth *disp_width = FindChild<KaxVideoDisplayWidth>(*tr_video);
	KaxVideoDisplayHeight *disp_height = FindChild<KaxVideoDisplayHeight>(*tr_video);
	KaxVideoPixelWidth *pxl_width = FindChild<KaxVideoPixelWidth>(*tr_video);
	KaxVideoPixelHeight *pxl_height = FindChild<KaxVideoPixelHeight>(*tr_video);
	
	if (disp_width != NULL && disp_height != NULL) {
		width = IntToFixed((int)uint32(*disp_width));
		height = IntToFixed((int)uint32(*disp_height));
		if (pxl_width != NULL && pxl_height != NULL) {
			**pasp = (PixelAspectRatio){uint32(*disp_width) * uint32(*pxl_height),uint32(*disp_height) * uint32(*pxl_width)};
			if ((*pasp)->hSpacing == (*pasp)->vSpacing) **pasp = (PixelAspectRatio){1,1};
		}
		
	} else if (pxl_width != NULL || pxl_height != NULL) {
		// if no display tags are set, the display dimensions are the pixel dimensions
		width = IntToFixed((int)uint32(*pxl_width));
		height = IntToFixed((int)uint32(*pxl_height));
		
	} else {
		printf("MatroskaQT: Won't create a video track with unknown dimensions.\n");
		return invalidTrack;
	}
	
	mkvTrack->theTrack = NewMovieTrack(theMovie, width, height, kNoVolume);
	if (mkvTrack->theTrack == NULL) {
		printf("MatroskaQT: Error creating video track %d.\n", GetMoviesError());
		return GetMoviesError();
	}
	
	mkvTrack->theMedia = NewTrackMedia(mkvTrack->theTrack, 'vide', GetMovieTimeScale(theMovie),
									   dataRef, dataRefType);
	if (mkvTrack->theMedia == NULL) {
		printf("MatroskaQT: Error creating video media %d.\n", GetMoviesError());
		return GetMoviesError();
	}
	
	imgDesc = (ImageDescriptionHandle) NewHandleClear(sizeof(ImageDescription));
	(*imgDesc)->idSize = sizeof(ImageDescription);
    (*imgDesc)->width = uint16(*pxl_width);
    (*imgDesc)->height = uint16(*pxl_height);
    (*imgDesc)->frameCount = 1;
	(*imgDesc)->cType = GetFourCC(tr_entry);
    (*imgDesc)->depth = 24;
    (*imgDesc)->clutID = -1;
	if ((*pasp)->hSpacing != (*pasp)->vSpacing) {
		(*pasp)->hSpacing = EndianU32_NtoB((*pasp)->hSpacing);
		(*pasp)->vSpacing = EndianU32_NtoB((*pasp)->vSpacing);
		AddImageDescriptionExtension(imgDesc,(Handle)pasp,kICMImageDescriptionPropertyID_PixelAspectRatio);
	}
	
	// this sets up anything else needed in the description for the specific codec.
	FinishSampleDescription(tr_entry, (SampleDescriptionHandle) imgDesc);
	
	mkvTrack->sampleHdl = (SampleDescriptionHandle) imgDesc;
	
	return err;
}

ComponentResult MkvCreateAudioTrack(MkvTrackPtr mkvTrack, KaxTrackEntry *tr_entry, 
									Movie theMovie, Handle dataRef, OSType dataRefType)
{
	OSStatus err = noErr;
	SoundDescriptionHandle sndDesc;
	AudioStreamBasicDescription asbd = {0};
	AudioChannelLayout acl;
    AudioChannelLayout *pacl = &acl;
    ByteCount acl_size = sizeof(acl);
	ByteCount ioSize = sizeof(asbd);
	ByteCount cookieSize = 0;
	Ptr magicCookie = NULL;
	bool aclIsFromESDS = false;
	
	mkvTrack->theTrack = NewMovieTrack(theMovie, 0, 0, kFullVolume);
	if (mkvTrack->theTrack == NULL) {
		printf("MatroskaQT: Error creating audio track %d.\n", GetMoviesError());
		return GetMoviesError();
	}
	
	mkvTrack->theMedia = NewTrackMedia(mkvTrack->theTrack, 'soun', GetMovieTimeScale(theMovie),
									   dataRef, dataRefType);
	if (mkvTrack->theMedia == NULL) {
		printf("MatroskaQT: Error creating audio media %d.\n", GetMoviesError());
		return GetMoviesError();
	}
	
	KaxTrackAudio *tr_audio = FindChild<KaxTrackAudio>(*tr_entry);
	if (tr_audio == NULL)
		return -1;
	
	KaxAudioSamplingFreq *sampleFreq = FindChild<KaxAudioSamplingFreq>(*tr_audio);
	if (sampleFreq == NULL) {
		dprintf("MatroskaQT: Audio Track is lacking a sample frequency\n");
		return -2;
	}
	
	KaxAudioChannels *numChannels = FindChild<KaxAudioChannels>(*tr_audio);
	if (numChannels == NULL) {
		dprintf("MatroskaQT: Audio Track is lacking the number of channels\n");
		return -3;
	}
	
	KaxAudioBitDepth *bitDepth = FindChild<KaxAudioBitDepth>(*tr_audio);
	if (bitDepth)
		asbd.mBitsPerChannel = uint32(*bitDepth);

	asbd.mFormatID = GetFourCC(tr_entry);
	asbd.mSampleRate = Float64(*sampleFreq);
	asbd.mChannelsPerFrame = uint32(*numChannels);
	
	MkvFinishASBD(tr_entry, &asbd);
	
	// get more info about the codec
	AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, NULL, &ioSize, &asbd);
	
	// TODO: implement correct channel layout choosing for AC3 and Vorbis
	if (!aclIsFromESDS) {
		acl.mChannelLayoutTag = GetDefaultChannelLayout(asbd);
		
		if (acl.mChannelLayoutTag == 0) {
			dprintf("MatroskaQT: No channel default for %d channels and %4.4s audio\n", 
					asbd.mChannelsPerFrame, &asbd.mFormatID);
			pacl = NULL;
			acl_size = 0;
		}
		acl.mChannelBitmap = 0;
		acl.mNumberChannelDescriptions = 0;
	}
	
	// Why does a V1 SoundDescription fail with MP3?
	err = QTSoundDescriptionCreate(&asbd, pacl, acl_size, magicCookie, cookieSize, 
								   kQTSoundDescriptionKind_Movie_AnyVersion, &sndDesc);
	if (err) {
		printf("MatroskaQT: Error creating sound description: %ld\n", err);
		return err;
	}
	
	FinishSampleDescription(tr_entry, (SampleDescriptionHandle) sndDesc);
	mkvTrack->sampleHdl = (SampleDescriptionHandle) sndDesc;
	
	return err;
}

ComponentResult MkvCreateSubtitleTrack(MkvTrackPtr mkvTrack, KaxTrackEntry *tr_entry, 
									   Movie theMovie, Handle dataRef, OSType dataRefType)
{
	ComponentResult err = noErr;
	Fixed trackWidth, trackHeight;
	Rect movieBox;
	MediaHandler mh;
	ImageDescriptionHandle imgDesc = (ImageDescriptionHandle) NewHandleClear(sizeof(ImageDescription));
	
	// we assume that a video track has already been created, so we can set the subtitle track
	// to have the same dimensions as it. Note that this doesn't work so well with multiple
	// video tracks with different dimensions; but QuickTime doesn't expect that; how should we handle them?
	GetMovieBox(theMovie, &movieBox);
	trackWidth = IntToFixed(movieBox.right - movieBox.left);
	trackHeight = IntToFixed(movieBox.bottom - movieBox.top);
	
	(*imgDesc)->idSize = sizeof(ImageDescription);
	(*imgDesc)->cType = GetFourCC(tr_entry);
	(*imgDesc)->frameCount = 1;
	(*imgDesc)->depth = 32;
	(*imgDesc)->clutID = -1;
	
	if ((*imgDesc)->cType == kSubFormatVobSub) {
		int width, height;
		
		// bitmap width & height is in the codec private in text format
		KaxCodecPrivate *idxFile = FindChild<KaxCodecPrivate>(*tr_entry);
		string idxFileStr((char *)idxFile->GetBuffer(), idxFile->GetSize());
		string::size_type loc = idxFileStr.find("size:", 0);
		if (loc == string::npos) {
			// we can't continue without bitmap width/height
			err = invalidTrack;
			goto bail;
		}
		sscanf(&idxFileStr.c_str()[loc], "size: %dx%d", &width, &height);
		(*imgDesc)->width = width;
		(*imgDesc)->height = height;
		
		mkvTrack->theTrack = NewMovieTrack(theMovie, trackWidth, trackHeight, kNoVolume);
		if (mkvTrack->theTrack == NULL) {
			err = GetMoviesError();
			goto bail;
		}
		
		mkvTrack->theMedia = NewTrackMedia(mkvTrack->theTrack, VideoMediaType, GetMovieTimeScale(theMovie), dataRef, dataRefType);
		if (mkvTrack->theMedia == NULL) {
			err = GetMoviesError();
			goto bail;
		}
		
		// finally, say that we're transparent
		mh = GetMediaHandler(mkvTrack->theMedia);
		MediaSetGraphicsMode(mh, graphicsModePreBlackAlpha, NULL);
		
	} else if ((*imgDesc)->cType == kSubFormatUTF8) {
		mkvTrack->theTrack = CreatePlaintextSubTrack(theMovie, imgDesc, GetMovieTimeScale(theMovie), dataRef, dataRefType);
		if (mkvTrack->theTrack == NULL) {
			err = GetMoviesError();
			goto bail;
		}
		
		mkvTrack->theMedia = GetTrackMedia(mkvTrack->theTrack);
		mh = GetMediaHandler(mkvTrack->theMedia);
		MediaSetGraphicsMode(mh, graphicsModePreBlackAlpha, NULL);
		
	} else {
		err = invalidTrack;
		goto bail;
	}
	
	// this sets up anything else needed in the description for the specific codec.
	FinishSampleDescription(tr_entry, (SampleDescriptionHandle) imgDesc);
	
	// and save our sample description
	mkvTrack->sampleHdl = (SampleDescriptionHandle) imgDesc;

bail:
	if (err) DisposeHandle((Handle) imgDesc);
	return err;
}

int GetAACProfile(KaxTrackEntry *tr_entry) {
	KaxCodecID *tr_codec = FindChild<KaxCodecID>(*tr_entry);
	if (tr_codec == NULL)
		return 0;
#if 0
	string codecString(*tr_codec);
	if (codecString.compare(0, strlen(MKV_A_AAC), string(MKV_A_AAC)) != 0)
		return 0;
	
	// I just found these flags in CoreAudioTypes.h
	// I wish Apple decoded SBR
	// the LC part of HE-AAC sounds awful
	if (codecString.compare(codecString.size() - strlen(AAC_MAIN), 
							strlen(AAC_MAIN), string(AAC_MAIN)) == 0) {
		return kMPEG4Object_AAC_Main;
		
	} else if (codecString.compare(codecString.size() - strlen(AAC_LC), 
								   strlen(AAC_LC), string(AAC_LC)) == 0) {
		return kMPEG4Object_AAC_LC;
		
	} else if (codecString.compare(codecString.size() - strlen(AAC_SSR), 
								   strlen(AAC_SSR), string(AAC_SSR)) == 0) {
		return kMPEG4Object_AAC_SSR;
		
	} else if (codecString.compare(codecString.size() - strlen(AAC_LTP), 
								   strlen(AAC_LTP), string(AAC_LTP)) == 0) {
		return kMPEG4Object_AAC_LTP;
		
	} else if (codecString.compare(codecString.size() - strlen(AAC_SBR), 
								   strlen(AAC_SBR), string(AAC_SBR)) == 0) {
		return kMPEG4Object_AAC_SBR;
	}
#endif
	return 0;
}

UInt32 GetDefaultChannelLayout(AudioStreamBasicDescription &asbd) {
	switch (asbd.mChannelsPerFrame) {
		case 1:
			return kAudioChannelLayoutTag_Mono;
			break;
			
		case 2:
			return kAudioChannelLayoutTag_Stereo;
			break;
			
		case 3:
			// AAC defaults come from the defined non-custom layouts
			switch (asbd.mFormatID) {
				case kAudioFormatMPEG4AAC:
					return kAudioChannelLayoutTag_MPEG_3_0_B;
					break;
			}
			break;
			
		case 4:
			switch (asbd.mFormatID) {
				case kAudioFormatMPEG4AAC:
					return kAudioChannelLayoutTag_AAC_4_0;
					break;
			}
			break;
			
		case 5:
			switch (asbd.mFormatID) {
				case kAudioFormatMPEG4AAC:
					return kAudioChannelLayoutTag_AAC_5_0;
					break;
					
				case kAudioFormatAC3:
					// I've found one AC3 track that uses this
					return kAudioChannelLayoutTag_MPEG_5_0_A;
					break;
			}
			break;
			
		case 6:
			switch (asbd.mFormatID) {
				case kAudioFormatMPEG4AAC:
					return kAudioChannelLayoutTag_AAC_5_1;
					break;
					
				case kAudioFormatXiphVorbis:
					// the one vorbis track I've seen is L C R Ls Rs LFE
					return kAudioChannelLayoutTag_MPEG_5_1_C;
					break;
					
					// and the AC3 I've seen is L R C LFE Ls Rs
				case kAudioFormatAC3:
					// same with the one FLAC track I have
				case kAudioFormatXiphFLAC:
					return kAudioChannelLayoutTag_MPEG_5_1_A;
					break;
			}
			break;
			
		case 7:
			switch (asbd.mFormatID) {
				case kAudioFormatMPEG4AAC:
					// nothing at wiki.multimedia.cx about 7 channels
					return kAudioChannelLayoutTag_AAC_6_1;
					break;
			}
			break;
			
		case 8:
			switch (asbd.mFormatID) {
				case kAudioFormatMPEG4AAC:
					return kAudioChannelLayoutTag_AAC_7_1;
					break;
			}
			break;
	}
	return 0;
}

void FinishSampleDescription(KaxTrackEntry *tr_entry, SampleDescriptionHandle desc)
{
	KaxCodecID *tr_codec = FindChild<KaxCodecID>(*tr_entry);
	if (tr_codec == NULL)
		return;
	
	string codecString(*tr_codec);
	
	if (codecString == MKV_V_MS) {
		// BITMAPINFOHEADER is stored in the private data, and some codecs (WMV)
		// need it to decode
		ImageDescriptionHandle imgDesc = (ImageDescriptionHandle) desc;
		
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL)
			return;
		
		Handle imgDescExt = NewHandle(codecPrivate->GetSize());
		memcpy(*imgDescExt, codecPrivate->GetBuffer(), codecPrivate->GetSize());
		
		AddImageDescriptionExtension(imgDesc, imgDescExt, 'strf');
		
	} else if (codecString == MKV_V_QT) {
		// This seems to work fine, but there's something it's missing to get the 
		// image description to match perfectly (last 2 bytes are different)
		// Figure it out later...
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL || codecPrivate->GetSize() < sizeof(ImageDescription)) {
			printf("MatroskaQT: QuickTime track %hu doesn't have needed stsd data\n", 
				   uint16(tr_entry->TrackNumber()));
			return;
		}
		
		ImageDescriptionHandle imgDesc = (ImageDescriptionHandle) desc;
		SetHandleSize((Handle) imgDesc, codecPrivate->GetSize());
		memcpy(&(*imgDesc)->cType, codecPrivate->GetBuffer(), codecPrivate->GetSize());
		// it's stored in big endian, so flip endian to native
		// I think we have to do this, need to check on Intel without them
		(*imgDesc)->idSize = codecPrivate->GetSize();
		(*imgDesc)->cType = EndianU32_BtoN((*imgDesc)->cType);
		(*imgDesc)->resvd1 = EndianS32_BtoN((*imgDesc)->resvd1);
		(*imgDesc)->resvd2 = EndianS16_BtoN((*imgDesc)->resvd2);
		(*imgDesc)->dataRefIndex = EndianS16_BtoN((*imgDesc)->dataRefIndex);
		(*imgDesc)->version = EndianS16_BtoN((*imgDesc)->version);
		(*imgDesc)->revisionLevel = EndianS16_BtoN((*imgDesc)->revisionLevel);
		(*imgDesc)->vendor = EndianS32_BtoN((*imgDesc)->vendor);
		(*imgDesc)->temporalQuality = EndianU32_BtoN((*imgDesc)->temporalQuality);
		(*imgDesc)->spatialQuality = EndianU32_BtoN((*imgDesc)->spatialQuality);
		(*imgDesc)->width = EndianS16_BtoN((*imgDesc)->width);
		(*imgDesc)->height = EndianS16_BtoN((*imgDesc)->height);
		(*imgDesc)->vRes = EndianS32_BtoN((*imgDesc)->vRes);
		(*imgDesc)->hRes = EndianS32_BtoN((*imgDesc)->hRes);
		(*imgDesc)->dataSize = EndianS32_BtoN((*imgDesc)->dataSize);
		(*imgDesc)->frameCount = EndianS16_BtoN((*imgDesc)->frameCount);
		(*imgDesc)->depth = EndianS16_BtoN((*imgDesc)->depth);
		(*imgDesc)->clutID = EndianS16_BtoN((*imgDesc)->clutID);
		
#if 0
	} else if (codecString == MKV_V_THEORA) {
		// we need 3 setup packets for the image description extension, but
		// the sample files I muxed have only 2 packets in the Codec Private element.
		// Despite the Matroska specs agreeing with me about the 3 packets.
		// Will figure out later.
	} else if (codecString.compare(0, strlen(MKV_A_AAC), string(MKV_A_AAC)) == 0) {
		// esds extension gives us the channel mapping for AAC
		SoundDescriptionHandle sndDesc = (SoundDescriptionHandle) desc;
		
		KaxCodecPrivate *codecPrivate = FindChild<KaxCodecPrivate>(*tr_entry);
		if (codecPrivate == NULL)
			return;
		
		Handle sndDescExt = NewHandle(codecPrivate->GetSize());
		memcpy(*sndDescExt, codecPrivate->GetBuffer(), codecPrivate->GetSize());
		
		AddSoundDescriptionExtension(sndDesc, sndDescExt, 'esds');
		
		
		
		DisposeHandle((Handle) sndDescExt);
#endif
	} else {
		MkvFinishSampleDescription(tr_entry, desc, kToSampleDescription);
	}
}
