#!/bin/sh -v
PATH=/usr/local/bin:/sw/bin:/opt/local/bin:/usr/bin:$PWD/Binaries:$PATH
buildid_ffmpeg="r`svn info ffmpeg | grep -F Revision | awk '{print $2}'`"

if [ "$MACOSX_DEPLOYMENT_TARGET" = "" ]; then
	MACOSX_DEPLOYMENT_TARGET="10.4"
fi

generalConfigureOptions="--disable-muxers --disable-encoders --disable-stripping --enable-pthreads --disable-ffmpeg --disable-network --disable-ffplay --disable-ffserver"
cflags="-isysroot $SDKROOT -mmacosx-version-min=$MACOSX_DEPLOYMENT_TARGET -g -Dattribute_deprecated= -w"

if [ "$BUILD_STYLE" = "Development" ] ; then
    generalConfigureOptions="$generalConfigureOptions --disable-optimizations --disable-mmx"
	buildid_ffmpeg="${buildid_ffmpeg}Dev"
else
    optcflags="-falign-loops=16 -fweb -fstrict-aliasing -finline-limit=1000"
	buildid_ffmpeg="${buildid_ffmpeg}Dep"
fi

if what /usr/bin/ld | grep -q ld64-77; then
  no_pic=0
else
  no_pic=1
fi

if [ $no_pic -gt 0 ]; then
    cflags="$cflags -mdynamic-no-pic" # ld can't handle -fno-pic on ppc
else
    #no-pic only on pre-leopard
    echo "warning: Due to issues with Xcode 3.0, Perian will run very slowly! Please upgrade Xcode or fix Patches/ffmpeg.diff!";
    generalConfigureOptions="$generalConfigureOptions --disable-decoder=cavs --disable-decoder=vc1 --disable-decoder=wmv3 --disable-mmx --enable-shared"
fi 

export cflags

x86tune="generic"
if [ "$CC" = "" ]; then
	if [[ -e /usr/bin/gcc-4.2 && -e /Developer/SDKs/MacOSX10.4u.sdk/usr/lib/gcc/i686-apple-darwin9/4.2.1 ]]; then
		CC="gcc-4.2"
		x86tune="core2"
	else
		CC="gcc"
	fi
	export CC
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

	patch -p0 < Patches/ffmpeg-forceinline.diff
	patch -p0 < Patches/ffmpeg-no-interlaced.diff
	patch -p0 < Patches/ffmpeg-faltivec.diff
	patch -p0 < Patches/ffmpeg-h264-nounrollcabac.diff
	patch -p0 < Patches/ffmpeg-no-h264-warning.diff
	touch ffmpeg/patched

    echo -n "Building "
    if [ $buildi386 -eq $buildppc ] ; then
        echo "Universal"
    elif [ $buildi386 -gt 0 ] ; then
        echo "Intel-only"
    else
        echo "PPC-only"
    fi
    
    mkdir "$BUILT_PRODUCTS_DIR"
    #######################
    # Intel shlibs
    #######################
    if [ $buildi386 -gt 0 ] ; then
        BUILDDIR="$BUILT_PRODUCTS_DIR/intel"
        mkdir "$BUILDDIR"

		if [ "$BUILD_STYLE" != "Development" ] ; then
        	optcflags="$optcflags -mtune=$x86tune -frerun-cse-after-loop" 
        fi

        cd "$BUILDDIR"
        if [ "$oldbuildid_ffmpeg" != "quick" ] ; then
            if [ `arch` = ppc ] ; then
                "$SRCROOT/ffmpeg/configure" --cc=$CC --enable-cross-compile --arch=i386 --extra-ldflags="$cflags -arch i386" --extra-cflags="-arch i386 $cflags $optcflags" $extraConfigureOptions $generalConfigureOptions --cpu=pentium-m 
            else
                "$SRCROOT/ffmpeg/configure" --cc=$CC --extra-ldflags="$cflags -arch i386" --extra-cflags="-arch i386 $cflags $optcflags" $extraConfigureOptions $generalConfigureOptions --cpu=pentium-m
            fi
        
            make depend > /dev/null 2>&1 || true
        fi
        if [ "$BUILD_STYLE" = "Development" ] ; then
            cd libavcodec
            export CFLAGS="-O1 -fomit-frame-pointer -funit-at-a-time"; make h264.o cabac.o h264_parser.o motion_est.o
            unset CFLAGS;
            cd ..
        fi
        make -j3
    fi
    
    #######################
    # PPC shlibs
    #######################
    if [ $buildppc -gt 0 ] ; then
        BUILDDIR="$BUILT_PRODUCTS_DIR/ppc"
        mkdir "$BUILDDIR"

		if [ "$BUILD_STYLE" != "Development" ] ; then
       		optcflags="$optcflags -mcpu=G3 -mtune=G5 -funroll-loops -mmultiple"
       	fi
    
        cd "$BUILDDIR"
        if [ "$oldbuildid_ffmpeg" != "quick" ] ; then
            if [ `arch` = ppc ] ; then
                "$SRCROOT/ffmpeg/configure" --cc=$CC --extra-cflags="-faltivec $cflags $optcflags" $extraConfigureOptions $generalConfigureOptions
            else
                "$SRCROOT/ffmpeg/configure" --cc=$CC --enable-cross-compile --arch=ppc  --extra-ldflags="$cflags -arch ppc" --extra-cflags="-faltivec -arch ppc $cflags $optcflags" $extraConfigureOptions $generalConfigureOptions
            fi
        
            make depend > /dev/null 2>&1 || true
        fi
        make -j3
    fi

	#######################
	# lipo/copy shlibs
	#######################
	BUILDDIR="$BUILT_PRODUCTS_DIR/Universal"
	INTEL="$BUILT_PRODUCTS_DIR/intel"
	PPC="$BUILT_PRODUCTS_DIR/ppc"
	
	rm -rf "$BUILDDIR"
	mkdir "$BUILDDIR"
	echo $buildi386 $buildppc
	if [ $buildi386 -eq $buildppc ] ; then
		# lipo them
		for aa in "$INTEL"/*/*.a ; do
			echo lipo -create $aa `echo -n $aa | sed 's/intel/ppc/'` -output `echo -n $aa | sed 's/intel\/.*\//Universal\//'`
			lipo -create $aa `echo -n $aa | sed 's/intel/ppc/'` -output `echo -n $aa | sed 's/intel\/.*\//Universal\//'`
		done
	else
		if [ $buildppc -gt 0 ] ; then
			archDir="ppc"
			BUILDARCHDIR=$PPC
		else
			archDir="intel"
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
	mkdir "$SYMROOT/Universal" || true
	cp "$BUILT_PRODUCTS_DIR/Universal"/* "$SYMROOT/Universal"
	ranlib "$SYMROOT/Universal"/*.a
fi