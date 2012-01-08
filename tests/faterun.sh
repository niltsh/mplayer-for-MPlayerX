#!/bin/sh
if [ -z "$FATE_SAMPLES" ] ; then
  echo "FATE_SAMPLES is not set!"
  exit 1
fi

sample="$1"
md5out="tests/res/$sample.md5"
ref_file="tests/ref/$sample.md5"
echo "testing $sample"

# create necessary files and run
mkdir -p $(dirname "$md5out")
touch "$md5out"
./mplayer -noconfig all -lavdopts threads=4:bitexact -really-quiet -noconsolecontrols -nosound -benchmark -vo md5sum:outfile="$md5out" "$FATE_SAMPLES/$sample"

# check result
if ! [ -e "$ref_file" ] ; then
  touch tests/ref/empty.md5
  ref_file=tests/ref/empty.md5
fi
if ! diff -uw "$ref_file" "$md5out" ; then
  mv "$md5out" "$md5out.bad"
  exit 1
fi
