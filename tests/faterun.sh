#!/bin/sh
i=$1
echo "running $i"
mkdir -p res/$(dirname $i)
touch res/$i.md5
../mplayer -noconfig all -lavdopts threads=4:bitexact -really-quiet -noconsolecontrols -nosound -benchmark -vo md5sum:outfile=res/$i.md5 $FATE_SAMPLES/$i
ref_file=ref/$i.md5
if ! [ -e $ref_file ] ; then
  touch ref/empty.md5
  ref_file=ref/empty.md5
fi
if ! diff -uw $ref_file res/$i.md5 ; then
  mv res/$i.md5 res/$i.md5.bad
  exit 1
fi
