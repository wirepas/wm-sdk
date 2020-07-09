#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# Change to parent folder of this script
cd $DIR/..

set -e

# Acquire SHA-1 of the current codebase. ${ZUUL_COMMIT} contains this info but this 
# allows executing script locally without zuul/jenkins dependencies
SHA1=$(git rev-parse HEAD)

# Run code analysis

CSVCHECK=$DIR/csvcheck.sh
PMD=/opt/pmd-bin-5.4.1/bin/run.sh
# Define here the excluded folders!
PMD_EXCLUDES="--exclude ./build --exclude ./image --exclude ./mcu --exclude ./source/ruuvi_evk --exclude ./util/tinycbor"
LIZARD_EXCLUDES='--exclude "./build/*" --exclude "./image/*" --exclude "./mcu/*" --exclude "./source/ruuvi_evk/*" --exclude "./util/tinycbor/*"'
CPPCHECK_EXCLUDES="-i build -i image -i mcu -i source/ruuvi_evk -i util/tinycbor"

echo Using these commands for code analysis:
echo ===================================================================
echo cppcheck --force --enable=warning $CPPCHECK_EXCLUDES .
echo lizard -w $LIZARD_EXCLUDES
echo $PMD cpd --minimum-tokens 50 --language cpp --format csv --files . $PMD_EXCLUDES
echo ===================================================================

cppcheck --force --enable=warning $CPPCHECK_EXCLUDES . 2> cppcheck.txt
# Create csv file from violations
echo cppcheck > cppcheck.csv
wc -l < cppcheck.txt >> cppcheck.csv

# Execute lizard complexity measurement
eval lizard -w $LIZARD_EXCLUDES > lizard.txt || true
sort lizard.txt -o lizard.txt

# Create csv file from violations
echo  lizard > lizard.csv
wc -l < lizard.txt >> lizard.csv

# Create copy paste detector report
$PMD cpd --minimum-tokens 50 --language cpp --format csv --files . $PMD_EXCLUDES > pmd.txt || true
echo pmd > pmd.csv
wc -l < pmd.txt >> pmd.csv

# Revert code to the previous version
rm -rf old/
mkdir old
# git checkout origin/master
git checkout HEAD~1

cppcheck --force --enable=warning $CPPCHECK_EXCLUDES . 2> old/cppcheck.txt
# Create csv file from violations
echo cppcheck > old/cppcheck.csv
wc -l < old/cppcheck.txt >> old/cppcheck.csv

# Execute lizard complexity measurement
eval lizard -w $LIZARD_EXCLUDES > old/lizard.txt || true
sort old/lizard.txt -o old/lizard.txt

# Create csv file from violations
echo  lizard > old/lizard.csv
wc -l < old/lizard.txt >> old/lizard.csv

# Create copy paste detector report
$PMD cpd --minimum-tokens 50 --language cpp --format csv --files . $PMD_EXCLUDES > old/pmd.txt || true

echo pmd > old/pmd.csv
wc -l < old/pmd.txt >> old/pmd.csv

# Test that cppcheck value has not been increased (too much)
echo "Validating cppcheck violations. If this fails, examine cppcheck.html"
$DIFF2HTML old/cppcheck.txt cppcheck.txt >cppcheck.html
if [ -f "$CSVCHECK" ];then
  $CSVCHECK cppcheck.csv old/cppcheck.csv 1.0
fi

echo "Validating lizard (code complexity) violations. If this fails, examine lizard.html"
$DIFF2HTML old/lizard.txt lizard.txt >lizard.html
if [ -f "$CSVCHECK" ];then
  $CSVCHECK lizard.csv old/lizard.csv 1.0
fi

echo "Validating copy-paste code amount. If this fails, examine pmd.html"
$DIFF2HTML old/pmd.txt pmd.txt >pmd.html
if [ -f "$CSVCHECK" ];then
  $CSVCHECK pmd.csv old/pmd.csv 1.0
fi

# Reverting back to newest code
git checkout ${SHA1}

# Revert to folder that was used earlier
cd $PWD
