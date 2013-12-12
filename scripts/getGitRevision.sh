#!/bin/bash
# Xcode auto-versioning script for Subversion
# by Axel Andersson, modified by Daniel Jalkut to add
# "--revision HEAD" to the svn info line, which allows
# the latest revision to always be used.

# further modified by Augie Fackler to be gross and sh-based in places
# so that you can have svn installed anywhere
#PATH=/sw/bin:/opt/local/bin:/usr/local/bin:/usr/bin:$PATH

function GitRevCount {
	git --git-dir=$1 rev-list HEAD | sort > config.git-hash
	LOCALVER=`wc -l config.git-hash | awk '{print $1}'`

	if [ $LOCALVER \> 1 ] ; then
		VER=`git --git-dir=$1 rev-list $2 | sort | join config.git-hash - | wc -l | awk '{print $1}'`

		if [ $VER != $LOCALVER ] ; then
			VER=$LOCALVER
		fi

	else
		VER="unknown"
	fi
	rm -f config.git-hash
}

GitRevCount "./.git" "origin/lionAndLater"
REV=$VER
GitRevCount "./ffmpeg/.git" "origin/perianSvn"
ffmpeg_rev=$VER
echo $REV

cat > $SCRIPT_OUTPUT_FILE_0.tmp <<EOF
#define GITREVISION $REV
#define GITREVISION_C_STRING "$REV"
#define FFMPEGREVISION $ffmpeg_rev
#define FFMPEGREVISION_C_STRING "$ffmpeg_rev"
EOF

cmp -s $SCRIPT_OUTPUT_FILE_0 $SCRIPT_OUTPUT_FILE_0.tmp || mv $SCRIPT_OUTPUT_FILE_0.tmp $SCRIPT_OUTPUT_FILE_0
