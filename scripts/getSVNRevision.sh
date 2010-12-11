# Xcode auto-versioning script for Subversion
# by Axel Andersson, modified by Daniel Jalkut to add
# "--revision HEAD" to the svn info line, which allows
# the latest revision to always be used.

# further modified by Augie Fackler to be gross and sh-based in places
# so that you can have svn installed anywhere
PATH=/sw/bin:/opt/local/bin:/usr/local/bin:/usr/bin:$PATH
ffmpeg_rev=`svnversion -n ./ffmpeg/`
REV=`svnversion -n ./`
echo $REV

cat > $SCRIPT_OUTPUT_FILE_0.tmp <<EOF
#define SVNREVISION $REV
#define SVNREVISION_C_STRING "$REV"
#define FFMPEGREVISION $ffmpeg_rev
#define FFMPEGREVISION_C_STRING "$ffmpeg_rev"
EOF

cmp -s $SCRIPT_OUTPUT_FILE_0 $SCRIPT_OUTPUT_FILE_0.tmp || mv $SCRIPT_OUTPUT_FILE_0.tmp $SCRIPT_OUTPUT_FILE_0
