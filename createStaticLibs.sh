#!/bin/sh -v
PATH=$PATH:/usr/local/bin:/usr/bin:/sw/bin:/opt/local/bin
buildid_ffmpeg="r`svn info ffmpeg | grep -F Revision | awk '{print $2}'`"

if [ "$MACOSX_DEPLOYMENT_TARGET" = "" ]; then
	MACOSX_DEPLOYMENT_TARGET="10.4"
fi

generalConfigureOptions="--disable-muxers --disable-strip --enable-pthreads --disable-ffmpeg --disable-network --disable-ffplay --disable-vhook"
sdkflags="-isysroot $SDKROOT -mmacosx-version-min=$MACOSX_DEPLOYMENT_TARGET -gstabs+ -Dattribute_deprecated="

if [ "$BUILD_STYLE" = "Development" ] ; then
    generalConfigureOptions="$generalConfigureOptions --disable-optimizations --disable-mmx"
fi

ver=$(uname -r)
if (( ${ver:0:1} < 9 )); then
    #no-pic only on pre-leopard
    sdkflags="$sdkflags -mdynamic-no-pic"
fi
export sdkflags

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
        	export optCFlags="-mtune=nocona -fstrict-aliasing -frerun-cse-after-loop -fweb -falign-loops=16" 
        fi

        cd "$BUILDDIR"
        if [ "$oldbuildid_ffmpeg" != "quick" ] ; then
            if [ `arch` = ppc ] ; then
                "$SRCROOT/ffmpeg/configure" --cross-compile --arch=i386 --extra-ldflags='-arch i386' --extra-cflags='-arch i386 $sdkflags $optCFlags' $extraConfigureOptions $generalConfigureOptions --cpu=pentium-m 
            else
                "$SRCROOT/ffmpeg/configure" --extra-cflags='$sdkflags $optCFlags' $extraConfigureOptions $generalConfigureOptions --cpu=pentium-m
            fi
        
            make depend > /dev/null 2>&1 || true
        fi
        if [ "$BUILD_STYLE" = "Development" ] ; then
            cd libavcodec
            export CFLAGS="-O1 -fomit-frame-pointer -funit-at-a-time"; make h264.o cabac.o h264_parser.o motion_est.o
            unset CFLAGS;
            cd ..
        fi
        make -j3            lib
    fi
    
    #######################
    # PPC shlibs
    #######################
    if [ $buildppc -gt 0 ] ; then
        BUILDDIR="$BUILT_PRODUCTS_DIR/ppc"
        mkdir "$BUILDDIR"

		if [ "$BUILD_STYLE" != "Development" ] ; then
       		export optCFlags="-mcpu=G3 -mtune=G5 -fstrict-aliasing -funroll-loops -falign-loops=16 -mmultiple"
       	fi
    
        cd "$BUILDDIR"
        if [ "$oldbuildid_ffmpeg" != "quick" ] ; then
            if [ `arch` = ppc ] ; then
                "$SRCROOT/ffmpeg/configure" --extra-cflags='$sdkflags $optCFlags' $extraConfigureOptions $generalConfigureOptions
            else
                "$SRCROOT/ffmpeg/configure" --cross-compile --arch=ppc  --extra-ldflags='-arch ppc' --extra-cflags='-arch ppc $sdkflags $optCFlags' $extraConfigureOptions $generalConfigureOptions
            fi
        
            make depend > /dev/null 2>&1 || true
        fi
        make -j3            lib
    fi
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
echo -n "$buildid_ffmpeg" > $OUTPUT_FILE

mkdir "$SYMROOT/Universal" || true
cp "$BUILT_PRODUCTS_DIR/Universal"/* "$SYMROOT/Universal"
if [ "$BUILD_STYLE" = "Deployment" ] ; then
    strip -S "$SYMROOT/Universal"/*.a
fi
ranlib "$SYMROOT/Universal"/*.a
