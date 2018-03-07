#!/bin/sh -v
PATH=$PWD/Binaries:$PATH
buildid_ffmpeg="r`git --git-dir=./ffmpeg/.git rev-parse HEAD`"

if [ "$MACOSX_DEPLOYMENT_TARGET" = "" ]; then
	MACOSX_DEPLOYMENT_TARGET="10.6"
fi

CC=`xcrun -find clang`

configureflags="--cc=$CC --disable-amd3dnow --disable-doc --disable-encoders \
    --disable-muxers --disable-network \
    --disable-avfilter --disable-programs --disable-ffmpeg --disable-ffprobe \
    --target-os=darwin"

cflags="-isysroot $SDKROOT -mmacosx-version-min=$MACOSX_DEPLOYMENT_TARGET -Dattribute_deprecated= -fvisibility=hidden -w"

if [ "$BUILD_STYLE" = "Debug" -o "$CONFIGURATION" = "Debug" ] ; then
    configureflags="$configureflags --disable-optimizations --disable-asm"
	buildid_ffmpeg="${buildid_ffmpeg}Dev"
else
    optcflags="-fstrict-aliasing"
	buildid_ffmpeg="${buildid_ffmpeg}Dep"
fi

BUILD_ID_FILE="$BUILT_PRODUCTS_DIR/Universal/buildid"

if [[ -e "$BUILD_ID_FILE" ]] ; then
    oldbuildid_ffmpeg=`cat "$BUILD_ID_FILE"`
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

if [ `echo $ARCHS | grep -c i386` -gt 0 ] ; then
   buildi386=1
   buildppc=0
else
    echo "No valid architectures"
    exit 0
fi

if [ "$buildid_ffmpeg" = "$oldbuildid_ffmpeg" ] ; then
    echo "Static ffmpeg libs are up-to-date ; not rebuilding"
else
    echo "Static ffmpeg libs are out-of-date ; rebuilding"
    
    if [ -e ffmpeg/.git ]; then
    if [ -e ffmpeg/patched ] ; then
		cd ffmpeg && git reset --hard && rm patched && cd ..
	fi

	cd ffmpeg
	#patch -p1 < ../Patches/0002-Workaround-for-AVI-audio-tracks-importing-1152x-too-.patch
    #patch -p1 < ../Patches/config.patch
    #patch -p1 < ../Patches/ignore-clock-gettime.patch
    patch -p1 < ../Patches/cxx11Quiet.patch
	cd ..

	touch ffmpeg/patched
	fi

    echo "Building i386"
    
    mkdir -p "$BUILT_PRODUCTS_DIR"

    arch=`arch`
    # files we'd like to keep frame pointers in for in-the-wild debugging
    fptargets="libavformat/libavformat.a libavutil/libavutil.a libavcodec/utils.o"
	
    #######################
    # Intel shlibs
    #######################
    if [ $buildi386 -gt 0 ] ; then
        BUILDDIR="$BUILT_PRODUCTS_DIR/i386"
        mkdir -p "$BUILDDIR"

		if [ "$BUILD_STYLE" != "Debug" ] ; then
        	optcflags_i386="$optcflags -mdynamic-no-pic $x86flags"
        fi

        cd "$BUILDDIR"
        if [ "$oldbuildid_ffmpeg" != "quick" ] ; then
            "$SRCROOT/ffmpeg/configure" --extra-ldflags="$cflags -arch i386" \
            --cpu=core2 --extra-cflags="-arch i386 $cflags $optcflags_i386 $OTHER_CFLAGS" \
            $configureflags || exit 1
        
            make depend > /dev/null 2>&1 || true
        fi
        
        fpcflags=`grep -m 1 -e "^CFLAGS=" "$BUILDDIR"/config.mak | sed -e s/CFLAGS=// -e s/-fomit-frame-pointer//` 

        make -j3 CFLAGS="$fpcflags" V=1 $fptargets || exit 1
        make -j3 V=1 || exit 1
    fi

	#######################
	# lipo/copy shlibs
	#######################
	BUILDDIR="$BUILT_PRODUCTS_DIR/Universal"
	INTEL="$BUILT_PRODUCTS_DIR/i386"
	PPC="$BUILT_PRODUCTS_DIR/ppc"
	
	rm -rf "$BUILDDIR"
	mkdir "$BUILDDIR"

	if [ $buildi386 -eq $buildppc ] ; then
		# lipo them
		for aa in "$INTEL"/*/*.a ; do
			echo lipo -create -arch i386 $aa -arch ppc `/bin/echo -n $aa | sed 's/i386/ppc/'` \
			-output `echo -n $aa | sed 's/i386\/.*\//Universal\//'`
			lipo -create -arch i386 $aa -arch ppc `/bin/echo -n $aa | sed 's/i386/ppc/'` \
			-output `echo -n $aa | sed 's/i386\/.*\//Universal\//'`
		done
	else
		if [ $buildppc -gt 0 ] ; then
			archDir="ppc"
			BUILDARCHDIR=$PPC
		else
			archDir="i386"
			BUILDARCHDIR=$INTEL
		fi
		# just copy them
		for aa in "$BUILDARCHDIR"/*/*.a ; do
			echo cp "$aa" `/bin/echo -n $aa | sed 's/'$archDir'\/.*\//Universal\//'`
			cp "$aa" `/bin/echo -n $aa | sed 's/'$archDir'\/.*\//Universal\//'`
		done
	fi
	/bin/echo -n "$buildid_ffmpeg" > $BUILD_ID_FILE
fi

FINAL_BUILD_ID_FILE="$BUILT_PRODUCTS_DIR/Universal/buildid"
if [[ -e "$FINAL_BUILD_ID_FILE" ]] ; then
    oldbuildid_ffmpeg=`cat "$FINAL_BUILD_ID_FILE"`
else
    oldbuildid_ffmpeg="buildme"
fi
