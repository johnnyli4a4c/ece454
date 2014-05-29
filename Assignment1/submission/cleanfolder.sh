mkdir tmp

mv buildsampleapps.sh tmp/
mv cleanfolder.sh tmp/
mv ece454a1.zip tmp/
mv unpack.sh tmp/

rm *.*
rm Makefile

mv tmp/*.* ./
rm -r tmp