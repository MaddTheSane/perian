/*
 *  MatroskaExport.c
 *
 *    MatroskaExport.c - QuickTime exporter interface for creating a Matroska file.
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

#include <ebml/c/libebml_t.h>
#include <ebml/IOCallback.h>

#include "DataHandlerCallback.h"

#include "MatroskaExportVersion.h"
#include "MatroskaExport.h"
#include "MkvExportPrivate.h"

#include <iostream>
using namespace std;

#pragma mark- Component Dispatch

// Setup required for ComponentDispatchHelper.c
#define MOVIEEXPORT_BASENAME() 		MatroskaExport
#define MOVIEEXPORT_GLOBALS() 		MatroskaExportGlobals storage

#define CALLCOMPONENT_BASENAME()	MOVIEEXPORT_BASENAME()
#define	CALLCOMPONENT_GLOBALS()		MOVIEEXPORT_GLOBALS()

#define QT_BASENAME()               MOVIEEXPORT_BASENAME()
#define QT_GLOBALS()                MOVIEEXPORT_GLOBALS()

#define COMPONENT_DISPATCH_FILE		"MatroskaExportDispatch.h"
#define COMPONENT_UPP_SELECT_ROOT()	MovieExport

extern "C" 
{
#if __MACH__
	#include <CoreServices/Components.k.h>
	#include <QuickTime/QuickTimeComponents.k.h>
    #include <QuickTime/ImageCompression.k.h>   // for ComponentProperty selectors
	#include <QuickTime/ComponentDispatchHelper.c>
#else
	#include <Components.k.h>
	#include <QuickTimeComponents.k.h>
    #include <ImageCompression.k.h>
	#include <ComponentDispatchHelper.c>
#endif
}

#pragma mark-

// Component Open Request - Required
pascal ComponentResult MatroskaExportOpen(MatroskaExportGlobals store, ComponentInstance self)
{		
	ComponentDescription cd;
	ComponentResult 	 err;
	
	// Allocate memory for our globals, and inform the component manager that we've done so
	store = (MatroskaExportGlobals)NewPtrClear(sizeof(MatroskaExportGlobalsRecord));
	err = MemError();
	if ( err ) goto bail;

	store->self = self;
	SetComponentInstanceStorage(self, (Handle)store);

	// Get the QuickTime Movie export component
	// Because we use the QuickTime Movie export component, search for
	// the 'MooV' exporter using the following ComponentDescription values
	cd.componentType = MovieExportType;
	cd.componentSubType = kQTFileTypeMovie;
	cd.componentManufacturer = kAppleManufacturer;
	cd.componentFlags = canMovieExportFromProcedures | movieExportMustGetSourceMediaType;
	cd.componentFlagsMask = cd.componentFlags;

	err = OpenAComponent(FindNextComponent(NULL, &cd), &store->quickTimeMovieExporter);
	
	store->outputTracks = new vector<OutputTrack>();
	store->sampleBuffer = NewPtr(0);
	
bail:	
	return err;
}

// Component Close Request - Required
pascal ComponentResult MatroskaExportClose(MatroskaExportGlobals store, ComponentInstance self)
{
	// Make sure to deallocate our storage
	if (store) {
		if (store->quickTimeMovieExporter)
			CloseComponent(store->quickTimeMovieExporter);
		
		if (store->outputTracks)
			delete store->outputTracks;
		
		if (store->clusterMetaSeek)
			delete store->clusterMetaSeek;
		
		if (store->segment)
			delete store->segment;
		
		if (store->metaSeek)
			delete store->metaSeek;
		
		if (store->metaSeekPlaceholder)
			delete store->metaSeekPlaceholder;
		
		if (store->sampleBuffer)
			DisposePtr(store->sampleBuffer);
		
		if (store->cues)
			delete store->cues;
		
		if (store->currentCluster) {
			store->currentCluster->ReleaseFrames();
			delete store->currentCluster;
		}
		
		DisposePtr((Ptr)store);
	}

	return noErr;
}

// Component Version Request - Required
pascal ComponentResult MatroskaExportVersion(MatroskaExportGlobals store)
{
	return kMatroskaExportVersion;
}

// GetComponentPropertyInfo
// Component Property Info request - Optional but good practice for QuickTime 7 forward
// Returns information about the properties of a component
pascal ComponentResult MatroskaExportGetComponentPropertyInfo(MatroskaExportGlobals   store,
                                                               ComponentPropertyClass inPropClass,
                                                               ComponentPropertyID    inPropID,
                                                               ComponentValueType     *outPropType,
                                                               ByteCount              *outPropValueSize,
                                                               UInt32                 *outPropertyFlags)
{
	// we support kComponentPropertyClassPropertyInfo and our own kComponentPropertyClass_Matroska class
    ComponentResult err = kQTPropertyNotSupportedErr;
    
    switch (inPropClass) {
		case kComponentPropertyClassPropertyInfo:
			switch (inPropID) {
			case kComponentPropertyInfoList:
				if (outPropType) *outPropType = kExportProperties[0].propType;
				if (outPropValueSize) *outPropValueSize = kExportProperties[0].propSize;
				if (outPropertyFlags) *outPropertyFlags = kExportProperties[0].propFlags;
				err = noErr;
				break;
			default:
				break;
			}
			break;
		default:
			break;
    }
        
    return err;
}

// GetComponentProperty
// Get Component Property request - Optional but good practice for QuickTime 7 forward
// Returns the value of a specific component property
pascal ComponentResult MatroskaExportGetComponentProperty(MatroskaExportGlobals  store,
                                                          ComponentPropertyClass inPropClass,
                                                          ComponentPropertyID    inPropID,
                                                          ByteCount              inPropValueSize,
                                                          ComponentValuePtr      outPropValueAddress,
                                                          ByteCount              *outPropValueSizeUsed)
{
	ByteCount size = 0;
	UInt32 flags = 0;
    CFDataRef *outPropCFDataRef;
    
    ComponentResult err = noErr;
    
    // sanity check
    if (NULL == outPropValueAddress) return paramErr;
    
    err = QTGetComponentPropertyInfo(store->self, inPropClass, inPropID, NULL, &size, &flags);
    if (err) goto bail;
    
    if (size > inPropValueSize) return kQTPropertyBadValueSizeErr;
    
    if (flags & kComponentPropertyFlagCanGetNow) {
        switch (inPropID) {
		case kComponentPropertyInfoList:
            outPropCFDataRef = (CFDataRef *)outPropValueAddress;
            *outPropCFDataRef = CFDataCreate(kCFAllocatorDefault, 
											 (UInt8 *)((ComponentPropertyInfo *)kExportProperties), 
											 sizeof(kExportProperties));
            if (outPropValueSizeUsed) *outPropValueSizeUsed = size;
			break;
        default:
            break;
        }
    }
    
bail:
    return err;
}

// SetComponentProperty
// Set Component Property request - Optional but good practice for QuickTime 7 forward
// Sets the value of a specific component property
pascal ComponentResult MatroskaExportSetComponentProperty(MatroskaExportGlobals  store,
                                                          ComponentPropertyClass inPropClass,
                                                          ComponentPropertyID    inPropID,
                                                          ByteCount              inPropValueSize,
                                                          ConstComponentValuePtr inPropValueAddress)
{
	ByteCount size = 0;
	OSType type;
	UInt32 flags;
        
    ComponentResult err = noErr;
        
    // validate the caller's params    
    err = QTGetComponentPropertyInfo(store->self, inPropClass, inPropID, &type, &size, &flags);
    if (err) goto bail;
    
    if (size != inPropValueSize) return kQTPropertyBadValueSizeErr;
    if (NULL == inPropValueAddress) return paramErr;
    
    if (!(flags & kComponentPropertyFlagCanSetNow)) return kQTPropertyReadOnlyErr;
    
bail:
    return err;
}

#pragma mark-

// MovieExportToFile
// 		Exports data to a file. The requesting program or Movie Toolbox must create the destination file
// before calling this function. Your component may not destroy any data in the destination file. If you
// cannot add data to the specified file, return an appropriate error. If your component can write data to
// a file, be sure to set the canMovieExportFiles flag in the componentFlags field of your component's
// ComponentDescription structure. Your component must be prepared to perform this function at any time.
// You should not expect that any of your component's configuration functions will be called first. 
pascal ComponentResult MatroskaExportToFile(MatroskaExportGlobals store, const FSSpec *theFilePtr,
                                             Movie theMovie, Track onlyThisTrack, TimeValue startTime,
                                             TimeValue duration)
{
	AliasHandle		alias;
	ComponentResult	err;

	err = QTNewAlias(theFilePtr, &alias, true);
	if ( err ) goto bail;

	// Implement the export though a file dataRef
	err = MovieExportToDataRef(store->self, (Handle)alias, rAliasType, theMovie, 
							   onlyThisTrack, startTime, duration);

	DisposeHandle((Handle)alias);

bail:
	return err;
}

// MovieExportToDataRef
//      Allows an application to request that data be exported to a data reference.
pascal ComponentResult MatroskaExportToDataRef(MatroskaExportGlobals store, Handle dataRef, 
											   OSType dataRefType, Movie theMovie, 
											   Track onlyThisTrack, TimeValue startTime, 
											   TimeValue duration)
{
	TimeScale 				  scale;
	MovieExportGetPropertyUPP getVideoPropertyProc = NULL;
	MovieExportGetDataUPP	  getVideoDataProc = NULL;
	void					  *videoRefCon;
	long					  trackID;
	ComponentResult			  err;
	
	// Set up the video source
	if (true) {
		// we only do passthrough for now
		store->theMovie = theMovie;
		store->onlyThisTrack = onlyThisTrack;
		
		for (int i = 1; i <= GetMovieTrackCount(theMovie); i++) {
			OutputTrack outTrack = {0};
			outTrack.theTrack = GetMovieIndTrack(theMovie, i);
			store->outputTracks->push_back(outTrack);
		}
		
	} else {
	
		// This call returns a MovieExportGetPropertyProc (MovieExportGetPropertyUPP) 
		// and a MovieExportGetDataProc (MovieExportGetDataUPP) callbacks that can be
		// passed to MovieExportAddDataSource to create a new data source. 
		// This function provides a standard way of getting data using this protocol 
		// out of a movie or track. The returned procedures must be disposed by calling
		// MovieExportDisposeGetDataAndPropertiesProcs. 
		err = MovieExportNewGetDataAndPropertiesProcs(store->quickTimeMovieExporter,
													  VideoMediaType, &scale, theMovie,
													  onlyThisTrack, startTime, duration,	
													  &getVideoPropertyProc, &getVideoDataProc,
													  &videoRefCon);
		if (err) goto bail;

		// ** Add the video data source **
		
		// Before starting an export operation, all the data sources must be defined 
		// by calling this function once for each data source
		err = MovieExportAddDataSource(store->self, VideoMediaType, scale, &trackID, 
									   getVideoPropertyProc, getVideoDataProc, videoRefCon);
		if (err) goto bail;
	}

	// Perform the export operation
	err = MovieExportFromProceduresToDataRef(store->self, dataRef, dataRefType);

bail:
	if (getVideoPropertyProc || getVideoDataProc)
		// Dispose the the memory associated with the procedures returned by 
		// MovieExportNewGetDataAndPropertiesProcs 
		MovieExportDisposeGetDataAndPropertiesProcs(store->quickTimeMovieExporter, 
													getVideoPropertyProc, 
													getVideoDataProc, 
													videoRefCon);
	
	return err;
}

// MovieExportFromProceduresToDataRef
//		Exports data provided by MovieExportAddDataSource to a location specified 
// by dataRef and dataRefType.
// Movie data export components that support export operations from procedures 
// must set the canMovieExportFromProcedures flag in their component flags. 
pascal ComponentResult MatroskaExportFromProceduresToDataRef(MatroskaExportGlobals store, 
															 Handle dataRef, OSType dataRefType)
{
	long			numOfFrames = 0;
	TimeValue		duration = 0;
	DataHandler 	dataH = NULL;
	Boolean			progressOpen = false;
	ComponentResult	err;
	
	if (!dataRef || !dataRefType)
		return paramErr;

	// Get and open a Data Handler Component that can write to the dataRef
	err = OpenAComponent(GetDataHandler(dataRef, dataRefType, kDataHCanWrite), &dataH);
	if (err) goto bail;

	// Set the DataRef
	DataHSetDataRef(dataH, dataRef);

	// Create the file
	err = DataHCreateFile(dataH, 'TVOD', false);
	if ( err ) goto bail;

	// Set the file type - this will fail on some platforms, and that's fine
	DataHSetMacOSFileType(dataH, 'MkvF');

	// Open for write operations
	err = DataHOpenForWrite(dataH);
	if (err) goto bail;

	// Open for write operations
	try {
		store->ioHandler = new DataHandlerCallback(dataH, MODE_WRITE);
	} catch (CRTError crtErr) {
		printf("%s\n", crtErr.what());
		goto bail;
	}

	// If we have at least one source track added in the MovieExportAddDataSource call, write some frames
	if (store->outputTracks->size() > 0) {
		// Since the property proc we call may be a proc returned from MovieExportNewGetDataAndPropertiesProcs,
		// configure the QT movie exporter so that it's properties match what we have as our defaults.
		// Before we call the proc, we always init the video property to the current default. The proc can either
		// be a client proc or one of the procs returned from MovieExportNewGetDataAndPropertiesProcs. In the
		// first case, if the proc returns an error for a property selector, the default will be used. If the 
		// proc is a movie exporter proc, it will always return a property -- namely the property configured for
		// the current exporter. If we don't configure the movie exporter, the wrong default would be returned.
		err = ConfigureQuickTimeMovieExporter(store);
		if (err) goto bail;
		
		// Call the property proc to get our export properties
		//GetExportProperty(store);

		// If there's a progress proc call it with open request
		if (store->progressProc) {
			TimeRecord durationTimeRec;
			
			// Get the track duration if it is available
			if (store->theMovie) {
				store->duration.value.lo = GetMovieDuration(store->theMovie);
				store->duration.scale = GetMovieTimeScale(store->theMovie);
				
				InvokeMovieProgressUPP(NULL, movieProgressOpen, progressOpExportMovie, 0, 
									   store->progressRefcon, store->progressProc);
				progressOpen = true;
				
			} else {
				
				if (InvokeMovieExportGetPropertyUPP(store->outputTracks->at(0).refCon, 1, 
													movieExportDuration, &durationTimeRec, 
													store->outputTracks->at(0).getPropertyProc) == noErr) {
					
					ConvertTimeScale(&durationTimeRec, store->outputTracks->at(0).sourceTimeScale);
					duration = durationTimeRec.value.lo;

					InvokeMovieProgressUPP(NULL, movieProgressOpen, progressOpExportMovie, 0, 
										   store->progressRefcon, store->progressProc);
					progressOpen = true;	
					
					store->duration = durationTimeRec;
				} else
					store->duration.scale = 1;
			}
		}
		
		WriteHeader(store);
		WriteTrackInfo(store);
		StartNewCluster(store, 0);
		
		while (true) {
			err = WriteFrame(store);
			if (err == eofErr)
				break;
			
			// Indicate our components progress if required
			if (progressOpen) {
				
				Fixed percentDone = FixDiv(store->outputTracks->at(0).currentTime, duration);
				
				if (percentDone > 0x010000)
					percentDone = 0x010000;
				
				err = InvokeMovieProgressUPP(NULL, movieProgressUpdatePercent, 
											 progressOpExportMovie, percentDone, 
											 store->progressRefcon, store->progressProc);
				if (err) goto bail;
			}
		}
			
		FinishFile(store);
	}
	
bail:
	//if (outputTrack)
	//	EmptyOutputTrack(outputTrack);

	if (store->ioHandler)
		delete store->ioHandler;

	if (dataH)
		CloseComponent(dataH);
	
	// Call the progress proc with a close request if required
	if (progressOpen)
		InvokeMovieProgressUPP(NULL, movieProgressClose, progressOpExportMovie, 
							   0, store->progressRefcon, store->progressProc);

	return err;
}

// MovieExportAddDataSource
// 		Defines a data source for use with an export operation performed by 
// MovieExportFromProceduresToDataRef.
// Before starting an export operation, all the data sources must be defined by 
// calling this function once for each data source.
pascal ComponentResult MatroskaExportAddDataSource(MatroskaExportGlobals store, 
												   OSType trackType, TimeScale scale,
												   long *trackIDPtr, 
												   MovieExportGetPropertyUPP getPropertyProc,
												   MovieExportGetDataUPP getDataProc, void *refCon)
{
	ComponentResult	err = noErr;
	
	// We need these or we can't add the data source
	if (!scale || !trackType || !getDataProc || !getPropertyProc)
		return paramErr;

	if (trackType == VideoMediaType || trackType == SoundMediaType) { 
		OutputTrack outputTrack = {0};

		outputTrack.trackID = store->outputTracks->size() + 1;
		outputTrack.getPropertyProc = getPropertyProc;
		outputTrack.getDataProc = getDataProc;
		outputTrack.refCon = refCon;
		outputTrack.sourceTimeScale = scale;

		store->outputTracks->push_back(outputTrack);
		*trackIDPtr = outputTrack.trackID;
	}

bail:
	return err;
}

// MovieExportValidate 
//		Determines whether a movie export component can export all the data for 
// a specified movie or track.
// This function allows an application to determine if a particular movie or track 
// could be exported by the specified movie data export component. The movie or 
// track is passed in the theMovie and onlyThisTrack parameters as they are
// passed to MovieExportToFile. Although a movie export component can export one 
// or more media types, it may not be able to export all the kinds of data stored 
// in those media. Movie data export components that implement this function must
// also set the canMovieExportValidateMovie flag.
pascal ComponentResult MatroskaExportValidate(MatroskaExportGlobals store, 
											  Movie theMovie, Track onlyThisTrack, 
											  Boolean *valid)
{
	OSErr err;

	// The QT movie export component must be cool with this before we can be
	err = MovieExportValidate(store->quickTimeMovieExporter, theMovie, onlyThisTrack, valid);
	if (err) goto bail;

	if (*valid == true) {
		*valid = false;
		// check for at least one supported media type
		if (onlyThisTrack == NULL) {
			for (int i = 0; i < sizeof(kSupportedMediaTypes) / sizeof(OSType); i++) {
				if (GetMovieIndTrackType(theMovie, 1, kSupportedMediaTypes[i], movieTrackMediaType)) {
					*valid = true;
					break;
				}
			}
		} else {
			OSType mediaType;
			GetMediaHandlerDescription(GetTrackMedia(onlyThisTrack), &mediaType, NULL, NULL);
			
			for (int i = 0; i < sizeof(kSupportedMediaTypes) / sizeof(OSType); i++) {
				if (mediaType == kSupportedMediaTypes[i]) {
					*valid = true;
					break;
				}
			}
		}	
	}
	
bail:
	return err;
}

#pragma mark -

/* Your component may provide configuration functions. These functions allow 
   applications to configure your component before the Movie Toolbox calls your 
   component to start the export process. Note that applications may call these 
   functions directly. These functions are optional. If your component receives 
   a request that it does not support, you should return the badComponentSelector
   error code. In addition, your component should work properly even if none of 
   these functions is called. 

   Applications may retrieve additional data from your component by calling 
    MovieExportGetAuxiliaryData.
   Applications may specify a progress function callback for use by your 
    component by calling MovieExportSetProgressProc.
   Applications may instruct your component to display it's configuration dialog 
    box by calling MovieExportDoUserDialog.
*/

// MovieExportSetProgressProc
// 		Assigns a movie progress function to your component. This progress functions 
// supports the same interface as Movie Toolbox progress functions. Note that this 
// interface not only allows you to report progress to the  application, but also 
// allows the application to cancel the request.
// See http://developer.apple.com/qa/qa2001/qa1230.html
pascal ComponentResult MatroskaExportSetProgressProc(MatroskaExportGlobals store, 
													 MovieProgressUPP proc, long refcon)
{
	store->progressProc = proc;
	store->progressRefcon = refcon;

	return noErr;
}

#pragma mark-

// MovieExportGetFileNameExtension
// 		Returns an OSType of the movie export components current file name extention. 
pascal ComponentResult MatroskaExportGetFileNameExtension(MatroskaExportGlobals store, OSType *extension)
{
	*extension = kMatroskaExportFileNameExtention;
	
	return noErr;
}

// MovieExportGetShortFileTypeString
//		Returns a pascal file type string for the exported file.
pascal ComponentResult MatroskaExportGetShortFileTypeString(MatroskaExportGlobals store, Str255 typeString)
{
	return GetComponentIndString((Component)store->self, typeString, kMatroskaExportShortFileTypeNamesResID, 1);
}

// MovieExportGetSourceMediaType
//		Return either the track type if the movie export component is 
// track-specific or 0 if it is track-independent.
pascal ComponentResult MatroskaExportGetSourceMediaType(MatroskaExportGlobals store, OSType *mediaType)
{
#pragma unused(store)

	if (!mediaType)
		return paramErr;
	
	// we take entire movies; more than one type of track
	*mediaType = 0;

	return noErr;
}

// ConfigureQuickTimeMovieExporter
// 		Since the property proc we call may be a proc returned from 
// MovieExportNewGetDataAndPropertiesProcs, configure the QuickTime Movie Exporter 
// so that it's properties match what we have as our defaults.	
static OSErr ConfigureQuickTimeMovieExporter(MatroskaExportGlobals store)
{
	SCTemporalSettings temporalSettings;
	SCSpatialSettings  spatialSettings;
	ComponentInstance  stdImageCompression = NULL;
	QTAtomContainer	   movieExporterSettings = NULL;
	OSErr			   err;

	// Open the Standard Compression component
	err = OpenADefaultComponent(StandardCompressionType, StandardCompressionSubType, &stdImageCompression);
	if (err) goto bail;
	
	// Set the frame rate
	//temporalSettings.frameRate = store->fps;
	err = SCSetInfo(stdImageCompression, scTemporalSettingsType, &temporalSettings);
	if (err) goto bail;

	// Set the depth
	//spatialSettings.depth = store->depth;
	err = SCSetInfo(stdImageCompression, scSpatialSettingsType, &spatialSettings);
	if (err) goto bail;

	// Get the settings atom
	err = SCGetSettingsAsAtomContainer(stdImageCompression, &movieExporterSettings);
	if (err) goto bail;
	
	// Set the compression settings for the QT Movie Exporter
	err = MovieExportSetSettingsFromAtomContainer(store->quickTimeMovieExporter, movieExporterSettings);
	
bail:
	if (stdImageCompression)
		CloseComponent(stdImageCompression);
	
	if (movieExporterSettings)
		DisposeHandle(movieExporterSettings);
		
	return err;
}
#if 0
// GetExportProperty
//		This routine calls the MovieExportGetPropertyProc to set up our output properties
static void GetExportProperty(MatroskaExportGlobals store)
{
	SCTemporalSettings	temporalSettings;
	SCSpatialSettings	spatialSettings;
	Fixed				width, height;
	OutputTrackPtr		outputTrack = &store->outputTracks[0];
	
	// Get the defaults
	//outputTrack->fps = store->fps;
	//outputTrack->depth = store->depth;
	
 	// The getPropertyProc may override some values
 	if (InvokeMovieExportGetPropertyUPP(outputTrack->refCon, outputTrack->trackID, 
										scTemporalSettingsType, &temporalSettings, 
										outputTrack->getPropertyProc) == noErr)
		outputTrack->fps = temporalSettings.frameRate;

	if (InvokeMovieExportGetPropertyUPP(outputTrack->refCon, outputTrack->trackID, 
										scSpatialSettingsType, &spatialSettings, 
										outputTrack->getPropertyProc) == noErr)
		outputTrack->depth = spatialSettings.depth;

	if (InvokeMovieExportGetPropertyUPP(outputTrack->refCon, outputTrack->trackID, 
										movieExportWidth, &width, 
										outputTrack->getPropertyProc) == noErr)
		outputTrack->width = width;
	
	if (InvokeMovieExportGetPropertyUPP(outputTrack->refCon, outputTrack->trackID, 
										movieExportHeight, &height, 
										outputTrack->getPropertyProc) == noErr)
		outputTrack->height = height;
}
#endif
