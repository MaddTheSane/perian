#!/bin/sh -v
PATH=$PATH:/usr/local/bin:/usr/bin:/sw/bin:/opt/local/bin
buildid_ffmpeg="r`svn info ffmpeg | grep -F Revision | awk '{print $2}'`"

generalConfigureOptions="--disable-encoders --disable-muxers --disable-strip --enable-pthreads --disable-ffmpeg --disable-network --disable-ffplay --disable-vhook"

if [ "$BUILD_STYLE" = "Development" ] ; then
	generalConfigureOptions="$generalConfigureOptions --disable-opts"
fi

OUTPUT_FILE="$BUILT_PRODUCTS_DIR/Universal/buildid"

if [[ -e "$OUTPUT_FILE" ]] ; then
	oldbuildid_ffmpeg=`cat "$OUTPUT_FILE"`
else
	oldbuildid_ffmpeg="buildme"
fi

QUICKBUILD="$BUILT_PRODUCTS_DIR/Universal/quickbuild"
if [[ -e "$QUICKBUILD" ]] ; then
    oldbuildid_ffmpeg="quick"
    rm "$QUICKBUILD"
fi

if [[ $buildid == "r" ]] ; then
	echo "error: you're using svk. Please ask someone to add svk support to the build system. There's a script in Adium svn that can do this."
	exit 1;
fi

if [ "$buildid_ffmpeg" = "$oldbuildid_ffmpeg" ] ; then
	echo "Static ffmpeg libs are up-to-date ; not rebuilding"
else
	echo "Static ffmpeg libs are out-of-date ; rebuilding"
	
	mkdir "$BUILT_PRODUCTS_DIR"
	#######################
	# Intel shlibs
	#######################
	BUILDDIR="$BUILT_PRODUCTS_DIR/intel"
	mkdir "$BUILDDIR"
	
	export optCFlags="-mtune=nocona -fstrict-aliasing -frerun-cse-after-loop -fweb -gstabs+ -mdynamic-no-pic" 

	cd "$BUILDDIR"
	if [ "$oldbuildid_ffmpeg" != "quick" ] ; then
        if [ `arch` != i386 ] ; then
            "$SRCROOT/ffmpeg/configure" --cross-compile --arch=x86_32 --extra-ldflags='-arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk' --extra-cflags='-arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk $optCFlags' $extraConfigureOptions $generalConfigureOptions --cpu=pentium-m 
        else
            "$SRCROOT/ffmpeg/configure" $extraConfigureOptions $generalConfigureOptions --cpu=pentium-m --extra-cflags='$optCFlags'
        fi
        
        make depend > /dev/null 2>&1 || true
    fi
    if [ "$BUILD_STYLE" = "Development" ] ; then
        cd libavcodec
        export CFLAGS="-O1 -fomit-frame-pointer -funit-at-a-time"; make h264.o cabac.o
        unset CFLAGS;
        cd ..
    fi
    make -j3            lib
	
	
	#######################
	# PPC shlibs
	#######################
	BUILDDIR="$BUILT_PRODUCTS_DIR/ppc"
	mkdir "$BUILDDIR"
	
	export optCFlags="-mcpu=G3 -mtune=G5 -fstrict-aliasing -funroll-loops -mmultiple -gstabs+ -mdynamic-no-pic"
	
	cd "$BUILDDIR"
	if [ "$oldbuildid_ffmpeg" != "quick" ] ; then
        if [ `arch` = ppc ] ; then
            "$SRCROOT/ffmpeg/configure" $extraConfigureOptions $generalConfigureOptions --extra-cflags='$optCFlags'
        else
            "$SRCROOT/ffmpeg/configure" --cross-compile --arch=ppc  --extra-ldflags='-arch ppc -isysroot /Developer/SDKs/MacOSX10.4u.sdk' --extra-cflags='-arch ppc -isysroot /Developer/SDKs/MacOSX10.4u.sdk $optCFlags' $extraConfigureOptions $generalConfigureOptions
        fi
        
        make depend > /dev/null 2>&1 || true
    fi
    make -j3            lib
	
	#######################
	# lipo shlibs
	#######################
	BUILDDIR="$BUILT_PRODUCTS_DIR/Universal"
	INTEL="$BUILT_PRODUCTS_DIR/intel"
	PPC="$BUILT_PRODUCTS_DIR/ppc"
	rm -rf "$BUILDDIR"
	mkdir "$BUILDDIR"
	for aa in "$INTEL"/*/*.a ; do
		echo lipo -create $aa `echo -n $aa | sed 's/intel/ppc/'` -output `echo -n $aa | sed 's/intel\/.*\//Universal\//'`
		lipo -create $aa `echo -n $aa | sed 's/intel/ppc/'` -output `echo -n $aa | sed 's/intel\/.*\//Universal\//'`
	done
	echo -n "$buildid_ffmpeg" > $OUTPUT_FILE
fi

mkdir "$SYMROOT/Universal" || true
cp "$BUILT_PRODUCTS_DIR/Universal"/* "$SYMROOT/Universal"
if [ "$BUILD_STYLE" = "Deployment" ] ; then
	strip -S "$SYMROOT/Universal"/*.a
fi
ranlib "$SYMROOT/Universal"/*.a
