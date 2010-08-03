#!/bin/sh

# hack to avoid having to configure mplayer just for building the
# xml documentation

doc_lang_all=`echo DOCS/xml/??/ DOCS/xml/??_??/ | sed -e "s:DOCS/xml/\(..\)/:\1:g" -e "s:DOCS/xml/\(.._..\)/:\1:g"`

echo "DOC_LANG_ALL = ${doc_lang_all}"
echo "DOC_LANGS = ${doc_lang_all}"
