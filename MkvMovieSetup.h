/*
 *  MkvMovieSetup.h
 *
 *    MkvMovieSetup.h - Functions to create the track structure of a Matroska file as a QuickTime movie
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

#ifndef __MKVMOVIESETUP_H__
#define __MKVMOVIESETUP_H__

#include "MatroskaImport.h"

#include <matroska/KaxTracks.h>
#include <matroska/KaxInfo.h>
#include <matroska/KaxChapters.h>

using namespace libmatroska;

ComponentResult MkvSetupTracks(KaxTracks *tracks, MkvSegment &segment, 
							   Movie theMovie, Handle dataRef, OSType dataRefType);
ComponentResult MkvParseSegmentInfo(MatroskaImportGlobals globals, KaxInfo *segmentInfo);
ComponentResult ReadSegmentInfo(KaxInfo *segmentInfo, struct MkvSegment *segment, Movie theMovie);
ComponentResult SetupChapters(KaxChapters *chapters, KaxPrivate *priv, MkvSegment *segment);

#endif