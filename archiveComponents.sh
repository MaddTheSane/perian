#!/bin/sh -v
PATH=$PATH:/usr/local/bin:/usr/bin:/sw/bin:/opt/local/bin

function compressAndInsertComponent(){
itemName="$1"
itemLength=${#itemName}
item=${itemName:0:$itemLength-10}
if [[ -d $itemName ]] ; then
    ditto -c -k --rsrc --keepParent "$itemName" "${item}.zip"
    cp "${item}.zip" "$2"
fi
}

function compressComponents(){
mkdir -p "$2"
cd $1
for itemName in *.component ; do
    compressAndInsertComponent $itemName $2
done
}

cd "${BUILT_PRODUCTS_DIR}"
mkdir -p "${BUILT_PRODUCTS_DIR}/Perian.prefPane/Contents/Resources/Components/"
compressAndInsertComponent "Perian.component" "${BUILT_PRODUCTS_DIR}/Perian.prefPane/Contents/Resources/Components/"
if [[ -d "${BUILT_PRODUCTS_DIR}/CoreAudio" ]] ; then
    compressComponents "${BUILT_PRODUCTS_DIR}/CoreAudio" "${BUILT_PRODUCTS_DIR}/Perian.prefPane/Contents/Resources/Components/CoreAudio"
fi
if [[ -d "${BUILT_PRODUCTS_DIR}/QuickTime" ]] ; then
    compressComponents "${BUILT_PRODUCTS_DIR}/QuickTime" "${BUILT_PRODUCTS_DIR}/Perian.prefPane/Contents/Resources/Components/QuickTime"
fi
