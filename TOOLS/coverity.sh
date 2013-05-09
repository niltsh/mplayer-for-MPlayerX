rm -rf cov-int MPlayer.tgz
make distclean
svn up
./configure $MPLAYER_COV_OPTS && make -j5 ffmpeglibs
"$MPLAYER_COV_PATH"/bin/cov-build --dir cov-int make -j5
tar -czf MPlayer.tgz cov-int
curl --form file=@MPlayer.tgz --form project=MPlayer --form password="$MPLAYER_COV_PWD" --form email="$MPLAYER_COV_EMAIL" --form version=2.5 --form description="automated run" http://scan5.coverity.com/cgi-bin/upload.py

