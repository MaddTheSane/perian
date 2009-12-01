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

#define kPerianBaseVersion			(0x00000004)
#define kFFusionCodecVersion		(kPerianBaseVersion+0x00030002)
#define kTextSubCodecVersion		kPerianBaseVersion
#define kVobSubCodecVersion			kPerianBaseVersion
#define kFFissionCodecVersion		kFFusionCodecVersion
#define kMatroskaImportVersion		(kPerianBaseVersion+0x00020001)
#define kFFAviComponentVersion		(kPerianBaseVersion+0x00010003)
		
enum{
include(<!codecList.m4!>)
};
