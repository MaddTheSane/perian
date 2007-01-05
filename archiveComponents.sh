#!/bin/sh -v
PATH=$PATH:/usr/local/bin:/usr/bin:/sw/bin:/opt/local/bin

cd "$BUILT_PRODUCTS_DIR"
mkdir "PerianPane.prefPane/Contents/Resources/Components"
for itemName in *.component ; do
    itemLength=${#itemName}
    item=${itemName:0:$itemLength-10}
    echo $item
    if [[ -d $itemName ]] ; then
        zip -9r "${item}.zip" "$itemName"
        cp "${item}.zip" "PerianPane.prefPane/Contents/Resources/Components"
    fi
done