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

#ifndef __MKVIMPORTPRIVATE_H__
#define __MKVIMPORTPRIVATE_H__

#include "MatroskaImport.h"
#include <QuickTime/QuickTime.h>

#ifdef __cplusplus
#include <ebml/c/libebml_t.h>
#include <vector>

#include "DataHandlerCallback.h"
#include <ebml/EbmlHead.h>
#include <ebml/EbmlSubHead.h>
#include <ebml/EbmlStream.h>

#include <matroska/KaxCluster.h>
#include <matroska/KaxCues.h>
#include <matroska/KaxSeekHead.h>

using namespace std;
using namespace libmatroska;

typedef struct MkvBlock {
	vector<int64> dataOffset;
	vector<int64> dataSize;
	uint64 timecode;
	uint32 duration;
	uint16 numFrames;
	//uint16 trackNum;
	MediaSampleFlags sampleFlags;
} MkvBlock;

typedef struct MkvSeekEntry {
	int64		offset;
	UInt8		ebmlID[4];
	UInt8		length;
} MkvSeekEntry;

typedef struct MkvSegment {
	KaxSegment				*segment;
	vector<MkvSeekEntry>	seekEntries;
	UInt8					UID[16];
	Float64					duration;
	UInt64					timecodeScale;
	
	vector<MkvTrackRecord>	tracks;
} MkvSegment;

typedef struct MkvOrderedChapter {
	UInt8			segmentUID[16];
	UInt64			segmentStart;
	UInt64			segmentEnd;
} MkvOrderedChapter;

typedef struct KaxPrivate {
	IOCallback					*ioHandler;
	EbmlStream					*aStream;
	vector<MkvSegment>			segments;
	
	Movie						theMovie;
	Track						chapterTrack;
	Media						chapterMedia;
	bool						orderedChapters;
	vector<MkvOrderedChapter>	editList;
	
	KaxCluster					*currentCluster;
} KaxPrivate;

template <typename Type>
Type * FindNextChild(EbmlMaster & Master, const Type & PastElt)
{
	return static_cast<Type *>(Master.FindNextElt(PastElt, true));
}

extern "C"
{
#endif

ComponentResult ImportMkvRef(MatroskaImportGlobals globals);
	
// special value for importing an entire cluster at once (more efficient)
#define kFullCluster -1
ComponentResult ImportMkvIdle(MatroskaImportGlobals globals, 
							  long *outFlags, int numBlocks);

ComponentResult OpenFile(MatroskaImportGlobals globals);

void DisposeKaxPrivate(struct KaxPrivate *priv);
	
#ifdef __cplusplus
}
#endif

#define TIMECODE_SCALE_DEFAULT 1000000

#if defined(NDEBUG)
#define dprintf(...) {}
#else
#define dprintf printf
#endif

#define in_parent(p) (!p->IsFiniteSize() || (ioHandler->getFilePointer() < \
  (p->GetElementPosition() + p->HeadSize() + p->GetSize())))

#endif