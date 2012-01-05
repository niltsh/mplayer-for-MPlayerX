#!/bin/sh
i=$1
echo "running $i"
mkdir -p tests/res/$(dirname $i)
touch tests/res/$i.md5
./mplayer -noconfig all -lavdopts threads=4:bitexact -really-quiet -noconsolecontrols -nosound -benchmark -vo md5sum:outfile=tests/res/$i.md5 $FATE_SAMPLES/$i
ref_file=tests/ref/$i.md5
if ! [ -e $ref_file ] ; then
  touch tests/ref/empty.md5
  ref_file=tests/ref/empty.md5
fi
if ! diff -uw $ref_file tests/res/$i.md5 ; then
  mv tests/res/$i.md5 tests/res/$i.md5.bad
  exit 1
fi
