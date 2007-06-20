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
#include <list>
#include <map>

#include <QuickTime/QuickTime.h>
#include "DataHandlerCallback.h"
#include "SubImport.h"

#include <ebml/EbmlStream.h>
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxInfo.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxChapters.h>
#include <matroska/KaxBlock.h>
#include <matroska/KaxAttachments.h>

using namespace libmatroska;
using namespace std;

// the maximum number of frames that have to be decoded between decoding
// any single frame and displaying it, used to cap the maximum size of the
// pts -> dts reorder buffer. Set to 4 since that's what it is in FFmpeg; 
// shouldn't be more than 2 with current codecs
#define MAX_DECODE_DELAY 4

struct MatroskaFrame {
	TimeValue64		dts;			// decode timestamp
	TimeValue64		pts;			// presentation/display timestamp
	TimeValue		duration;
	SInt64			offset;
	SInt64			size;
	short			flags;
	DataBuffer	   *buffer;
};

struct MatroskaSeekContext {
	EbmlElement		*el_l1;
	uint64_t		position;
};

// a list of level one elements and their offsets in the segment
class MatroskaSeek {
public:
	EbmlId GetID() const { return EbmlId(ebmlID, idLength); }
	bool operator<(const MatroskaSeek &rhs) const { return segmentPos < rhs.segmentPos; }
	bool operator>(const MatroskaSeek &rhs) const { return segmentPos > rhs.segmentPos; }
	
	MatroskaSeekContext GetSeekContext(uint64_t segmentOffset = 0) const {
		return (MatroskaSeekContext){ NULL, segmentPos + segmentOffset };
	}
	
	uint32_t		ebmlID;
	uint8_t			idLength;
	uint64_t		segmentPos;
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
	void AddBlock(KaxInternalBlock &block, uint32 duration, short flags);
	
	// this adds all the samples added through AddBlock() to the track that aren't already
	// added, e.g. from a previous call to AddSamplesToTrack()
	void AddSamplesToTrack();
	
	void FinishTrack();
	
	UInt16					number;
	UInt8					type, is_vobsub;
	Track					theTrack;
	Media					theMedia;
	SampleDescriptionHandle desc;
	QTMutableSampleTableRef sampleTable;
	QTSampleDescriptionID	qtSampleDesc;
	SInt64					timecodeScale;
	TimeValue64				maxLoadedTime;
	CXXSubtitleSerializer	*subtitleSerializer;
	Handle					subDataRefHandler;
	uint8					isEnabled;
	uint32_t				defaultDuration;
	
	// laced tracks can have multiple frames per block, it's easier to ignore them
	// for pts -> dts conversion (and laced tracks can't have non-keyframes anyways)
	bool					usesLacing;
	
private:
	// adds an individual frame from a block group into the sample table if it exists,
	// the media otherwise, and into the track if the track is a subtitle track.
	void AddFrame(MatroskaFrame &frame);
	
	// parses the first frame of a supported data format to determine codec parameters,
	// which can be more correct than the codec headers.
	void ParseFirstBlock(KaxInternalBlock &block);
	
	// Since the duration in Matroska files is generally rather unreliable, rely only on
	// the difference in timestamps between two frames. Thus, AddBlock() buffers frames
	// from one block group until the next block group is found to set the duration of the
	// previous ones to be the difference in timestamps.
	vector<MatroskaFrame>	lastFrames;
	int currentFrame;						// the frame we're currently determining the dts for
	
	// insert pts values, sort, then smallest value is current dts if size > decode delay
	list<TimeValue64>		ptsReorder;
	
	// We calculate the duration at a given dts, and have to save it until we find a 
	// frame with the same pts. This makes it such that we are actually calculating
	// the duration between display timestamps instead of decode timestamps.
	map<TimeValue64, TimeValue> durationForPTS;
	
	bool					seenFirstBlock;
	
	// When using a sample table, these store the range of samples that are in the 
	// sample table but not yet added to the media.
	SInt64					firstSample;		// -1 means it's not set
	SInt64					numSamples;
	
	// the amount of the media that needs to be added to the track
	TimeValue64				durationToAdd;
	
	// We don't want to add regions with frames that are displayed after the region.
	// Assume that when the sum of the display offsets is zero, this is true, and
	// update durationToAdd by adding durationSinceZeroSum.
	int						displayOffsetSum;
	SInt64					durationSinceZeroSum;
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
	ComponentResult SetupMovie();
	
	// This finds the next level 1 element and both replaces the el_l1 variable with it and
	// returns it. Does not read the data.
	EbmlElement *NextLevel1Element();
	
	// sets up timescale & file name metadata
	ComponentResult ReadSegmentInfo(KaxInfo &segmentInfo);
	
	// sets up all the movie tracks and media
	ComponentResult ReadTracks(KaxTracks &trackEntries);
	
	// Creates a chapter track, but doesn't actually add the chapter reference to the other
	// enabled tracks in case some weird file has this element before the Tracks element
	ComponentResult ReadChapters(KaxChapters &chapterEntries);
	
	// Activates any attached fonts, ignores other attachment types for now
	ComponentResult ReadAttachments(KaxAttachments &attachments);
	
	// Fills the levelOneElements vector with the positions of the elements in the seek head
	ComponentResult ReadMetaSeek(KaxSeekHead &seekHead);
	
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
	
	// we need to save a bit of context when seeking if we're going to seek back
	// This function saves el_l1 and the current file position to the returned context
	// and clears el_l1 to null in preparation for a seek.
	MatroskaSeekContext SaveContext();
	
	// This function restores el_l1 to what is saved in the context, deleting the current
	// value if not null, and seeks to the specified point in the file.
	void SetContext(MatroskaSeekContext context);
		
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
	TimeValue				lastIdleTime;	// the playback time of the movie when last idled
	int						idlesSinceLastAdd;	// number of idles since the last time
												// samples were added to the tracks
	
	DataHandlerCallback		*ioHandler;
	EbmlStream				*aStream;
	
	EbmlElement				*el_l0;
	EbmlElement				*el_l1;
	uint64_t				segmentOffset;
	
	vector<MatroskaTrack>	tracks;
	vector<MatroskaSeek>	levelOneElements;
};

#endif
