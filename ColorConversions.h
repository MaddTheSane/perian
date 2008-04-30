/*
 *  ColorConversions.h
 *  Perian
 *
 *  Created by Alexander Strange on 1/10/07.
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA */

#include <Carbon/Carbon.h>
#include "avcodec.h"

extern void FastY420(UInt8 *baseAddr, AVFrame *picture);
extern void Y420toY422(UInt8* baseAddr, unsigned rowBytes, unsigned width, unsigned height, AVFrame * picture);
extern void RGB32toRGB32(UInt8 *baseAddr, unsigned rowBytes, unsigned width, unsigned height, AVFrame *picture);
extern void RGB24toRGB24(UInt8 *baseAddr, unsigned rowBytes, unsigned width, unsigned height, AVFrame *picture);
extern void BGR24toRGB24(UInt8 *baseAddr, unsigned rowBytes, unsigned width, unsigned height, AVFrame *picture);
extern void Y422toY422(UInt8* o, unsigned outRB, unsigned width, unsigned height, AVFrame * picture);
