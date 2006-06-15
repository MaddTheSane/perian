
#ifndef __EI_IMAGE__
#define __EI_IMAGE__

#if __APPLE_CC__
    #include <QuickTime/Movies.h>
#else
    #include <Movies.h>
#endif

#define imageCreator	'EIAD'
#define imageType		'EIDI'

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
	#pragma pack(2)
#endif

typedef struct {
	UInt16 imageVersion;						/* Image file version (5) */
	UInt32 imageFrames;							/* Number of frames in the file (1..?) */
} ImageHeader, *ImageHeaderPtr;

typedef struct {
	QTFloatSingle frameTime;					/* Time of frame (0.0) */
	Rect frameRect;								/* Frame Rectangle */
	UInt8 frameBitDepth;						/* Bits Per Pixel (not including alpha) */
	UInt8 frameType;							/* Pixel Type (0=Direct; 1=Indexed) */
	Rect framePackRect;							/* Packing rectangle */
	UInt8 framePacking;							/* Packing Mode (0=Not Packed; 1=RL Encoding) */
	UInt8 frameAlpha;							/* Alpha Bits per pixel */
	UInt32 frameSize;							/* Size in bytes of the body of the image */
	UInt16 framePalettes;						/* Number of entries in the color table (1..256) */
	UInt16 frameBackground;						/* The index of the background color (0) */
} ImageFrame, *ImageFramePtr;

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
	#pragma pack()
#endif

#endif