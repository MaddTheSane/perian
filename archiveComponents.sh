#!/bin/sh -v
PATH=$PATH:/usr/local/bin:/usr/bin:/sw/bin:/opt/local/bin

function compressComponents(){
mkdir -p "$2"
cd $1
for itemName in *.component ; do
    itemLength=${#itemName}
    item=${itemName:0:$itemLength-10}
#    echo $item
    if [[ -d $itemName ]] ; then
        ditto -c -k --rsrc --keepParent "$itemName" "${item}.zip"
        cp "${item}.zip" "$2"
    fi
done
}


compressComponents "${BUILT_PRODUCTS_DIR}" "${BUILT_PRODUCTS_DIR}/PerianPane.prefPane/Contents/Resources/Components/"
if [[ -d "${BUILT_PRODUCTS_DIR}/CoreAudio" ]] ; then
    compressComponents "${BUILT_PRODUCTS_DIR}/CoreAudio" "${BUILT_PRODUCTS_DIR}/PerianPane.prefPane/Contents/Resources/Components/CoreAudio"
fi
if [[ -d "${BUILT_PRODUCTS_DIR}/QuickTime" ]] ; then
    compressComponents "${BUILT_PRODUCTS_DIR}/QuickTime" "${BUILT_PRODUCTS_DIR}/PerianPane.prefPane/Contents/Resources/Components/QuickTime"
fi