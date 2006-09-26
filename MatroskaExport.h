/*
 *  MatroskaExport.h
 *
 *    MatroskaExport.h - Structs for the Matroska exporter
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

#ifndef __MATROSKAEXPORT_H__
#define __MATROSKAEXPORT_H__

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>
#include <vector>

#include <ebml/c/libebml_t.h>
#include <ebml/IOCallback.h>
#include <ebml/EbmlVoid.h>

#include <matroska/KaxCues.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxSeekHead.h>

using namespace libebml;
using namespace libmatroska;
using namespace std;

typedef struct OutputTrackRecord {
	long						trackID;
	MovieExportGetPropertyUPP	getPropertyProc;
	MovieExportGetDataUPP		getDataProc;
	void						*refCon;
	TimeScale					sourceTimeScale;
	
	KaxTrackEntry               *kaxTrack;
	Track                       theTrack;
	TimeValue64                 currentTime;	// decode time
} OutputTrack, *OutputTrackPtr;

typedef struct {
	ComponentInstance           self;
	ComponentInstance           quickTimeMovieExporter;
	vector<OutputTrack>         *outputTracks;
	MovieProgressUPP            progressProc;
	long                        progressRefcon;
	Boolean                     canceled;
	
	Movie                       theMovie;
	Track                       onlyThisTrack;
	uint64                      timecodeScale;         // default: milliseconds (can't be changed)
	TimeRecord                  duration;
	
	IOCallback                  *ioHandler;
	KaxSegment                  *segment;
	uint64                      segmentSize;           // the initial size of the segment
	uint64                      actSegmentSize;        // the actual size of the segment, calculated from its elements
	EbmlVoid                    *metaSeekPlaceholder;  // this saves space for the first seek head
	KaxSeekHead                 *metaSeek;             // this seek head indexes everything except clusters
	KaxSeekHead                 *clusterMetaSeek;      // this seek head indexes clusters, and is at the end
	KaxCues                     *cues;
	
	KaxCluster                  *currentCluster;
	TimeValue64                 currentClusterDuration;
// in ms
#define MAX_CLUSTER_DURATION 5000
	
	Ptr                         sampleBuffer;
} MatroskaExportGlobalsRecord, *MatroskaExportGlobals;

enum {
	kMatroskaExportFileNameExtention = 'mkv '
};

// Component data types, these are not yet public but they're just a FourCC
// indicating the value type - turn this off if one day they do go public
#if 1
enum {
    kComponentDataTypeFixed     = 'fixd', // Fixed (same as typeFixed )
	kComponentDataTypeInt16     = 'shor', // SInt16 (same as typeSInt16) 
	kComponentDataTypeCFDataRef = 'cfdt'  // CFDataRef 
};
#endif

// Component Properties specific to the Matroska Exporter component
enum {
    kComponentPropertyClass_Matroska = 'MkvF', // Matroska Component property class
};

static const ComponentPropertyInfo kExportProperties[] = 
{
    { kComponentPropertyClassPropertyInfo, kComponentPropertyInfoList,  kComponentDataTypeCFDataRef, sizeof(CFDataRef), kComponentPropertyFlagCanGetNow | kComponentPropertyFlagValueIsCFTypeRef | kComponentPropertyFlagValueMustBeReleased },
};

static const OSType kSupportedMediaTypes[] = 
{
	VideoMediaType,
	SoundMediaType
};

// Prototypes
static OSErr ConfigureQuickTimeMovieExporter(MatroskaExportGlobals store);
static void  GetExportProperty(MatroskaExportGlobals store);

#endif