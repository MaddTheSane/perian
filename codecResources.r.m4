include(`defines.m4')
define(<!printTypes!>, <!foreach(<!X!>, <!
<!#!>define kCodecSubType X
<!#include "PerianResources.r"!>!>, $@)!>
)dnl
dnl
define(<!Codec!>, <!<!#define kCodecInfoResID $1!>
ifelse(lastCodecID, $1, , <!#define kCodecName $3!>)
define(<!lastCodecID!>, $1)
<!#define kCodecDescription $4!>dnl
printTypes(shift(shift(shift(shift($@)))))!>dnl
)dnl
dnl
define(<!EntryPoint!>, <!resource 'dlle' ($3) {
        $4
};!>
define(<!ResourceOnly!>, <!$@!>)dnl

<!#define kCodecManufacturer $1!>
<!#define kCodecVersion $2!>
<!#define kEntryPointID $3!>
<!#define kDecompressionFlags $5!>
<!#define kFormatFlags $6!>
)dnl
<!#define TARGET_REZ_CARBON_MACHO 1!>
<!#define thng_RezTemplateVersion 1	// multiplatform 'thng' resource!>
<!#define cfrg_RezTemplateVersion 1	// extended version of 'cfrg' resource!>
<!!>
<!#include <Carbon/Carbon.r>!>
<!#include <QuickTime/QuickTime.r>!>
<!#include "PerianResourceIDs.h"!>
<!#define kStartTHNGResID 128!>
include(<!codecList.m4!>)