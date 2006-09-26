/*
 *  MatroskaImport.c
 *
 *    MatroskaImport.c - QuickTime importer interface for opening a Matroska file.
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

#if __MACH__
    #include <Carbon/Carbon.h>
    #include <QuickTime/QuickTime.h>
#else
    #include <ConditionalMacros.h>
    #include <Endian.h>
    #include <Movies.h>
    #include <QuickTimeComponents.h>
    #include <FixMath.h> // for fixed1
#endif

#include "MatroskaImportVersion.h"
#include "MatroskaImport.h"
#include "MkvImportPrivate.h"

#pragma mark- Component Dispatch

// Setup required for ComponentDispatchHelper.c
#define MOVIEIMPORT_BASENAME() 		MatroskaImport
#define MOVIEIMPORT_GLOBALS() 		MatroskaImportGlobals storage

#define CALLCOMPONENT_BASENAME()	MOVIEIMPORT_BASENAME()
#define	CALLCOMPONENT_GLOBALS()		MOVIEIMPORT_GLOBALS()

#define COMPONENT_DISPATCH_FILE		"MatroskaImportDispatch.h"
#define COMPONENT_UPP_SELECT_ROOT()	MovieImport

#if __MACH__
	#include <CoreServices/Components.k.h>
	#include <QuickTime/QuickTimeComponents.k.h>
	#include <QuickTime/ComponentDispatchHelper.c>
#else
	#include <Components.k.h>
	#include <QuickTimeComponents.k.h>
	#include <ComponentDispatchHelper.c>
#endif

#pragma mark-

// Component Open Request - Required
pascal ComponentResult MatroskaImportOpen(MatroskaImportGlobals store, ComponentInstance self)
{
	OSErr err;

	// Allocate memory for our globals, and inform the component manager that we've done so
	store = (MatroskaImportGlobals)NewPtrClear(sizeof(MatroskaImportGlobalsRecord));
	if (err = MemError()) goto bail;

	store->self = self;

	SetComponentInstanceStorage(self, (Handle)store);

bail:
	return err;
}

// Component Close Request - Required
pascal ComponentResult MatroskaImportClose(MatroskaImportGlobals store, ComponentInstance self)
{
#pragma unused(self)
	// Make sure to dealocate our storage
	if (store) {
		if (store->kaxPrivate)
			DisposeKaxPrivate(store->kaxPrivate);
		
		DisposePtr((Ptr) store);
	}

	return noErr;
}

// Component Version Request - Required
pascal ComponentResult MatroskaImportVersion(MatroskaImportGlobals store)
{
#pragma unused(store)

	return kMatroskaImportVersion;
}

#pragma mark-

// MovieImportFile
pascal ComponentResult MatroskaImportFile(MatroskaImportGlobals store, const FSSpec *theFile,
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

	CFURLRef url = CFURLCreateFromFSRef(kCFAllocatorDefault, &theFileFSRef);
	if (url != NULL) {
		store->filename = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
		CFRelease(url);
	}
	
	
	err = MovieImportDataRef(store->self,
							(Handle)alias,
							rAliasType,
							theMovie,
							targetTrack,
							usedTrack,
							atTime,
							durationAdded,
							inFlags,
							outFlags);

bail:
	if (store->filename)
		CFRelease(store->filename);
	
	if (alias)
		DisposeHandle((Handle)alias);

	return err;
}

// MovieImportDataRef
pascal ComponentResult MatroskaImportDataRef(MatroskaImportGlobals store, Handle dataRef,
											  OSType dataRefType, Movie theMovie,
											  Track targetTrack, Track *usedTrack,
											  TimeValue atTime, TimeValue *durationAdded,
											  long inFlags, long *outFlags)
{
#pragma unused(store,targetTrack)

	OSErr err = noErr;

	*outFlags = 0;

	// movieImportMustUseTrack indicates that we must use an existing track, because we
	// don't support this and always create a new track return paramErr.
	if (inFlags & movieImportMustUseTrack)
		return paramErr;
	
	store->theMovie = theMovie;
	store->dataRef = dataRef;
	store->dataRefType = dataRefType;
	store->useIdle = 0;//inFlags & movieImportWithIdle;
	ImportMkvRef(store);
	
	if (store->useIdle)
		*outFlags |= movieImportResultNeedIdles;
	else
		*outFlags |= movieImportResultComplete;
	
    *durationAdded = GetMovieDuration(theMovie);
bail:

	return err;
}

// MovieImportValidate
pascal ComponentResult MatroskaImportValidate(MatroskaImportGlobals store, 
                                              const FSSpec *theFile, 
                                              Handle theData, Boolean *valid)
{
#pragma unused(theData)

	OSErr err = noErr;
	AliasHandle alias = NULL; 
	FSRef theFileFSRef;
	
	*valid = false;
	
    err = FSpMakeFSRef(theFile, &theFileFSRef);
	if (err) goto bail;
	
	err = FSNewAliasMinimal(&theFileFSRef, &alias);
	
	err = MovieImportValidateDataRef(store->self, (Handle)alias, rAliasType, (UInt8 *)valid);

bail:
	if (alias)
		DisposeHandle((Handle)alias);

	return err;
}

// MovieImportGetMIMETypeList
pascal ComponentResult MatroskaImportGetMIMETypeList(MatroskaImportGlobals store, 
                                                     QTAtomContainer *retMimeInfo)
{
	// Note that GetComponentResource is only available in QuickTime 3.0 or later.
	// However, it's safe to call it here because GetMIMETypeList is only defined in QuickTime 3.0 or later.
	return GetComponentResource((Component)store->self, FOUR_CHAR_CODE('mime'), 
                                512, (Handle *)retMimeInfo);
}

// MovieImportValidateDataRef
pascal ComponentResult MatroskaImportValidateDataRef(MatroskaImportGlobals store, 
                                                     Handle dataRef, 
                                                     OSType dataRefType, UInt8 *valid)
{
#pragma unused(store)

	OSErr err = noErr;
	
	store->dataRef = dataRef;
	store->dataRefType = dataRefType;
	
	if (OpenFile(store) == noErr)
		*valid = 128;
	else
		*valid = 0;
	
bail:
    return err;
}

pascal ComponentResult MatroskaImportIdle(MatroskaImportGlobals store, long inFlags, long *outFlags)
{
	ComponentResult err = noErr;
	TimeValue currentTime = GetMovieTime(store->theMovie, NULL);
	
	// if we're not actively playing back the movie or if we're too far behind, 
	// import an entire cluster because it's more efficient 
	if (store->lastIdleTime == currentTime)
		err = ImportMkvIdle(store, outFlags, kFullCluster);
	
	// but if we're playing it back, only import blocks up to 
	else if (currentTime + 2 * GetMovieTimeScale(store->theMovie) > store->maxLoadedTime)
		err = ImportMkvIdle(store, outFlags, 1);
	
	store->lastIdleTime = currentTime;
	
	if (*outFlags & movieImportResultComplete && store->baseTrack)
		// when we're done importing, we don't need the fake track around any longer to keep the duration
		DisposeMovieTrack(store->baseTrack);
	
	return err;
}
