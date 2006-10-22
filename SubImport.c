/*
 *  SubImport.c
 *  Perian
 *
 *  Created by David Conrad on 10/12/06.
 *  Copyright 2006 Perian Project. All rights reserved.
 *
 */

#include <QuickTime/QuickTime.h>
#include "SubImport.h"
#include "CommonUtils.h"

// if the subtitle filename is something like title.en.srt or movie.fre.srt
// this function detects it and returns the subtitle language
short GetFilenameLanguage(CFStringRef filename)
{
	CFRange findResult;
	CFStringRef baseName = NULL;
	CFStringRef langStr = NULL;
	short lang = langUnspecified;

	// find and strip the extension
	findResult = CFStringFind(filename, CFSTR("."), kCFCompareBackwards);
	findResult.length = findResult.location;
	findResult.location = 0;
	baseName = CFStringCreateWithSubstring(NULL, filename, findResult);

	// then find the previous period
	findResult = CFStringFind(baseName, CFSTR("."), kCFCompareBackwards);
	findResult.location++;
	findResult.length = CFStringGetLength(baseName) - findResult.location;

	// check for 3 char language code
	if (findResult.length == 3) {
		char langCStr[4] = "";

		langStr = CFStringCreateWithSubstring(NULL, baseName, findResult);
		CFStringGetCString(langStr, langCStr, 4, kCFStringEncodingASCII);
		lang = ThreeCharLangCodeToQTLangCode(langCStr);

		CFRelease(langStr);
	
	// and for a 2 char language code
	} else if (findResult.length == 2) {
		char langCStr[3] = "";
		
		langStr = CFStringCreateWithSubstring(NULL, baseName, findResult);
		CFStringGetCString(langStr, langCStr, 3, kCFStringEncodingASCII);
		lang = TwoCharLangCodeToQTLangCode(langCStr);

		CFRelease(langStr);
	}
	
	CFRelease(baseName);
	return lang;
}

Track CreatePlaintextSubTrack(Movie theMovie, ImageDescriptionHandle imgDesc, 
                              TimeScale timescale, Handle dataRef, OSType dataRefType)
{
	Rect movieBox;
	Fixed trackWidth, trackHeight;
	Track theTrack;
	Media theMedia;

	// plain text subs have no size on their own
	GetMovieBox(theMovie, &movieBox);
	trackWidth = IntToFixed(movieBox.right - movieBox.left);
	trackHeight = IntToFixed(movieBox.bottom - movieBox.top);

	(*imgDesc)->idSize = sizeof(ImageDescription);
	(*imgDesc)->cType = kSubFormatUTF8;
	(*imgDesc)->width = FixedToInt(trackWidth);
	(*imgDesc)->height = FixedToInt(trackHeight);
	(*imgDesc)->frameCount = 1;
	(*imgDesc)->depth = 32;
	(*imgDesc)->clutID = -1;

	theTrack = NewMovieTrack(theMovie, trackWidth, trackHeight, kNoVolume);
	if (theTrack != NULL) {
		theMedia = NewTrackMedia(theTrack, VideoMediaType, timescale, dataRef, dataRefType);

		if (theMedia != NULL) {
			// finally, say that we're transparent
			MediaHandler mh = GetMediaHandler(theMedia);
			MediaSetGraphicsMode(mh, graphicsModePreWhiteAlpha, NULL);
		}
	}

	return theTrack;
}

ComponentResult LoadSubRipSubtitles(const FSRef *theDirectory, CFStringRef filename, Movie theMovie, Track *firstSubTrack)
{
	ComponentResult err = noErr;
	Handle dataRef = NULL;
	OSType dataRefType = rAliasType;
	HFSUniStr255 hfsFilename;
	CFRange filenameLen;
	Track theTrack = NULL;
	Media theMedia = NULL;

	ComponentInstance dataHandler = NULL;
	long filePos = 0;
	long fileSize;
	char *data = NULL;
	ImageDescriptionHandle textDesc = (ImageDescriptionHandle) NewHandleClear(sizeof(ImageDescription));

	filenameLen.location = 0;	
	filenameLen.length = hfsFilename.length = CFStringGetLength(filename);
	CFStringGetCharacters(filename, filenameLen, hfsFilename.unicode);

	err = FSNewAliasMinimalUnicode(theDirectory, hfsFilename.length, hfsFilename.unicode, (AliasHandle *)&dataRef, NULL);
	if (err) goto bail;

	err = OpenAComponent(GetDataHandler(dataRef, dataRefType, kDataHCanRead), &dataHandler);
	if (err) goto bail;

	err = DataHSetDataRef(dataHandler, dataRef);
	if (err) goto bail;

	err = DataHOpenForRead(dataHandler);
	if (err) goto bail;

	err = DataHGetFileSize(dataHandler, &fileSize);
	if (err) goto bail;

	// cap subs at 5 megabytes; any more is really insane
	if (fileSize > 5*1024*1024)
		goto bail;

	// load the subtitle file
	data = NewPtr(fileSize + 1);
	err = DataHScheduleData(dataHandler, data, 0, fileSize, 0, NULL, NULL);
	if (err) goto bail;
	data[fileSize] = '\0';

	// millisecond accuracy
	theTrack = CreatePlaintextSubTrack(theMovie, textDesc, 1000, dataRef, dataRefType);
	if (theTrack == NULL) {
		err = GetMoviesError();
		goto bail;
	}

	theMedia = GetTrackMedia(theTrack);
	if (theMedia == NULL) {
		err = GetMoviesError();
		goto bail;
	}

	unsigned int subNum = 1;
	char subNumStr[10];

	sprintf(subNumStr, "%u", subNum);
	char *subOffset = strstr(data, subNumStr);

	while (subOffset != NULL) {
		unsigned int starthour, startmin, startsec, startmsec;
		unsigned int endhour, endmin, endsec, endmsec;

		// skip the line with the subtitle number in it
		char *subTimecodeOffset = strchr(subOffset, '\n') + 1;

		int ret = sscanf(subTimecodeOffset, "%u:%u:%u,%u --> %u:%u:%u,%u", 
				         &starthour, &startmin, &startsec, &startmsec, 
				         &endhour, &endmin, &endsec, &endmsec);

		// find the next subtitle
		// setting subOffset to point the end of the current subtitle text
		sprintf(subNumStr, "\n%u", ++subNum);
		subOffset = strstr(subTimecodeOffset, subNumStr);

		if (ret == 8) {
			// skip to the beginning of the subtitle text
			char *subTextOffset = strchr(subTimecodeOffset, '\n') + 1;

			TimeValue startTime = startmsec + 1000*startsec + 60*1000*startmin + 60*60*1000*starthour;
			TimeValue duration = endmsec + 1000*endsec + 60*1000*endmin + 60*60*1000*endhour - startTime;
			TimeValue sampleTime;
			TimeRecord movieStartTime = { SInt64ToWide(startTime), 1000, 0 };

			if (subOffset != NULL) {
				if(subOffset - subTextOffset - 1 > 0 && duration > 0) //Make sure subtitle is not empty
				{
					err = AddMediaSampleReference(theMedia, subTextOffset - data, 
												  subOffset - subTextOffset - 1, 
												  duration, (SampleDescriptionHandle) textDesc, 
												  1, 0, &sampleTime);
					if (err) goto bail;

					ConvertTimeScale(&movieStartTime, GetMovieTimeScale(theMovie));

					err = InsertMediaIntoTrack(theTrack, movieStartTime.value.lo, sampleTime, duration, fixed1);
					if (err) goto bail;
				}

				// with the subtitle offset, we want the number at the beginning
				// so advance past the newline in the stream
				subOffset += 1;
			}
		}
	}

	if (*firstSubTrack == NULL)
		*firstSubTrack = theTrack;
	else
		SetTrackAlternate(*firstSubTrack, theTrack);

	SetMediaLanguage(theMedia, GetFilenameLanguage(filename));

bail:
	if (err) {
		if (theMedia)
			DisposeTrackMedia(theMedia);

		if (theTrack)
			DisposeMovieTrack(theTrack);
	}

	if (textDesc)
		DisposeHandle((Handle) textDesc);

	if (data)
		DisposePtr(data);

	if (dataHandler)
		CloseComponent(dataHandler);

	return err;
}

ComponentResult LoadVobSubSubtitles(const FSRef *theDirectory, CFStringRef filename, Movie theMovie, Track *firstSubTrack)
{
	ComponentResult err = noErr;
	
	return err;
}

ComponentResult LoadExternalSubtitles(const FSRef *theFile, Movie theMovie)
{
	ComponentResult err = noErr;
	Track firstSubTrack = NULL;
	HFSUniStr255 hfsFilename;
	CFStringRef cfFilename = NULL;
	FSRef parentDir;
	FSIterator dirItr = NULL;
	CFRange baseFilenameRange;
	ItemCount filesFound;
	Boolean containerChanged;

	err = FSGetCatalogInfo(theFile, kFSCatInfoNone, NULL, &hfsFilename, NULL, &parentDir);
	if (err) goto bail;

	// find the location of the extension
	cfFilename = CFStringCreateWithCharacters(NULL, hfsFilename.unicode, hfsFilename.length);
	baseFilenameRange = CFStringFind(cfFilename, CFSTR("."), kCFCompareBackwards);

	// strip the extension
	if (baseFilenameRange.location != kCFNotFound) {
		CFStringRef temp = cfFilename;
		baseFilenameRange.length = baseFilenameRange.location;
		baseFilenameRange.location = 0;
		cfFilename = CFStringCreateWithSubstring(NULL, temp, baseFilenameRange);
		CFRelease(temp);
	}

	err = FSOpenIterator(&parentDir, kFSIterateFlat, &dirItr);
	if (err) goto bail;

	do {
		FSRef foundFileRef;
		HFSUniStr255 hfsFoundFilename;
		CFStringRef cfFoundFilename = NULL;
		CFComparisonResult cmpRes;

		err = FSGetCatalogInfoBulk(dirItr, 1, &filesFound, &containerChanged, kFSCatInfoNone, 
				                   NULL, &foundFileRef, NULL, &hfsFoundFilename);
		if (err) goto bail;

		cfFoundFilename = CFStringCreateWithCharacters(NULL, hfsFoundFilename.unicode, hfsFoundFilename.length);
		cmpRes = CFStringCompareWithOptions(cfFoundFilename, cfFilename, baseFilenameRange, kCFCompareCaseInsensitive);

		// two files share the same base, so now check the extension of the found file
		if (cmpRes == kCFCompareEqualTo) {
			CFRange extRange = { CFStringGetLength(cfFoundFilename) - 4, 4 };
			CFRange actRange;

			// SubRip
			actRange = CFStringFind(cfFoundFilename, CFSTR(".srt"), kCFCompareCaseInsensitive | kCFCompareBackwards);
			if (actRange.location == extRange.location)
				LoadSubRipSubtitles(&parentDir, cfFoundFilename, theMovie, &firstSubTrack);

			// VobSub
			actRange = CFStringFind(cfFoundFilename, CFSTR(".idx"), kCFCompareCaseInsensitive | kCFCompareBackwards);
			if (actRange.location == extRange.location)
				LoadVobSubSubtitles(&foundFileRef, cfFoundFilename, theMovie, &firstSubTrack);
		}

		CFRelease(cfFoundFilename);
	} while (filesFound && !containerChanged);
	
bail:	
	if (err == errFSNoMoreItems)
		err = noErr;

	if (dirItr)
		FSCloseIterator(dirItr);

	if (cfFilename)
		CFRelease(cfFilename);

	return err;
}
