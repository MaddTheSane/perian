#!/bin/sh
PATH=$PATH:/usr/local/bin:/usr/bin:/sw/bin:/opt/local/bin
buildid_ffmpeg="r`svn info ffmpeg | grep -F Revision | awk '{print $2}'`"

if [ "$BUILD_STYLE" = "Development" ] ; then
	extraConfigureOptions="--disable-strip --disable-opts --disable-mmx --disable-altivec"
else
	extraConfigureOptions="--enable-small"
fi

OUTPUT_FILE="$BUILT_PRODUCTS_DIR/Universal/buildid"

if [[ -e "$OUTPUT_FILE" ]] ; then
	oldbuildid_ffmpeg=`cat "$OUTPUT_FILE"`
else
	oldbuildid_ffmpeg="buildme"
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
	
	cd "$SRCROOT/ffmpeg"
	
	cd "$BUILDDIR"
	if [ `arch` != i386 ] ; then
		"$SRCROOT/ffmpeg/configure" --cross-compile --cpu=x86 --enable-pp --enable-gpl --extra-ldflags='-arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk' --extra-cflags='-arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk' $extraConfigureOptions
	else
		"$SRCROOT/ffmpeg/configure" --enable-pp --enable-gpl --enable-memalign-hack $extraConfigureOptions
	fi
	make -j3
	
	cd "$SRCROOT/ffmpeg"
	
	
	#######################
	# PPC shlibs
	#######################
	BUILDDIR="$BUILT_PRODUCTS_DIR/ppc"
	mkdir "$BUILDDIR"
	
	cd "$SRCROOT/ffmpeg"
	
	cd "$BUILDDIR"
	if [ `arch` = ppc ] ; then
		"$SRCROOT/ffmpeg/configure" --enable-pp --enable-gpl $extraConfigureOptions
	else
		"$SRCROOT/ffmpeg/configure" --enable-pp --enable-gpl --cpu=ppc  --extra-ldflags='-arch ppc -isysroot /Developer/SDKs/MacOSX10.4u.sdk' --extra-cflags='-arch ppc -isysroot /Developer/SDKs/MacOSX10.4u.sdk' $extraConfigureOptions
	fi
	make -j3
	
	cd "$SRCROOT/ffmpeg"

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
ranlib "$SYMROOT/Universal"/*.a
