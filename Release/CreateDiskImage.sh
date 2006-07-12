#dmgSize=${DiskImageSizeInMB}
#dmgBasePath="${TARGET_BUILD_DIR}/${DiskImageVolumeName}"
dmgSize=8
BUILT_PRODUCTS_DIR=build/Deployment
DiskImageProduct=Perian.component
dmgBasePath=build/Perian
DiskImageVolumeName="Perian Codec"

if [ x${ACTION} = xclean ]; then
	echo "Removing disk image ${dmgPath}.dmg"
	rm -f "${dmgBasePath}.dmg"
	exit 0
fi

echo "Creating ${dmgSize} MB disk image named '${DiskImageVolumeName}'..."
rm -f "${dmgBasePath}.dmg"
hdiutil create "${dmgBasePath}.dmg" -volname "${DiskImageVolumeName}" -megabytes ${dmgSize} -layout NONE -fs HFS+ -quiet
if [ $? != 0 ]; then
	echo error:0: Failed to create disk image at ${dmgBasePath}.dmg
	exit 1
fi
echo "...done"
echo

echo "Mounting newly created disk image..."
hdidOutput=`hdiutil mount "${dmgBasePath}.dmg" | grep '/dev/disk[0-9]*' | awk '{print $1}'`
mountedDmgPath="/Volumes/${DiskImageVolumeName}"
if [ $? != 0  -o  ! -x "${mountedDmgPath}" ]; then
	echo error:0: Failed to mount newly created disk image at ${dmgBasePath}.dmg
	exit 1
fi
sleep 2
echo "...done"
echo

echo "Copying contents to ${dmgPath}..."
#cp "${SOURCE_ROOT}/Fire-README.txt" "${mountedDmgPath}"
#cp "${SOURCE_ROOT}/GPL" "${mountedDmgPath}"
ditto -rsrc "${BUILT_PRODUCTS_DIR}/${DiskImageProduct}" "${mountedDmgPath}/${DiskImageProduct}"
strip -u -r "${mountedDmgPath}/${DiskImageProduct}/Contents/MacOS/Fire"
mkdir "${mountedDmgPath}/Pictures"
cp "${SOURCE_ROOT}/scripts/daily-img/camp_fire.jpg" "${mountedDmgPath}/Pictures"
cp "${SOURCE_ROOT}/scripts/daily-img/dsstore" "${mountedDmgPath}/.DS_Store"
echo "...done"
echo
echo "${mountedDmgPath}"

echo "Configuring folder properties..."
osascript -e "tell application \"Finder\"" \
          -e "    set mountedDiskImage to disk \"${DiskImageVolumeName}\"" \
          -e "    open mountedDiskImage" \
          -e "    tell container window of mountedDiskImage" \
          -e "        set toolbar visible to false" \
          -e "        set current view to icon view" \
          -e "        set bounds to {${DiskImageWindowMinX}, ${DiskImageWindowMinY}, ${DiskImageWindowMaxX}, ${DiskImageWindowMaxY}}" \
          -e "        set position of file \"Fire-README.txt\" to {0, 64}" \
          -e "        set position of folder \"Pictures\" to {0, 64}" \
          -e "        set position of file \"${DiskImageProduct}\" to {(${DiskImageWindowMaxX} - ${DiskImageWindowMinX}) / 2 - 64, ${DiskImageWindowMaxY} - ${DiskImageWindowMinY} - 90}" \
          -e "        set position of file \"GPL\" to {${DiskImageWindowMaxX} - ${DiskImageWindowMinX} - 155, 64}" \
          -e "        set opts to icon view options of container window of mountedDiskImage" \
          -e "        set background picture of opts to file \"camp_fire.jpg\" of folder \"Pictures\"" \
          -e "    end tell" \
          -e "end tell" \
          > /dev/null
#          -e "    set icon size of icon view options of container window of mountedDiskImage to 128" \
#          -e "    set background color of icon view options of container window of mountedDiskImage to { ${DiskImageWindowBackgroundRed} * 65535, ${DiskImageWindowBackgroundGreen} * 65535, ${DiskImageWindowBackgroundBlue} * 65535 }" \
echo "...done"
echo

mv "${mountedDmgPath}/Pictures" "${mountedDmgPath}/.Pictures"

osascript -e 'tell application "Xcode" to display dialog "Eject the image through Finder to keep background image"'

#echo "Unmounting disk image..."
#hdiutil eject -quiet ${hdidOutput}
#echo "...done"
#echo

echo "Compressing disk image..."
mv "${dmgBasePath}.dmg" "${dmgBasePath}-orig.dmg"
hdiutil convert "${dmgBasePath}-orig.dmg" -format UDZO -o "${dmgBasePath}"
if [ $? != 0 ]; then
	echo error:0: Failed to compress newly created disk image at ${dmgBasePath}.dmg
	exit 1
fi
rm "${dmgBasePath}-orig.dmg"
echo "...done"
echo

osascript -e "tell application \"Finder\"" -e "select posix file \"${TARGET_BUILD_DIR}/${DiskImageVolumeName}.dmg\"" -e "end tell" > /dev/null

svnrev=`svn info "${SOURCE_ROOT}" | grep 'Revision' | awk '{print $2}'`
echo "Creating 1${dmgSize} MB disk image named '${DiskImageVolumeName}'..."
rm -f "${dmgBasePath}_${svnrev}.dmg"
hdiutil create "${dmgBasePath}_${svnrev}.dmg" -volname "${DiskImageVolumeName}" -megabytes 1${dmgSize} -layout NONE -fs HFS+ -quiet
if [ $? != 0 ]; then
	echo error:0: Failed to create disk image at ${dmgBasePath}_${svnrev}.dmg
	exit 1
fi
echo "...done"
echo

echo "Mounting newly created disk image..."
hdidOutput=`hdiutil mount "${dmgBasePath}_${svnrev}.dmg" | grep '/dev/disk[0-9]*' | awk '{print $1}'`
mountedDmgPath="/Volumes/${DiskImageVolumeName}"
if [ $? != 0  -o  ! -x "${mountedDmgPath}" ]; then
	echo error:0: Failed to mount newly created disk image at ${dmgBasePath}_${svnrev}.dmg
	exit 1
fi
sleep 2
echo "...done"
echo

echo "Copying contents to ${dmgPath}_${svnrev}..."
cd "${BUILT_PRODUCTS_DIR}/"
#tricky regex to reject those files within Fire.app/Contents/Resources and reject the mdimporter
find -E . -iregex "\./[^/]+[^r]/Contents/MacOS/[^/]*" -type f -perm +111 -exec cp {} "${mountedDmgPath}/" \;
#tricky regex to reject those frameworks within Fire.app/Contents/Frameworks
find -E . -iregex "\./[^/]+\.framework/.*" -type f -perm +111 -exec cp {} "${mountedDmgPath}/" \;
echo "...done"
echo
echo "${mountedDmgPath}"

echo "Unmounting disk image..."
hdiutil eject -quiet ${hdidOutput}
echo "...done"
echo

echo "Compressing disk image..."
mv "${dmgBasePath}_${svnrev}.dmg" "${dmgBasePath}_${svnrev}-orig.dmg"
hdiutil convert "${dmgBasePath}_${svnrev}-orig.dmg" -format UDZO -o "${dmgBasePath}_${svnrev}"
if [ $? != 0 ]; then
	echo error:0: Failed to compress newly created disk image at ${dmgBasePath}_${svnrev}.dmg
	exit 1
fi
rm "${dmgBasePath}_${svnrev}-orig.dmg"
echo "...done"
echo
exit 0
