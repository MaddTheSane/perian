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

enum {
	// these are atoms/extension types defined by XiphQT for their codecs
	kCookieTypeOggSerialNo = 'oCtN',
	
	kCookieTypeVorbisHeader = 'vCtH',
	kCookieTypeVorbisComments = 'vCt#',
	kCookieTypeVorbisCodebooks = 'vCtC',
	kCookieTypeVorbisFirstPageNo = 'vCtN',

	kCookieTypeSpeexHeader = 'sCtH',
	kCookieTypeSpeexComments = 'sCt#',
	kCookieTypeSpeexExtraHeader	= 'sCtX',

	kCookieTypeTheoraHeader = 'tCtH',
	kCookieTypeTheoraComments = 'tCt#',
	kCookieTypeTheoraCodebooks = 'tCtC',

	kCookieTypeFLACStreaminfo = 'fCtS',
	kCookieTypeFLACMetadata = 'fCtM',

	kTheoraDescExtension = 'XdxT',
	
	
	// contains same as MKV's codec private for real video
	kRealVideoExtension = 'RVex',
	
	// contains the .idx file for vobsub (codec private in mkv)
	kVobSubIdxExtension = '.IDX',
	
	// contains a single byte equal to the ContentCompAlgo element in matroska
	kMKVCompressionExtension = 'MCmp',
	// contains the compression settings equal to ContentCompSettings in matroska
	kCompressionSettingsExtension = 'CpSt',
	kCompressionAlgorithm = 'CpAl',
};


enum {
	// XiphQT's codec IDs
	kAudioFormatXiphVorbis                  = 'XiVs',
	kAudioFormatXiphSpeex                   = 'XiSp',
	kAudioFormatXiphFLAC                    = 'XiFL',
	kVideoFormatXiphTheora                  = 'XiTh',
	
	// the samples for these codec IDs are OGG pages containing the actual codec data
	kAudioFormatXiphOggFramedVorbis         = 'XoVs',
	kAudioFormatXiphOggFramedSpeex          = 'XoSp',
	kAudioFormatXiphOggFramedFLAC           = 'XoFL',
	
	// these are 'ms' + the wav/avi twocc
	kAudioFormatAC3MS                       = 0x6D732000,
	kAudioFormatWMA1MS                      = 0x6D730160,
	kAudioFormatWMA2MS                      = 0x6D730161,
	
	kVideoFormatMSMPEG4v3                   = 'MP43',
	kMPEG1VisualCodecType                   = 'MPEG',
	kMPEG2VisualCodecType                   = 'MPG2',
	kVideoFormatReal5                       = 'RV10',
	kVideoFormatRealG2                      = 'RV20',
	kVideoFormatReal8                       = 'RV30',
	kVideoFormatReal9                       = 'RV40',
	kVideoFormatSnow                        = 'SNOW',
	kVideoFormatIndeo2                      = 'RT21',
	kVideoFormatIndeo3                      = 'IV32',
	kVideoFormatVP8                         = 'VP80',
	kVideoFormatTheora                      = 'PeTh',
	
	kAudioFormatFlashADPCM                  = 'FlAd',
	kAudioFormatDTS                         = 'DTS ', 
	kAudioFormatTTA                         = 'TTA1',
	kAudioFormatWavepack                    = 'WVPK',
	kAudioFormatReal1                       = '14_4',
	kAudioFormatReal2                       = '28_8',
	kAudioFormatRealCook                    = 'COOK',
	kAudioFormatRealSipro                   = 'SIPR',
	kAudioFormatRealLossless                = 'RALF',
	kAudioFormatRealAtrac3                  = 'ATRC',
	kAudioFormatNellymoser					= 'NELL',
	
	kSubFormatUTF8                          = 'SRT ',
	kSubFormatSSA                           = 'SSA ',
	kSubFormatASS                           = 'ASS ',
	kSubFormatUSF                           = 'USF ',
	kSubFormatVobSub                        = 'SPU ',
	
	kCompressedAVC1							= 'CAVC',
	kCompressedMP4V							= 'CMP4',
	kCompressedAC3							= 'CAC3',
	kCompressedMP1							= 'CMP1',
	kCompressedMP2							= 'CMP2',
	kCompressedMP3							= 'CMP3',
	kCompressedDTS							= 'CDTS',
};

#ifndef REZ
int getCodecID(OSType componentType);
pascal ComponentResult getPerianCodecInfo(ComponentInstance self, OSType componentType, void *info);
#endif

#endif
