rm -rf build_ninjaPCR;
rm -f MakersToyPCR.zip;
rm -rf build_MakersToyPCR;
cp -r NinjaPCR  build_MakersToyPCR;

# Replace "NinjaPCR" with "Makers Toy PCR"
find ./build_MakersToyPCR -type f -print | xargs sed -i "" "s/NinjaPCR/Makers Toy PCR/g"

zip -r MakersToyPCR.zip build_MakersToyPCR

#open https://chrome.google.com/webstore/developer/dashboard

echo "----------------------------------";

grep "pcr_localize_appTitle" build_MakersToyPCR/PCR.html | gsed -e "s/.*>\(.*\)<.*/\1/g"
grep "version" build_MakersToyPCR/manifest.json | gsed -e "s/.*\"\([0-9.]\+\)\".*/Version: \1/"

echo "----------------------------------";