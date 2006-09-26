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

#include <QuickTime/QuickTime.h>

struct MkvBlock;
struct KaxPrivate;

typedef struct {
	UInt16							trackNumber;
	Track							theTrack;
	Media							theMedia;
	SampleDescriptionHandle			sampleHdl;
	AudioStreamBasicDescription		asbd;
	UInt64							defaultDuration;
	// this is so the last block of a track that has no DefaultDuration can be imported
	// with the same duration as the preceding block
	// any better solutions?
	UInt64							lastDuration;
	UInt64							uid;
	SInt64							timecodeScale;
	
	QTMutableSampleTableRef			sampleTable;
	QTSampleDescriptionID			qtSampleDesc;
		
	UInt8							trackType;
	
	struct MkvBlock					*lastBlock;
} MkvTrackRecord, *MkvTrackPtr, **MkvTrackHandle;

// Component globals
typedef struct {
	ComponentInstance		self;
	Handle					dataRef;
	OSType					dataRefType;
	
	Movie					theMovie;
	Track					baseTrack;			// so that the duration of the movie remains 
												// constant while idle importing
	SInt64					segmentTimecodeScale;
	TimeValue64				movieDuration;		// in the timescale of segmentTimecodeScale

	CFStringRef				filename;
	bool					useIdle;
	TimeValue				maxLoadedTime;
	TimeValue				lastIdleTime;
	struct KaxPrivate		*kaxPrivate;
} MatroskaImportGlobalsRecord, *MatroskaImportGlobals;

#endif