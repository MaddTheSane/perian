#!/bin/sh -v
PATH=/usr/local/bin:/sw/bin:/opt/local/bin:/usr/bin:$PWD/Binaries:$PATH
buildid_ffmpeg="r`svn info ffmpeg | grep -F Revision | awk '{print $2}'`"

if what /usr/bin/ld | grep -q ld64-77; then
    echo "Xcode 3.1 is required to build Perian";
    exit 1
fi 

x86tune="generic"
x86flags=""

if [ -e /usr/bin/gcc-4.2 ]; then
	CC="${DEVELOPER_BIN_DIR}/gcc-4.2"
	x86tune="core2"
	x86flags="--param max-completely-peel-times=2"
fi

if [ "$MACOSX_DEPLOYMENT_TARGET" = "" ]; then
	MACOSX_DEPLOYMENT_TARGET="10.4"
fi

configureflags="--cc=$CC --disable-amd3dnow --disable-doc --disable-encoders \
     --disable-ffprobe --disable-ffserver --disable-muxers --disable-network \
     --disable-swscale --disable-avfilter --target-os=darwin"

cflags="-isysroot $SDKROOT -mmacosx-version-min=$MACOSX_DEPLOYMENT_TARGET \
        -D__DARWIN_UNIX03=0 -Dattribute_deprecated= -w"

if [ "$BUILD_STYLE" = "Development" ] ; then
    configureflags="$configureflags --disable-optimizations --disable-asm"
	buildid_ffmpeg="${buildid_ffmpeg}Dev"
else
    optcflags="-fweb -fstrict-aliasing -finline-limit=1000 -freorder-blocks"
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
   if [ `echo $ARCHS | grep -c ppc` -gt 0 ] ; then
       buildppc=1
   else
       buildppc=0
   fi
elif [ `echo $ARCHS | grep -c ppc` -gt 0 ] ; then
   buildi386=0
   buildppc=1
else
    echo "No architectures"
    exit 0
fi

if [ "$buildid_ffmpeg" = "$oldbuildid_ffmpeg" ] ; then
    echo "Static ffmpeg libs are up-to-date ; not rebuilding"
else
    echo "Static ffmpeg libs are out-of-date ; rebuilding"
    
    if [ -e ffmpeg/patched ] ; then
		cd ffmpeg && svn revert -R . && rm patched && cd ..
	fi

	cd ffmpeg
	patch -p1 < ../Patches/0001-Disable-some-parts-of-h264.c-Perian-never-uses.patch
	patch -p1 < ../Patches/0002-Use-faltivec-instead-of-maltivec.patch
	patch -p1 < ../Patches/0003-Remove-the-warning-Cannot-parallelize-deblocking-typ.patch
	patch -p1 < ../Patches/0004-Hardcode-results-of-runtime-cpu-detection-in-dsputil.patch
	patch -p1 < ../Patches/0005-Double-INTERNAL_BUFFER_SIZE-to-fix-running-out-of-bu.patch
	patch -p1 < ../Patches/0006-Workaround-for-AVI-audio-tracks-importing-1152x-too-.patch
	cd ..

	touch ffmpeg/patched

    echo -n "Building "
    if [ $buildi386 -eq $buildppc ] ; then
        echo "Universal"
    elif [ $buildi386 -gt 0 ] ; then
        echo "Intel-only"
    else
        echo "PPC-only"
    fi
    
    mkdir -p "$BUILT_PRODUCTS_DIR"
    mkdir -p "$SYMROOT/Universal"

    arch=`arch`
    # files we'd like to keep frame pointers in for in-the-wild debugging
    fptargets="libavformat/libavformat.a libavutil/libavutil.a libavcodec/utils.o"
	
    #######################
    # Intel shlibs
    #######################
    if [ $buildi386 -gt 0 ] ; then
        BUILDDIR="$BUILT_PRODUCTS_DIR/i386"
        mkdir -p "$BUILDDIR"

		if [ "$BUILD_STYLE" != "Development" ] ; then
        	optcflags_i386="$optcflags -mdynamic-no-pic -mtune=$x86tune $x86flags" 
        fi

        cd "$BUILDDIR"
        if [ "$oldbuildid_ffmpeg" != "quick" ] ; then
            if [ "$arch" = ppc ] ; then
                "$SRCROOT/ffmpeg/configure" --enable-cross-compile --arch=i386 \
                --cpu=pentium-m --extra-ldflags="$cflags -arch i386" \
                --extra-cflags="-arch i386 $cflags $optcflags_i386" $configureflags || exit 1
            else
                "$SRCROOT/ffmpeg/configure" --extra-ldflags="$cflags -arch i386" \
                --cpu=pentium-m --extra-cflags="-arch i386 $cflags $optcflags_i386" \
                $configureflags || exit 1
            fi
        
            make depend > /dev/null 2>&1 || true
        fi
        
        fpcflags=`grep -m 1 CFLAGS= "$BUILDDIR"/config.mak | sed -e s/CFLAGS=// -e s/-fomit-frame-pointer//` 

        make -j3 CFLAGS="$fpcflags" $fptargets
        make -j3 || exit 1
    fi
    
    #######################
    # PPC shlibs
    #######################
    if [ $buildppc -gt 0 ] ; then
        BUILDDIR="$BUILT_PRODUCTS_DIR/ppc"
        mkdir -p "$BUILDDIR"

		if [ "$BUILD_STYLE" != "Development" ] ; then
       		optcflags_ppc="$optcflags -mdynamic-no-pic -mcpu=G3 -mtune=G5"
       	fi
    
        cd "$BUILDDIR"
        if [ "$oldbuildid_ffmpeg" != "quick" ] ; then
            if [ "$arch" = ppc ] ; then
                "$SRCROOT/ffmpeg/configure" --extra-cflags="-faltivec $cflags \
                $optcflags_ppc" $configureflags || exit 1
            else
                "$SRCROOT/ffmpeg/configure" --enable-cross-compile --arch=ppc \
                --extra-ldflags="$cflags -arch ppc" --extra-cflags="-faltivec \
                -arch ppc $cflags $optcflags_ppc" $configureflags || exit 1
            fi
        
            make depend > /dev/null 2>&1 || true
        fi
        
        fpcflags=`grep -m 1 CFLAGS= "$BUILDDIR"/config.mak | sed -e s/CFLAGS=// -e s/-fomit-frame-pointer//` 

        make -j3 CFLAGS="$fpcflags" $fptargets
        make -j3 || exit 1
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
			echo lipo -create -arch i386 $aa -arch ppc `echo -n $aa | sed 's/i386/ppc/'` \
			-output `echo -n $aa | sed 's/i386\/.*\//Universal\//'`
			lipo -create -arch i386 $aa -arch ppc `echo -n $aa | sed 's/i386/ppc/'` \
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
			echo cp "$aa" `echo -n $aa | sed 's/'$archDir'\/.*\//Universal\//'`
			cp "$aa" `echo -n $aa | sed 's/'$archDir'\/.*\//Universal\//'`
		done
	fi
	echo -n "$buildid_ffmpeg" > $BUILD_ID_FILE
fi

FINAL_BUILD_ID_FILE="$SYMROOT/Universal/buildid"
if [[ -e "$FINAL_BUILD_ID_FILE" ]] ; then
    oldbuildid_ffmpeg=`cat "$FINAL_BUILD_ID_FILE"`
else
    oldbuildid_ffmpeg="buildme"
fi

if [ "$buildid_ffmpeg" = "$oldbuildid_ffmpeg" ] ; then
    echo "Final static ffmpeg libs are up-to-date ; not copying"
else
	cp "$BUILT_PRODUCTS_DIR/Universal"/* "$SYMROOT/Universal"
	ranlib "$SYMROOT/Universal"/*.a
fi