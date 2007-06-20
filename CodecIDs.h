/*
 *  CodecIDs.h
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

#ifndef __CODECIDS_H__
#define __CODECIDS_H__

// Description Extensions
enum {
    kCookieTypeOggSerialNo = 'oCtN'
};

// format specific cookie types
enum {
    kCookieTypeVorbisHeader = 'vCtH',
    kCookieTypeVorbisComments = 'vCt#',
    kCookieTypeVorbisCodebooks = 'vCtC',
	kCookieTypeVorbisFirstPageNo = 'vCtN'
};

enum {
    kCookieTypeSpeexHeader = 'sCtH',
    kCookieTypeSpeexComments = 'sCt#',
    kCookieTypeSpeexExtraHeader	= 'sCtX'
};

enum {
    kCookieTypeTheoraHeader = 'tCtH',
    kCookieTypeTheoraComments = 'tCt#',
    kCookieTypeTheoraCodebooks = 'tCtC'
};

enum {
    kCookieTypeFLACStreaminfo = 'fCtS',
    kCookieTypeFLACMetadata = 'fCtM'
};

enum {
	kSampleDescriptionExtensionTheora = 'XdxT',
	kSampleDescriptionExtensionVobSubIdx = '.IDX',
	kSampleDescriptionExtensionReal = 'RVex',
	kSampleDescriptionExtensionMKVCompression = 'Comp',
};


enum {
	// unofficial QuickTime FourCCs
    kAudioFormatXiphVorbis                  = 'XiVs',
    kAudioFormatXiphSpeex                   = 'XiSp',
    kAudioFormatXiphFLAC                    = 'XiFL',
    kVideoFormatXiphTheora                  = 'XiTh',
	kAudioFormatAC3MS                       = 0x6D732000,
	kAudioFormatWMA1MS                      = 0x6D730160,
	kAudioFormatWMA2MS                      = 0x6D730161,
	kAudioFormatFlashADPCM                  = 'FlAd',
	kVideoFormatMSMPEG4v3                   = 'MP43',
	kMPEG1VisualCodecType                   = 'MPEG',
	kMPEG2VisualCodecType                   = 'MPG2',
	
	kSubFormatUTF8                          = 'SRT ',
	kSubFormatSSA                           = 'SSA ',
	kSubFormatASS                           = 'ASS ',
	kSubFormatUSF                           = 'USF ',
	kSubFormatVobSub                        = 'SPU ',
	
	// the following 4CCs don't have decoder support yet
	kAudioFormatDTS                         = 'DTS ', 
	kAudioFormatTTA                         = 'TTA1',
	kAudioFormatWavepack                    = 'WVPK',
	kVideoFormatReal5                       = 'RV10',
	kVideoFormatRealG2                      = 'RV20',
	kVideoFormatReal8                       = 'RV30',
	kVideoFormatReal9                       = 'RV40',
	kAudioFormatReal1                       = '14_4',
	kAudioFormatReal2                       = '28_8',
	kAudioFormatRealCook                    = 'COOK',
	kAudioFormatRealSipro                   = 'SIPR',
	kAudioFormatRealLossless                = 'RALF',
	kAudioFormatRealAtrac3                  = 'ATRC'
};

#endif
