#!/bin/sh
# Check help message header files.

CHECK=checkhelp

trap "rm -f ${CHECK}.c ${CHECK}.o" EXIT

CC=$1
shift

for h in "$@"; do
  cat <<EOF > ${CHECK}.c
#include <inttypes.h>
#include <string.h>
#include "config.h"
#include "$h"
void $CHECK () {
EOF
  sed -n "s:^[ \t]*#define[ \t]\+\([0-9A-Za-z_]\+\)[ \t].*:strdup(\1);:p" "$h" >> ${CHECK}.c
  echo "}" >> ${CHECK}.c
  $CC -Werror -c -o ${CHECK}.o ${CHECK}.c || exit
done
