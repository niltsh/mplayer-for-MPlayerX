#!/bin/sh
if [ -z "$FATE_SAMPLES" ] ; then
  echo "FATE_SAMPLES is not set!"
  exit 1
fi

sample="$1"
md5out="tests/res/$sample.md5"
ref_file="tests/ref/$sample.md5"
options="-noconfig all -lavdopts threads=4:bitexact:idct=2 -really-quiet -noconsolecontrols -nosound -benchmark"
if [ -z ${sample##h264-conformance/*} ] ; then
  # these files generally only work when a fps is given explicitly
  options="$options -fps 25"
fi
echo "testing $sample"

# create necessary files and run
mkdir -p $(dirname "$md5out")
touch "$md5out"
./mplayer $options -vo md5sum:outfile="$md5out" "$FATE_SAMPLES/$sample"

# check result
if ! [ -e "$ref_file" ] ; then
  touch tests/ref/empty.md5
  ref_file=tests/ref/empty.md5
fi
if ! diff -uw "$ref_file" "$md5out" ; then
  mv "$md5out" "$md5out.bad"
  exit 1
fi
