/*
 *  MatroskaImport.h
 *
 *    MatroskaImport.h - QuickTime importer interface for opening a Matroska file.
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

#ifndef __MATROSKAIMPORT_H__
#define __MATROSKAIMPORT_H__

#include <vector>
#include <QuickTime/QuickTime.h>
#include "DataHandlerCallback.h"

#include <ebml/EbmlStream.h>
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxInfo.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxChapters.h>
#include <matroska/KaxBlock.h>
#include <matroska/KaxAttachments.h>

using namespace libmatroska;
using namespace std;


struct MatroskaFrame {
	TimeValue64		timecode;
//	TimeValue64		duration;
	UInt32			duration;		// hack so that H.264 tracks kinda work
	SInt64			offset;
	SInt64			size;
	short			flags;
};

class MatroskaTrack {
public:
	MatroskaTrack();
	
	// retains the sampleTable if it exists, allocates a new SampleDescriptionHandle
	// and copies it if necessary
	MatroskaTrack(const MatroskaTrack &copy);
	
	// releases the sampleTable and sample description
	~MatroskaTrack();
	
	// adds the all the frames in the block group to the sample table if it exists, 
	// the media otherwise. If this track type is subtitle, also inserts it into the track.
	void AddBlock(KaxBlockGroup &blockGroup);
	
	// this adds all the samples added through AddBlock() to the track that aren't already
	// added, e.g. from a previous call to AddSamplesToTrack()
	void AddSamplesToTrack();
	
	UInt16					number;
	UInt8					type;
	Track					theTrack;
	Media					theMedia;
	SampleDescriptionHandle desc;
	QTMutableSampleTableRef sampleTable;
	QTSampleDescriptionID	qtSampleDesc;
	SInt64					timecodeScale;
	TimeValue64				maxLoadedTime;
	
private:
	// adds an individual frame from a block group into the sample table if it exists,
	// the media otherwise, and into the track if the track is a subtitle track.
	void AddFrame(MatroskaFrame &frame);
	
	// Since the duration in Matroska files is generally rather unreliable, rely only on
	// the difference in timestamps between two frames. Thus, AddBlock() buffers frames
	// from one block group until the next block group is found to set the duration of the
	// previous ones to be the difference in timestamps.
	vector<MatroskaFrame>	lastFrames;
	bool					seenFirstFrame;
	
	// this is the first sample number (if sample table) or sample time that we haven't 
	// yet added to the media/track. A value of -1 means the value is invalid/unset.
	SInt64					firstSample;
	SInt64					amountToAdd;	// num samples if sampleTable, duration otherwise
};


class MatroskaImport {
public:
	// public interface functions, simply called from the C equivalents defined 
	// by ComponentDispatchHelper.c and implemented in MatroskaImport.cpp
	
	// MatroskaImportOpen()
	MatroskaImport(ComponentInstance self);
	
	// MatrosakImportClose()
	~MatroskaImport();
	
	// MatroskaImportDataRef()
	ComponentResult ImportDataRef(Handle dataRef, OSType dataRefType, Movie theMovie,
								  Track targetTrack, Track *usedTrack,
								  TimeValue atTime, TimeValue *durationAdded,
								  long inFlags, long *outFlags);
	
	// MatroskaImportValidateDataRef()
	ComponentResult ValidateDataRef(Handle dataRef, OSType dataRefType, UInt8 *valid);
	
	// MatroskaImportIdle()
	ComponentResult Idle(long inFlags, long *outFlags);
	
	// MatroskaImportSetIdleManager()
	ComponentResult SetIdleManager(IdleManager im);
	
	// MatroskaImportGetMaxLoadedTime()
	ComponentResult GetMaxLoadedTime(TimeValue *time);
	
	// MatroskaImportGetLoadState()
	ComponentResult GetLoadState(long *importerLoadState);
	
	// we need to get our component instance to get our mime type resource
	ComponentInstance Component() { return self; }
	
private:
	// open the ioHandler and EBML stream, and read the EBML head to verify it's a matroska file
	// returns true if it's a valid file and false otherwise
	bool OpenFile();
	
	// create all the tracks and their sample descriptions as described by the file header
	// also create chapters if any. Leaves el_l1 pointing to the first cluster, unread.
	void SetupMovie();
	
	// This finds the next level 1 element and both replaces the el_l1 variable with it and
	// returns it. Does not read the data.
	EbmlElement *NextLevel1Element();
	
	// sets up timescale & file name metadata
	void ReadSegmentInfo(KaxInfo &segmentInfo);
	
	// sets up all the movie tracks and media
	void ReadTracks(KaxTracks &trackEntries);
	
	// Creates a chapter track, but doesn't actually add the chapter reference to the other
	// enabled tracks in case some weird file has this element before the Tracks element
	void ReadChapters(KaxChapters &chapterEntries);
	
	// Activates any attached fonts, ignores other attachment types for now
	void ReadAttachments(KaxAttachments &attachments);
	
	// These three are called from ReadTracks to set up a track of the specific type, 
	// modifying the MatroskaTrack structure to reflect the newly create track. 
	// They return an error if the track couldn't be created or noErr on success.
	ComponentResult AddVideoTrack(KaxTrackEntry &kaxTrack, MatroskaTrack &mkvTrack);
	ComponentResult AddAudioTrack(KaxTrackEntry &kaxTrack, MatroskaTrack &mkvTrack);
	ComponentResult AddSubtitleTrack(KaxTrackEntry &kaxTrack, MatroskaTrack &mkvTrack);
	
	// this is called recursively to add only the leaves on the chapter tree to 
	// chapter track, since QT doesn't support chapter nesting.
	void AddChapterAtom(KaxChapterAtom *atom, Track chapterTrack);
	
	// assumes cluster has been read already, and cycles through the contained blocks and
	// adds the frames to the media/sample table, and to the track if addToTrack is true
	void ImportCluster(KaxCluster &cluster, bool addToTrack);
		
	ComponentInstance		self;
	Handle					dataRef;
	OSType					dataRefType;
	
	Movie					theMovie;
	Track					chapterTrack;
	Track					baseTrack;		// fake track created to set the duration 
											// of a movie while idle importing
	SInt64					timecodeScale;
	TimeValue64				movieDuration;	// in the timescale of timecodeScale
	
	IdleManager				idleManager;
	long					loadState;
	
	DataHandlerCallback		*ioHandler;
	EbmlStream				*aStream;
	
	EbmlElement				*el_l0;
	EbmlElement				*el_l1;
	
	vector<MatroskaTrack>	tracks;
};

#endif