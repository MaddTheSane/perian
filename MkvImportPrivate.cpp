/*
 *  MkvImportPrivate.cpp
 *
 *    MkvImportPrivate.cpp - C++ code for interfacing with libmatroska to import.
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

#include "MkvImportPrivate.h"
#include "DataHandlerCallback.h"
#include "MkvMovieSetup.h"
#include <QuickTime/QuickTime.h>

#include <ebml/EbmlHead.h>
#include <ebml/EbmlSubHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlVersion.h>
#include <ebml/EbmlVoid.h>
#include <ebml/StdIOCallback.h>

#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxClusterData.h>
#include <matroska/KaxInfo.h>
#include <matroska/KaxInfoData.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxTrackEntryData.h>
#include <matroska/KaxTrackAudio.h>
#include <matroska/KaxTrackVideo.h>
#include <string>

using namespace std;
using namespace libmatroska;

ComponentResult MkvImportBlock(MkvBlock *block, MkvTrackPtr mkvTrack, bool addToMedia, 
							   TimeValue *maxLoadedTime);


ComponentResult OpenFile(MatroskaImportGlobals globals)
{
	EbmlElement *el_l0;
	int upperLevel = 0;
	KaxPrivate *priv = new KaxPrivate;
	memset(priv, 0, sizeof(KaxPrivate));
	globals->kaxPrivate = priv;
	
	if (globals->filename) {
		// StdIOCallback is over twice as fast as my DataHandlerCallback; 
		// this discrepancy needs to be addressed eventually
		CFIndex bufferSize = CFStringGetMaximumSizeOfFileSystemRepresentation(globals->filename);
		Ptr buffer = NewPtr(bufferSize);
		CFStringGetFileSystemRepresentation(globals->filename, buffer, bufferSize);
		priv->ioHandler = new StdIOCallback(buffer, MODE_READ);
	} else
		priv->ioHandler = new DataHandlerCallback(globals->dataRef, globals->dataRefType, MODE_READ);
	
	if (priv->ioHandler == NULL)
		return ioErr;
	
	priv->aStream = new EbmlStream(*priv->ioHandler);
	
	el_l0 = priv->aStream->FindNextID(EbmlHead::ClassInfos, 0xFFFFFFFFL);
	if (el_l0 == NULL) {
		printf("MatroskaQT - Missing EBML Head\n");
		return unknownFormatErr;
	}
	
	el_l0->Read(*priv->aStream, EbmlHead::ClassInfos.Context, upperLevel, el_l0, true);
	EbmlHead *head = static_cast<EbmlHead *>(el_l0);
	
	EDocType docType = GetChild<EDocType>(*head);
	if (string(docType) != "matroska") {
		printf("MatroskaQT - Not a matroska file\n");
		delete el_l0;
		return unknownFormatErr;
	}
	
	EDocTypeReadVersion readVersion = GetChild<EDocTypeReadVersion>(*head);
	if (UInt64(readVersion) > 2) {
		printf("MatroskaQT - File too new to be read\n");
		delete el_l0;
		return unknownFormatErr;
	}
	
	delete el_l0;
	return noErr;
}

void DisposeKaxPrivate(KaxPrivate *priv)
{
	if (priv) {
		if (priv->ioHandler)
			delete priv->ioHandler;
		if (priv->aStream)
			delete priv->aStream;
		
		for (int i = 0; i < priv->segments.size(); i++) {
			if (priv->segments[i].segment)
				delete priv->segments[i].segment;
			
			for (int j = 0; j < priv->segments[i].tracks.size(); j++) {
				if (priv->segments[i].tracks[j].sampleHdl)
					DisposeHandle((Handle) priv->segments[i].tracks[j].sampleHdl);
				
				if (priv->segments[i].tracks[j].sampleTable)
					QTSampleTableRelease(priv->segments[i].tracks[j].sampleTable);

				if (priv->segments[i].tracks[j].lastBlock)
					DisposePtr((Ptr) priv->segments[i].tracks[j].lastBlock);
			}
		}
		delete priv;
	}
}

// this cycles through the children elements of a parent and returns the first child matching 
// the given callbacks. It relies on the position in the file already being set, and reads
// the returned element
EbmlElement *GetSubElement(EbmlStream &aStream, const EbmlCallbacks &callbacks, 
						   EbmlElement *parent) {
	EbmlElement *child;
	int upperLevel = 0;
	child = aStream.FindNextElement(parent->Generic().Context, upperLevel, 0xFFFFFFFFL, true);
				
	while (child != NULL) {
		if (EbmlId(*child) == callbacks.GlobalId) {
			child->Read(aStream, callbacks.Context, upperLevel, child, true);
			return child;
		} else 
			child->SkipData(aStream, child->Generic().Context);
		
		if (upperLevel > 0) {
			if (upperLevel > 1)
				break;
			delete child;
			child = NULL;
			continue;
		} else if (upperLevel < -1)
			break;
		upperLevel = 0;
		
		child->SkipData(aStream, child->Generic().Context);
		delete child;
		child = aStream.FindNextElement(parent->Generic().Context, upperLevel, 0xFFFFFFFFL, true);
	}
	return NULL;
}

// this figures out the offset and size of each segment in the file
// it does not read any data contained within the segments
void FindSegments(KaxPrivate *priv)
{
	EbmlElement *el_l0 = priv->aStream->FindNextID(KaxSegment::ClassInfos, 0xFFFFFFFFL);
	
	while (el_l0 != NULL) {
		if (!el_l0->IsDummy()) {
			MkvSegment seg = {0};
			seg.segment = static_cast<KaxSegment *>(el_l0);
			priv->segments.push_back(seg);
		}
		el_l0 = priv->aStream->FindNextID(KaxSegment::ClassInfos, 0xFFFFFFFFL);
	}
}

void ReadSeekHeads(KaxPrivate *priv) 
{
	// different seek heads for each segment
	for (int i = 0; i < priv->segments.size(); i++) {
		MkvSegment *seg = &priv->segments[i];
		
		// seek to the beginning of the segment and get the seek head
		priv->aStream->I_O().setFilePointer(seg->segment->GetDataStart());
		KaxSeekHead *seekHead = (KaxSeekHead *) GetSubElement(*priv->aStream, 
															  KaxSeekHead::ClassInfos, 
															  seg->segment);
		
		// there are files that have two seek heads
		// the first one contains every level one element except clusters, including the 
		// location of the second one, which contains all clusters
		// we don't care too much about clusters, so for now any other seek heads other than 
		// the first won't be read
		// hopefully, files will continue to be sane in this way...
		if (seekHead) {
			KaxSeek *seek = FindChild<KaxSeek>(*seekHead);
			for (int j = seekHead->ListSize(); j > 0; j--) {
				KaxSeekID *seekid = FindChild<KaxSeekID>(*seek);
				
				MkvSeekEntry entry;
				entry.offset = seek->Location();
				memcpy(entry.ebmlID, seekid->GetBuffer(), seekid->GetSize());
				entry.length = seekid->GetSize();
				
				seg->seekEntries.push_back(entry);
				
				seek = FindNextChild<KaxSeek>(*seekHead, *seek);
			}
		}
	}
}

// this function finds and reads the first of the given level one elements in a given segment, 
// using the seek head if available
EbmlElement *GetLevelOneElement(EbmlStream &aStream, const EbmlCallbacks &callbacks, 
								MkvSegment &segment)
{
	for (int i = 0; i < segment.seekEntries.size(); i++) {
		if (EbmlId(segment.seekEntries[i].ebmlID, segment.seekEntries[i].length) == 
				callbacks.GlobalId) {
			aStream.I_O().setFilePointer(segment.seekEntries[i].offset + 
										 segment.segment->GetDataStart());
			return GetSubElement(aStream, callbacks, segment.segment);
		}
	}
	// assume that if it isn't listed in the seek head, it isn't in the file
	// otherwise performance goes down too much when looking for optional elements like Chapters
	return NULL;
}

void ImportCluster(KaxCluster *cluster, MkvSegment &segment, bool addMediaToTrack, 
				   TimeValue *maxLoadedTime) {
	KaxClusterTimecode *clusterTime = FindChild<KaxClusterTimecode>(*cluster);
	cluster->InitTimecode(uint64(*clusterTime), segment.timecodeScale);
	
	KaxBlockGroup *blkGroup = FindChild<KaxBlockGroup>(*cluster);
	while (blkGroup && blkGroup->GetSize() > 0) {
		KaxBlock *block = FindChild<KaxBlock>(*blkGroup);
		block->SetParent(*cluster);
		
		MkvTrackPtr mkvTrack = NULL;
		for (int i = 0; i < segment.tracks.size(); i++) {
			if (segment.tracks[i].trackNumber == block->TrackNum()) {
				mkvTrack = &segment.tracks[i];
				break;
			}
		}
		if (mkvTrack == NULL) {
			dprintf("MatroskaQT: Block found without a track (num = %hu), discarding\n", 
					block->TrackNum());
		} else {
			MkvBlock *lastBlock = mkvTrack->lastBlock;
			
			// if we have a block waiting to be imported, calculate the duration and import it
			if (lastBlock->numFrames > 0) {
				if (lastBlock->duration == 0 || mkvTrack->trackType != track_subtitle) {
					// always calcualate the duration from the difference in timecodes between blocks
					// except for subtitle tracks, where the BlockDuration element has meaning
					lastBlock->duration = block->GlobalTimecode() / 
						mkvTrack->timecodeScale - lastBlock->timecode;
				}
				
				MkvImportBlock(lastBlock, mkvTrack, addMediaToTrack, maxLoadedTime);
			}
			
			// then set the fields in the last block structure
			lastBlock->numFrames = block->NumberFrames();
			// if no other blocks are referenced, assume keyframe
			lastBlock->sampleFlags = blkGroup->ReferenceCount() > 0 ? mediaSampleNotSync : 0;
			lastBlock->timecode = block->GlobalTimecode() / segment.timecodeScale;
			
			lastBlock->dataOffset.clear();
			lastBlock->dataSize.clear();
			
			// not reading the block makes for faster parsing, but also doesn't set the correct
			// data start position if the block's laced. Adjust for that here (this seems to work
			// for all 3 lacing types, though I'm not 100% sure why)
			short offset = 0;
			switch (block->GetLacingType()) {
				case LACING_XIPH:
					offset = 2 - lastBlock->numFrames;
					break;
				case LACING_FIXED:
				case LACING_EBML:
					offset = 1;
					break;
			}
			for (int j = 0; j < lastBlock->numFrames; j++) {
				lastBlock->dataOffset.push_back(block->GetDataPosition(j) + offset);
				lastBlock->dataSize.push_back(block->GetFrameSize(j));
			}
			
			KaxBlockDuration *blockDuration = FindChild<KaxBlockDuration>(*blkGroup);
			// set the duration if we have it (we need to check to make sure it isn't
			// greater than the difference in timecodes to the next block before importing it)
			if (blockDuration != NULL)
				lastBlock->duration = uint32(*blockDuration);
			else
				lastBlock->duration = 0;
			
			//if (mkvTrack->trackType == track_video)
			//	printf("Frame at %lld, duration %u\n", lastBlock->timecode, lastBlock->duration);
		}
		blkGroup = FindNextChild<KaxBlockGroup>(*cluster, *blkGroup);
	}
}

ComponentResult ImportMkvIdle(MatroskaImportGlobals globals, long *outFlags, int numBlocks)
{
	KaxPrivate *priv = globals->kaxPrivate;
	EbmlElement *el_l1;
	int upperLevel = 0;
	int i = 0;
	KaxCluster *cluster = NULL;
	
	// if we already are in the middle of importing a cluster, use it
	if (true || numBlocks == kFullCluster && priv->currentCluster == NULL) {
		cluster = (KaxCluster *) GetSubElement(*priv->aStream, KaxCluster::ClassInfos, 
												priv->segments[i].segment);
		
		if (cluster == NULL || cluster->GetSize() == 0) {
			*outFlags |= movieImportResultComplete;
			return noErr;
		}
		
		cluster->Read(*priv->aStream, KaxCluster::ClassInfos.Context, upperLevel, 
					  el_l1, true, SCOPE_PARTIAL_DATA);
		cluster->SetParent(*priv->segments[i].segment);
		ImportCluster(cluster, priv->segments[i], true, &globals->maxLoadedTime);
		
	} else {
#if 0
		if (priv->currentCluster == NULL) {
			priv->currentCluster = (KaxCluster *) GetSubElement(*priv->aStream, 
																KaxCluster::ClassInfos, 
																priv->segments[i].segment);
			if (priv->currentCluster == NULL || priv->currentCluster->GetSize() == 0) {
				*outFlags |= movieImportResultComplete;
				return noErr;
			}
		}
		cluster = priv->currentCluster;
		
		KaxBlockGroup *blkGroup = GetSubElement(*priv->aStrem, KaxBlockGroup::ClassInfos, cluster);
		for (int j = 0; j < numBlocks; j++) {
			
			KaxBlock *block = FindChild<KaxBlock>(*blkGroup);
			block->SetParent(*cluster);
			
			MkvTrackPtr mkvTrack = NULL;
			for (int i = 0; i < segment.tracks.size(); i++) {
				if (segment.tracks[i].trackNumber == block->TrackNum()) {
					mkvTrack = &segment.tracks[i];
					break;
				}
			}
			if (mkvTrack == NULL) {
				dprintf("MatroskaQT: Block found without a track (num = %hu), discarding\n", 
						block->TrackNum());
			} else {
				MkvBlock *lastBlock = mkvTrack->lastBlock;
				
				// if we have a block waiting to be imported, 
				// calculate the duration and import it
				if (lastBlock->numFrames > 0) {
					if (lastBlock->duration == 0 || lastBlock->duration > 
						(block->GlobalTimecode() / mkvTrack->timecodeScale - lastBlock->timecode)) {
						// if the BlockDuration element isn't set, the duration is
						// the difference in timecodes between a block and the next
						// also use the difference in timecodes if the BlockDuration
						// is greater than the difference
						lastBlock->duration = block->GlobalTimecode() / 
						mkvTrack->timecodeScale - lastBlock->timecode;
					}
					
					MkvImportBlock(lastBlock, mkvTrack, addMediaToTrack, maxLoadedTime);
				}
				
				// then set the fields in the last block structure
				lastBlock->numFrames = block->NumberFrames();
				// if no other blocks are referenced, assume keyframe
				lastBlock->sampleFlags = blkGroup->ReferenceCount() > 0 ? mediaSampleNotSync : 0;
				lastBlock->timecode = block->GlobalTimecode() / segment.timecodeScale;
				
				lastBlock->dataOffset.clear();
				lastBlock->dataSize.clear();
				
				// not reading the block makes for faster parsing, but also doesn't set the correct
				// data start position if the block's laced. Adjust for that here (this seems to work
				// for all 3 lacing types, though I'm not 100% sure why)
				short offset = 0;
				switch (block->GetLacingType()) {
					case LACING_XIPH:
						offset = 2 - lastBlock->numFrames;
						break;
					case LACING_FIXED:
					case LACING_EBML:
						offset = 1;
						break;
				}
				for (int j = 0; j < lastBlock->numFrames; j++) {
					lastBlock->dataOffset.push_back(block->GetDataPosition(j) + offset);
					lastBlock->dataSize.push_back(block->GetFrameSize(j));
				}
				
				KaxBlockDuration *blockDuration = FindChild<KaxBlockDuration>(*blkGroup);
				// set the duration if we have it (we need to check to make sure it isn't
				// greater than the difference in timecodes to the next block before importing it)
				if (blockDuration != NULL)
					lastBlock->duration = uint32(*blockDuration);
				else
					lastBlock->duration = 0;
				
				//if (mkvTrack->trackType == track_video)
				//	printf("Frame at %lld, duration %u\n", lastBlock->timecode, lastBlock->duration);
			}
			blkGroup = FindNextChild<KaxBlockGroup>(*cluster, *blkGroup);
		}
#endif
	}
	
	return noErr;
}

void AddBaseTrack(MatroskaImportGlobals globals, TimeValue duration)
{
	SampleDescriptionHandle sampleDesc;
	sampleDesc = (SampleDescriptionHandle) NewHandleClear(sizeof(SampleDescription));
	globals->baseTrack = NewMovieTrack(globals->theMovie, 0, 0, kNoVolume);
	if (globals->baseTrack) {
		Media baseMedia = NewTrackMedia(globals->baseTrack, 'gnrc', 
										GetMovieTimeScale(globals->theMovie),
										globals->dataRef, globals->dataRefType);
		if (baseMedia) {
			AddMediaSampleReference(baseMedia, 0, 1, duration, sampleDesc, 1, 0, NULL);
			InsertMediaIntoTrack(globals->baseTrack, 0, 0, GetMediaDuration(baseMedia), fixed1);
		} else {
			DisposeMovieTrack(globals->baseTrack);
			globals->baseTrack = NULL;
		}
	}
	DisposeHandle((Handle) sampleDesc);
}

ComponentResult ImportMkvRef(MatroskaImportGlobals globals)
{
	ComponentResult err = noErr;
	
	err = OpenFile(globals);
	if (err)
		return err;
	
	KaxPrivate *priv = globals->kaxPrivate;
	priv->theMovie = globals->theMovie;
	FindSegments(priv);
	if (globals->useIdle)
		// read seek head for locations of other level 1 elements
		ReadSeekHeads(priv);
	
	TimeValue segmentOffset = 0;
	
	for (int i = 0; i < priv->segments.size(); i++) {
		KaxSegment *segment = priv->segments[i].segment;
		KaxInfo *info = NULL;
		KaxChapters *chapters = NULL;
		KaxTracks *tracks = NULL;
		
		if (globals->useIdle) {
			info = (KaxInfo *) GetLevelOneElement(*priv->aStream, KaxInfo::ClassInfos, 
												  priv->segments[i]);
			chapters = (KaxChapters *) GetLevelOneElement(*priv->aStream, KaxChapters::ClassInfos, 
														  priv->segments[i]);
			tracks = (KaxTracks *) GetLevelOneElement(*priv->aStream, KaxTracks::ClassInfos, 
													  priv->segments[i]);
			
		} else {
			// read the entire structure of the segment in one go
			int upperLevel = 0;
			EbmlElement *el_l0 = static_cast<EbmlElement *>(segment);
			el_l0->Read(*priv->aStream, KaxSegment::ClassInfos.Context, upperLevel, 
						el_l0, true, SCOPE_PARTIAL_DATA);
			info = FindChild<KaxInfo>(*segment);
			chapters = FindChild<KaxChapters>(*segment);
			tracks = FindChild<KaxTracks>(*segment);
		}
		
		ReadSegmentInfo(info, &priv->segments[i], globals->theMovie);
		SetMovieTimeScale(globals->theMovie, S64Divide(S64Set(1000000000L), 
													   priv->segments[i].timecodeScale, 
													   NULL));
		
		MkvSetupTracks(tracks, priv->segments[i], globals->theMovie, 
					   globals->dataRef, globals->dataRefType);
		if (chapters)
			SetupChapters(chapters, priv, &priv->segments[i]);
		
		if (!globals->useIdle) {
			// import all blocks now
			KaxCluster *cluster = FindChild<KaxCluster>(*segment);
			cluster->SetParent(*segment);
			while (cluster && cluster->GetSize() > 0) {
				ImportCluster(cluster, priv->segments[i], false, &globals->maxLoadedTime);
				cluster = FindNextChild<KaxCluster>(*segment, *cluster);
				cluster->SetParent(*segment);
			}
			
			// and insert the samples into the track
			for (int j = 0; j < priv->segments[i].tracks.size(); j++) {
				MkvTrackPtr mkvTrack = &priv->segments[i].tracks[j];
				
				if (mkvTrack->theTrack) {
					if (mkvTrack->trackType == track_video) {
						err = AddSampleTableToMedia(mkvTrack->theMedia,
													mkvTrack->sampleTable,
													1,
									QTSampleTableGetNumberOfSamples(mkvTrack->sampleTable),
													NULL);
						if (err) 
							printf("MatroskaQT: Error adding sample table to media %d\n", err);
					}
					err = InsertMediaIntoTrack(mkvTrack->theTrack,
											   segmentOffset,
											   0,
											   GetMediaDuration(mkvTrack->theMedia),
											   fixed1);
					if (err)
						printf("MatroskaQT: Error inserting media into track %d\n", err);
				}
			}
			segmentOffset += TimeValue(priv->segments[i].duration);
		} else {
			// add a base track, then a small fake sample as the duration of the movie
			AddBaseTrack(globals, TimeValue(priv->segments[i].duration));
			priv->aStream->I_O().setFilePointer(priv->segments[i].segment->GetDataStart());
		}
		
		if (priv->chapterTrack) {
			for (int j = 0; j < priv->segments[i].tracks.size(); j++)
				AddTrackReference(priv->segments[i].tracks[j].theTrack, priv->chapterTrack, 
								  kTrackReferenceChapterList, NULL);
		}
	}
	
	return noErr;
}
	
ComponentResult MkvImportBlock(MkvBlock *block, MkvTrackPtr mkvTrack, bool addToTrack,
							   TimeValue *maxLoadedTime)
{
	ComponentResult err = noErr;
	int i;
	SInt64 sampleNum;
	SInt64 *sampleNumPtr = &sampleNum;
	
	SampleReference64Record sample;
	sample.numberOfSamples = 1;
	sample.sampleFlags = block->sampleFlags;
	TimeValue sampleTime;
	TimeValue *sampleTimePtr = &sampleTime;
	
	// we need a sample table, and if we don't have one, it's probably
	// because it's a track type we don't yet support.
	if (mkvTrack->sampleTable == NULL)
		return noErr;
	
	// if the duration doesn't divide evenly, add 1 to each of the
	// latter frames' duration to make it up
	// is this correct to do?
	int remainder = block->duration % block->numFrames;
	SInt64 divDuration = block->duration / block->numFrames;
	
	for (i = 0; i < block->numFrames; i++) {
		// sample tables are slow compared to adding a sample reference
		// so only use them for video tracks
		if (mkvTrack->trackType == track_video) {
			err = QTSampleTableAddSampleReferences(mkvTrack->sampleTable,
												   block->dataOffset.at(i),
												   block->dataSize.at(i),
												   divDuration + (block->numFrames - i <= 
																  remainder ? 1 : 0),
												   0,
												   1,
												   block->sampleFlags,
												   mkvTrack->qtSampleDesc,
												   sampleNumPtr);
			if (err) {
				printf("MatroskaQT: Error adding frame to sample table %d\n", err);
				return err;
			}
			// this is so that sampleNum is the sample number of the first sample added, not the last
			sampleNumPtr = NULL;
		} else {
			sample.dataOffset = SInt64ToWide(block->dataOffset.at(i));
			sample.dataSize = block->dataSize.at(i);
			sample.durationPerSample = divDuration + (block->numFrames - i <= remainder ? 1 : 0);
			
			
			err = AddMediaSampleReferences64(mkvTrack->theMedia,
											 mkvTrack->sampleHdl,
											 1,
											 &sample,
											 sampleTimePtr);
			if (err) {
				printf("MatroskaQT: Error adding sample to media (track %hu) %d\n", 
					   mkvTrack->trackNumber, err);
				return err;
			}
			sampleTimePtr = NULL;
		}
	}
	
	if (addToTrack) {
		if (mkvTrack->trackType == track_video) {
			TimeValue64 sampleTime64;
			err = AddSampleTableToMedia(mkvTrack->theMedia,
										mkvTrack->sampleTable,
										sampleNum,
										block->numFrames,
										&sampleTime64);
			if (err) {
				printf("MatroskaQT: Error adding sample table to media %d\n", err);
				return err;
			}
			sampleTime = sampleTime64;
		}

		err = InsertMediaIntoTrack(mkvTrack->theTrack,
								   block->timecode,
								   sampleTime,
								   block->duration,
								   fixed1);
		if (err)
			printf("MatroskaQT: Error inserting media into track %hu at %llu %d\n", 
				   mkvTrack->trackNumber, block->timecode, err);
		
		if (block->timecode > *maxLoadedTime)
			*maxLoadedTime = block->timecode;
	}
	
	//if (mkvTrack->theMedia)
	//	printf("%hu - %ls\n", mkvTrack->trackNumber, GetMediaDuration(mkvTrack->theMedia));
	
	return err;
}
