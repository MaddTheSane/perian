/*
 *  MkvExportPrivate.h
 *
 *    MkvExportPrivate.h - Utility functions for interfacing with libmatroska to export.
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

#ifndef __MKVEXPORTPRIVATE_H__
#define __MKVEXPORTPRIVATE_H__

#include <ebml/c/libebml_t.h>
#include <ebml/IOCallback.h>
#include <ebml/EbmlHead.h>
#include <ebml/EbmlSubHead.h>

#include <matroska/KaxConfig.h>
#include <matroska/KaxSegment.h>

using namespace libmatroska;


ComponentResult WriteHeader(MatroskaExportGlobals glob);
ComponentResult WriteTrackInfo(MatroskaExportGlobals glob);
ComponentResult StartNewCluster(MatroskaExportGlobals glob, TimeValue64 atTime);
ComponentResult WriteFrame(MatroskaExportGlobals glob);
ComponentResult FinishFile(MatroskaExportGlobals glob);

#endif