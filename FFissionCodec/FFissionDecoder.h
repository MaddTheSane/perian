/*
 *  FFissionDecoder.h
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

#ifndef __FFISSIONDECODER_H__
#define __FFISSIONDECODER_H__

#include "FFissionCodec.h"
#include "avcodec.h"

class FFissionDecoder : public FFissionCodec
{
public:
	FFissionDecoder(UInt32 inInputBufferByteSize = 76800);
	virtual ~FFissionDecoder();
	
	virtual void Initialize(const AudioStreamBasicDescription* inInputFormat, const AudioStreamBasicDescription* inOutputFormat, const void* inMagicCookie, UInt32 inMagicCookieByteSize);
	
	virtual void GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData);
	virtual void SetMagicCookie(const void* inMagicCookieData, UInt32 inMagicCookieDataByteSize);
	
	virtual void SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat);
	virtual void SetCurrentOutputFormat(const AudioStreamBasicDescription& inOutputFormat);
	virtual UInt32 GetVersion() const;

	virtual UInt32 ProduceOutputPackets(void* outOutputData, UInt32& ioOutputDataByteSize, UInt32& ioNumberPackets, AudioStreamPacketDescription* outPacketDescription);
	
private:
	void SetupExtradata(OSType formatID);
	
	UInt32 kIntPCMOutFormatFlag;
	Byte *magicCookie;
	UInt32 magicCookieSize;
};

#endif
