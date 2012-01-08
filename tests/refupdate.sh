#!/bin/sh
# updates all changed/new results in ref/
find res -name '*.bad' | while read bad_res ; do
  ref_file="ref/${bad_res#res/}"
  ref_file="${ref_file%.bad}"
  mkdir -p "$(dirname "$ref_file")"
  cp "$bad_res" "$ref_file"
done
