rm staticLibs/*
for aa in tmp-i386/* ; do
	lipo -create $aa `echo -n $aa | sed 's/i386/ppc/'` -output `echo -n $aa | sed 's/tmp-i386/staticLibs/'`
done
