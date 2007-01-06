/*
 *  MatroskaImport.cpp
 *
 *    MatroskaImport.cpp - QuickTime importer interface for opening a Matroska file.
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

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

#include <matroska/KaxSegment.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxBlock.h>

#include "MatroskaImportVersion.h"
#include "MatroskaImport.h"
#include "SubImport.h"
#include "Codecprintf.h"

using namespace libmatroska;

#pragma mark- Component Dispatch

// Setup required for ComponentDispatchHelper.c since I don't feel like writing a dispatcher
// for a C++ class akin to Apple's ACCodec.cpp classes for Core Audio codecs.
#define MOVIEIMPORT_BASENAME() 		MatroskaImport
#define MOVIEIMPORT_GLOBALS() 		MatroskaImport *storage

#define CALLCOMPONENT_BASENAME()	MOVIEIMPORT_BASENAME()
#define	CALLCOMPONENT_GLOBALS()		MOVIEIMPORT_GLOBALS()

#define COMPONENT_DISPATCH_FILE		"MatroskaImportDispatch.h"
#define COMPONENT_UPP_SELECT_ROOT()	MovieImport

extern "C" {
#if __MACH__
	#include <CoreServices/Components.k.h>
	#include <QuickTime/QuickTimeComponents.k.h>
	#include <QuickTime/ComponentDispatchHelper.c>
#else
	#include <Components.k.h>
	#include <QuickTimeComponents.k.h>
	#include <ComponentDispatchHelper.c>
#endif
}

#pragma mark-

// Component Open Request - Required
pascal ComponentResult MatroskaImportOpen(MatroskaImport *store, ComponentInstance self)
{
	// Allocate memory for our globals, and inform the component manager that we've done so
	store = new MatroskaImport(self);
	
	if (store == NULL)
		return memFullErr;

	SetComponentInstanceStorage(self, (Handle)store);
	
	return noErr;
}

// Component Close Request - Required
pascal ComponentResult MatroskaImportClose(MatroskaImport *store, ComponentInstance self)
{
	// Make sure to dealocate our storage
	delete store;
	
	return noErr;
}

// Component Version Request - Required
pascal ComponentResult MatroskaImportVersion(MatroskaImport *store)
{
	return kMatroskaImportVersion;
}

#pragma mark-

// MovieImportFile
pascal ComponentResult MatroskaImportFile(MatroskaImport *store, const FSSpec *theFile,
										  Movie theMovie, Track targetTrack, Track *usedTrack,
										  TimeValue atTime, TimeValue *durationAdded,
										  long inFlags, long *outFlags)
{
	OSErr err = noErr;
	AliasHandle alias = NULL;
    FSRef theFileFSRef;

	*outFlags = 0;

    err = FSpMakeFSRef(theFile, &theFileFSRef);
	if (err) goto bail;
	
	err = FSNewAliasMinimal(&theFileFSRef, &alias);
	if (err) goto bail;
	
	err = store->ImportDataRef((Handle)alias, rAliasType, theMovie, targetTrack,
							   usedTrack, atTime, durationAdded, inFlags, outFlags);
	if (err) goto bail;
	
	LoadExternalSubtitles(&theFileFSRef, theMovie);
	
bail:
	if (alias)
		DisposeHandle((Handle)alias);

	return err;
}

// MovieImportDataRef
pascal ComponentResult MatroskaImportDataRef(MatroskaImport *store, Handle dataRef,
											 OSType dataRefType, Movie theMovie,
											 Track targetTrack, Track *usedTrack,
											 TimeValue atTime, TimeValue *durationAdded,
											 long inFlags, long *outFlags)
{
	return store->ImportDataRef(dataRef, dataRefType, theMovie, targetTrack,
								usedTrack, atTime, durationAdded, inFlags, outFlags);
}

// MovieImportValidate
pascal ComponentResult MatroskaImportValidate(MatroskaImport *store, 
                                              const FSSpec *theFile, 
                                              Handle theData, Boolean *valid)
{
	OSErr err = noErr;
	AliasHandle alias = NULL; 
	FSRef theFileFSRef;
	
	*valid = false;
	
    err = FSpMakeFSRef(theFile, &theFileFSRef);
	if (err) goto bail;
	
	err = FSNewAliasMinimal(&theFileFSRef, &alias);
	if (err) goto bail;
	
	err = store->ValidateDataRef((Handle)alias, rAliasType, (UInt8 *)valid);

bail:
	if (alias)
		DisposeHandle((Handle)alias);

	return err;
}

// MovieImportGetMIMETypeList
pascal ComponentResult MatroskaImportGetMIMETypeList(MatroskaImport *store, 
                                                     QTAtomContainer *retMimeInfo)
{
	// Note that GetComponentResource is only available in QuickTime 3.0 or later.
	// However, it's safe to call it here because GetMIMETypeList is only defined in QuickTime 3.0 or later.
	return GetComponentResource((Component)store->Component(), 'mime', 510, (Handle *)retMimeInfo);
}

// MovieImportValidateDataRef
pascal ComponentResult MatroskaImportValidateDataRef(MatroskaImport *store, Handle dataRef, 
                                                     OSType dataRefType, UInt8 *valid)
{
	return store->ValidateDataRef(dataRef, dataRefType, valid);
}

pascal ComponentResult MatroskaImportIdle(MatroskaImport *store, long inFlags, long *outFlags)
{
	return noErr; //store->Idle(inFlags, outFlags);
}


MatroskaImport::MatroskaImport(ComponentInstance self)
{
	this->self = self;
	dataRef = NULL;
	dataRefType = 0;
	theMovie = NULL;
	chapterTrack = NULL;
	baseTrack = NULL;
	timecodeScale = 1000000;
	movieDuration = 0;
	ioHandler = NULL;
	aStream = NULL;
	el_l0 = NULL;
	el_l1 = NULL;
}

MatroskaImport::~MatroskaImport()
{
	if (el_l1)
		delete el_l1;
	
	if (el_l0)
		delete el_l0;
	
	if (aStream)
		delete aStream;
	
	if (ioHandler)
		delete ioHandler;
}

ComponentResult MatroskaImport::ImportDataRef(Handle dataRef, OSType dataRefType, Movie theMovie,
											  Track targetTrack, Track *usedTrack,
											  TimeValue atTime, TimeValue *durationAdded,
											  long inFlags, long *outFlags)
{
	ComponentResult err = noErr;
	this->dataRef = dataRef;
	this->dataRefType = dataRefType;
	this->theMovie = theMovie;
	
	*outFlags = 0;
	
	if (inFlags & movieImportMustUseTrack)
		return paramErr;
	
	try {
		if (!OpenFile())
			// invalid file, validate should catch this
			return invalidMovie;
		
		SetupMovie();
		
		do {
			KaxSegment & segment = *static_cast<KaxSegment *>(el_l0);
			
			if (EbmlId(*el_l1) == KaxCluster::ClassInfos.GlobalId) {
				int upperLevel = 0;
				EbmlElement *dummyElt = NULL;
				
				el_l1->Read(*aStream, KaxCluster::ClassInfos.Context, upperLevel, dummyElt, true);
				KaxCluster & cluster = *static_cast<KaxCluster *>(el_l1);
				KaxClusterTimecode & clusterTime = GetChild<KaxClusterTimecode>(cluster);
				
				cluster.SetParent(segment);
				cluster.InitTimecode(uint64(clusterTime), timecodeScale);
				
				KaxBlockGroup *blockGroup = FindChild<KaxBlockGroup>(cluster);
				while (blockGroup->GetSize() > 0) {
					KaxBlock & block = GetChild<KaxBlock>(*blockGroup);
					block.SetParent(cluster);
					
					for (int i = 0; i < tracks.size(); i++) {
						if (tracks[i].number == block.TrackNum())
							tracks[i].AddBlock(*blockGroup);
					}
					
					blockGroup = &GetNextChild<KaxBlockGroup>(cluster, *blockGroup);
				}
			}
		} while (NextLevel1Element());
		
		// insert the a/v tracks' samples
		for (int i = 0; i < tracks.size(); i++) {
			if (tracks[i].type != track_subtitle) {
				if (tracks[i].sampleTable) {
					err = AddSampleTableToMedia(tracks[i].theMedia, tracks[i].sampleTable, 1, 
												QTSampleTableGetNumberOfSamples(tracks[i].sampleTable), NULL);
					if (err)
						Codecprintf(NULL, "MKV: error adding sample table to media %d\n", err);
				}
				err = InsertMediaIntoTrack(tracks[i].theTrack, 0, 0, GetMediaDuration(tracks[i].theMedia), fixed1);
				if (err)
					Codecprintf(NULL, "MKV: error inserting media into track %d\n", err);
			}
		}
		
	} catch (CRTError &err) {
		return err.getError();
	}
	
	return noErr;
}

ComponentResult MatroskaImport::ValidateDataRef(Handle dataRef, OSType dataRefType, UInt8 *valid)
{
	this->dataRef = dataRef;
	this->dataRefType = dataRefType;
	
	*valid = 0;
	
	try {
		if (OpenFile())
			*valid = 128;
	} catch (CRTError &err) {
		return err.getError();
	}
	
	return noErr;
}
