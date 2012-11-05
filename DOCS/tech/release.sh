#!/bin/bash
# will create checkouts and tarballs in the current dir
ver=1.1
dst=MPlayer-$ver
svnurl=svn://svn.mplayerhq.hu/mplayer/trunk
#svnurl=svn://svn.mplayerhq.hu/mplayer/branches/$ver

rm -rf $dst/ $dst-DOCS/ $dst.tar*

svn export $svnurl $dst/
# branches should contain a VERSION file, but
# "previews" generated from trunk need it.
if ! test -e $dst/VERSION ; then
  echo $ver > $dst/VERSION
fi

# create HTML docs
cp -a $dst/ $dst-DOCS/
pushd $dst-DOCS/
mkdir -p ffmpeg/libavutil
./configure --yasm='' --language=all
make html-chunked
popd
mv $dst-DOCS/DOCS/HTML $dst/DOCS
rm -rf $dst-DOCS/

# git archive unfortunately is refused
git clone --depth 1 git://source.ffmpeg.org/ffmpeg.git $dst/ffmpeg
rm -rf $dst/.git* $dst/ffmpeg/.git*

# create .tar.gz and .tar.xz files
tar --owner=0 --group=0 -cf $dst.tar $dst/
xz -k $dst.tar
gzip -9 $dst.tar

# generate checksums/signatures
md5sum $dst.tar.xz > $dst.tar.xz.md5
sha1sum $dst.tar.xz > $dst.tar.xz.sha1
md5sum $dst.tar.gz > $dst.tar.gz.md5
sha1sum $dst.tar.gz > $dst.tar.gz.sha1
gpg -a --detach-sign $dst.tar.xz
gpg -a --detach-sign $dst.tar.gz
