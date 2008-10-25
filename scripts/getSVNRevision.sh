# Xcode auto-versioning script for Subversion
# by Axel Andersson, modified by Daniel Jalkut to add
# "--revision HEAD" to the svn info line, which allows
# the latest revision to always be used.

# further modified by Augie Fackler to be gross and sh-based in places
# so that you can have svn installed anywhere
PATH=$PATH:/usr/local/bin:/usr/bin:/sw/bin:/opt/local/bin
ffmpeg_rev=`svnversion -n ./ffmpeg/`
REV=`svnversion -n ./`
echo $REV

cat > $SCRIPT_OUTPUT_FILE_0 <<EOF
#define SVNREVISION $REV
#define SVNREVISION_C_STRING \"$REV\"
#define FFMPEGREVISION $ffmpeg_rev
#define FFMPEGREVISION_C_STRING \"$ffmpeg_rev\"
EOF