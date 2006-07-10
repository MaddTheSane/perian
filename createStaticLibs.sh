#!/bin/sh
PATH=$PATH:/usr/local/bin:/usr/bin:/sw/bin:/opt/local/bin
buildid_ffmpeg="r`svn info ffmpeg | grep -F Revision | awk '{print $2}'`"

OUTPUT_FILE="$SYMROOT/Universal/buildid"

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
	#######################
	# Intel shlibs
	#######################
	BUILDDIR="$SYMROOT/intel"
	mkdir "$BUILDDIR"
	
	cd "$SRCROOT/ffmpeg"
	patch -p0 < ../ffmpeg-configure-crosscomp.patch
	patch -p0 < ../ffmpeg-svn-mactel.patch
	
	cd "$BUILDDIR"
	if [ `arch` != i386 ] ; then
		"$SRCROOT/ffmpeg/configure" --cross-compile --cpu=x86 --enable-pp --enable-gpl --extra-cflags='-arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk' --extra-ldflags='-isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386'
	else
		"$SRCROOT/ffmpeg/configure" --enable-pp --enable-gpl --enable-memalign-hack
	fi
	make -j3
	
	cd "$SRCROOT/ffmpeg"
	patch -R -p0 < ../ffmpeg-configure-crosscomp.patch
	patch -R -p0 < ../ffmpeg-svn-mactel.patch
	
	
	#######################
	# PPC shlibs
	#######################
	BUILDDIR="$SYMROOT/ppc"
	mkdir "$BUILDDIR"
	
	cd "$SRCROOT/ffmpeg"
	patch -p0 < ../ffmpeg-configure-crosscomp.patch
	
	cd "$BUILDDIR"
	if [ `arch` = ppc ] ; then
		"$SRCROOT/ffmpeg/configure" --enable-pp --enable-gpl
	else
		"$SRCROOT/ffmpeg/configure" --enable-pp --enable-gpl --cpu=ppc  --extra-cflags='-arch ppc -isysroot /Developer/SDKs/MacOSX10.4u.sdk' --extra-ldflags='-isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch ppc'
	fi
	make -j3
	
	cd "$SRCROOT/ffmpeg"
	patch -R -p0 < ../ffmpeg-configure-crosscomp.patch
	
	
	#######################
	# lipo shlibs
	#######################
	BUILDDIR="$SYMROOT/Universal"
	INTEL="$SYMROOT/intel"
	PPC="$SYMROOT/ppc"
	rm -rf "$BUILDDIR"
	mkdir "$BUILDDIR"
	for aa in "$INTEL"/*/*.a ; do
		echo lipo -create $aa `echo -n $aa | sed 's/intel/ppc/'` -output `echo -n $aa | sed 's/intel\/.*\//Universal\//'`
		lipo -create $aa `echo -n $aa | sed 's/intel/ppc/'` -output `echo -n $aa | sed 's/intel\/.*\//Universal\//'`
	done
	echo -n "$buildid_ffmpeg" > $OUTPUT_FILE
fi

