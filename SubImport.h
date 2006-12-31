/*
 *  SubImport.h
 *  Perian
 *
 *  Created by David Conrad on 10/12/06.
 *  Copyright 2006 Perian Project. All rights reserved.
 *
 */

#ifndef __SUBIMPORT_H__
#define __SUBIMPORT_H__

#include <QuickTime/QuickTime.h>
#include "CodecIDs.h"

#ifdef __cplusplus
extern "C"
{
#endif

ComponentResult LoadExternalSubtitles(const FSRef *theFile, Movie theMovie);

Track CreatePlaintextSubTrack(Movie theMovie, ImageDescriptionHandle imgDesc, 
                              TimeScale timescale, Handle dataRef, OSType dataRefType);

#ifdef __cplusplus
}
#endif

#endif
