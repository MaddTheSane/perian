#!/bin/sh -v
PATH=$PATH:/usr/local/bin:/usr/bin:/sw/bin:/opt/local/bin

cd "$BUILT_PRODUCTS_DIR"
mkdir "PerianPane.prefPane/Contents/Resources/Components"
for item in Perian A52Codec AC3MovieImport ; do
    itemName="${item}.component"
    if [[ -d $itemName ]] ; then
        zip -9r "${item}.zip" "$itemName"
        cp "${item}.zip" "PerianPane.prefPane/Contents/Resources/Components"
    fi
done