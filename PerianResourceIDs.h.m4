include(`defines.m4')

define(<!Codec!>, <!ifelse($1, lastCase, ,<!define(<!lastCase!>, $1)	<!$1!>ifdef(<!StartResourceID!>, <! = StartResourceID,
undefine(<!StartResourceID!>)dnl!>, <!,!>)!>)!>)dnl
dnl
define(<!EntryPoint!>, <!!>)dnl
define(<!ResourceOnly!>, <!!>)dnl
define(<!StartResourceID!>, <!128!>)dnl

#import "CodecIDs.h"

#define kPerianManufacturer		'Peri'

#define kFFusionCodecManufacturer	kPerianManufacturer
#define kTextCodecManufacturer		kPerianManufacturer
#define kVobSubCodecManufacturer	kPerianManufacturer
#define kFFissionCodecManufacturer	kPerianManufacturer

#define kFFusionCodecVersion		(0x00030005)
#define kTextSubCodecVersion		(0x00020003)
#define kVobSubCodecVersion			(0x00020003)
#define kFFissionCodecVersion		kFFusionCodecVersion

enum{
include(<!codecList.m4!>)
};
