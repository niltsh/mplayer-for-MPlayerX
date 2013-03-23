# MPlayer Makefile
#
# copyright (c) 2008 Diego Biurrun
# Rewritten entirely from a set of Makefiles written by Arpi and many others.
#
# This file is part of MPlayer.
#
# MPlayer is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# MPlayer is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with MPlayer; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

include config.mak

###### variable declarations #######

# local fallbacks for missing operating system features
OS_FEATURE-$(GETTIMEOFDAY)           += osdep/gettimeofday.c
OS_FEATURE-$(GLOB_WIN)               += osdep/glob-win.c
OS_FEATURE-$(MMAP)                   += osdep/mmap-os2.c
OS_FEATURE-$(SETENV)                 += osdep/setenv.c
OS_FEATURE-$(SHMEM)                  += osdep/shmem.c
OS_FEATURE-$(STRSEP)                 += osdep/strsep.c
OS_FEATURE-$(VSSCANF)                += osdep/vsscanf.c

# conditional source declarations
SRCS_AUDIO_INPUT-$(ALSA)             += stream/ai_alsa.c
SRCS_AUDIO_INPUT-$(OSS)              += stream/ai_oss.c
SRCS_COMMON-$(AUDIO_INPUT)           += $(SRCS_AUDIO_INPUT-yes)
SRCS_COMMON-$(BITMAP_FONT)           += sub/font_load.c
SRCS_COMMON-$(CDDA)                  += stream/stream_cdda.c            \
                                        stream/cdinfo.c
SRCS_COMMON-$(CDDB)                  += stream/stream_cddb.c
SRCS_COMMON-$(DVBIN)                 += stream/dvb_tune.c               \
                                        stream/stream_dvb.c
SRCS_COMMON-$(DVDNAV)                += stream/stream_dvdnav.c
SRCS_COMMON-$(DVDNAV_INTERNAL)       += libdvdnav/dvdnav.c              \
                                        libdvdnav/highlight.c           \
                                        libdvdnav/navigation.c          \
                                        libdvdnav/read_cache.c          \
                                        libdvdnav/remap.c               \
                                        libdvdnav/searching.c           \
                                        libdvdnav/settings.c            \
                                        libdvdnav/vm/decoder.c          \
                                        libdvdnav/vm/vm.c               \
                                        libdvdnav/vm/vmcmd.c            \

SRCS_COMMON-$(DVDREAD)               += stream/stream_dvd.c             \
                                        stream/stream_dvd_common.c
SRCS_COMMON-$(DVDREAD_INTERNAL)      += libdvdread4/bitreader.c         \
                                        libdvdread4/dvd_input.c         \
                                        libdvdread4/dvd_reader.c        \
                                        libdvdread4/dvd_udf.c           \
                                        libdvdread4/ifo_print.c         \
                                        libdvdread4/ifo_read.c          \
                                        libdvdread4/md5.c               \
                                        libdvdread4/nav_print.c         \
                                        libdvdread4/nav_read.c          \

SRCS_COMMON-$(FAAD)                  += libmpcodecs/ad_faad.c
SRCS_COMMON-$(FASTMEMCPY)            += libvo/aclib.c
SRCS_COMMON-$(FFMPEG)                += av_helpers.c                    \
                                        av_opts.c                       \
                                        libaf/af_lavcac3enc.c           \
                                        libaf/af_lavcresample.c         \
                                        libmpcodecs/ad_ffmpeg.c         \
                                        libmpcodecs/ad_spdif.c          \
                                        libmpcodecs/vd_ffmpeg.c         \
                                        libmpcodecs/vf_geq.c            \
                                        libmpcodecs/vf_lavc.c           \
                                        libmpcodecs/vf_lavcdeint.c      \
                                        libmpcodecs/vf_screenshot.c     \
                                        libmpdemux/demux_lavf.c         \
                                        stream/stream_ffmpeg.c          \
                                        sub/av_sub.c                    \

SRCS_COMMON-$(CONFIG_VF_LAVFI)      +=  libmpcodecs/vf_lavfi.c

# These filters use private headers and do not work with shared FFmpeg.
SRCS_COMMON-$(FFMPEG_A)              += libmpcodecs/vf_fspp.c           \
                                        libmpcodecs/vf_mcdeint.c        \
                                        libmpcodecs/vf_qp.c             \
                                        libmpcodecs/vf_spp.c            \
                                        libmpcodecs/vf_uspp.c           \

SRCS_COMMON-$(FREETYPE)              += sub/font_load_ft.c
SRCS_COMMON-$(FTP)                   += stream/stream_ftp.c
SRCS_COMMON-$(GIF)                   += libmpdemux/demux_gif.c
SRCS_COMMON-$(HAVE_POSIX_SELECT)     += libmpcodecs/vf_bmovl.c
SRCS_COMMON-$(HAVE_SYS_MMAN_H)       += libaf/af_export.c osdep/mmap_anon.c
SRCS_COMMON-$(JPEG)                  += libmpcodecs/vd_ijpg.c
SRCS_COMMON-$(LADSPA)                += libaf/af_ladspa.c
SRCS_COMMON-$(LIBA52)                += libmpcodecs/ad_liba52.c
SRCS_COMMON-$(LIBASS)                += libmpcodecs/vf_ass.c            \
                                        sub/ass_mp.c                    \
                                        sub/subassconvert.c             \

SRCS_COMMON-$(LIBASS_INTERNAL)       += libass/ass.c                    \
                                        libass/ass_bitmap.c             \
                                        libass/ass_cache.c              \
                                        libass/ass_drawing.c            \
                                        libass/ass_font.c               \
                                        libass/ass_fontconfig.c         \
                                        libass/ass_library.c            \
                                        libass/ass_parse.c              \
                                        libass/ass_render.c             \
                                        libass/ass_render_api.c         \
                                        libass/ass_shaper.c             \
                                        libass/ass_strtod.c             \
                                        libass/ass_utils.c              \

SRCS_COMMON-$(LIBBLURAY)             += stream/stream_bluray.c
SRCS_COMMON-$(LIBBS2B)               += libaf/af_bs2b.c
SRCS_COMMON-$(LIBDCA)                += libmpcodecs/ad_libdca.c
SRCS_COMMON-$(LIBDV)                 += libmpcodecs/ad_libdv.c          \
                                        libmpcodecs/vd_libdv.c          \
                                        libmpdemux/demux_rawdv.c
SRCS_COMMON-$(LIBDVDCSS_INTERNAL)    += libdvdcss/css.c                 \
                                        libdvdcss/device.c              \
                                        libdvdcss/error.c               \
                                        libdvdcss/ioctl.c               \
                                        libdvdcss/libdvdcss.c           \

SRCS_COMMON-$(LIBMAD)                += libmpcodecs/ad_libmad.c

SRCS_COMMON-$(LIBMPEG2)              += libmpcodecs/vd_libmpeg2.c
SRCS_LIBMPEG2-INTERNAL-$(ARCH_ALPHA) += libmpeg2/idct_alpha.c           \
                                        libmpeg2/motion_comp_alpha.c
SRCS_LIBMPEG2-INTERNAL-$(ARCH_ARM)   += libmpeg2/motion_comp_arm.c      \
                                        libmpeg2/motion_comp_arm_s.S
SRCS_LIBMPEG2-INTERNAL-$(HAVE_ALTIVEC) += libmpeg2/idct_altivec.c       \
                                        libmpeg2/motion_comp_altivec.c
SRCS_LIBMPEG2-INTERNAL-$(HAVE_MMX)   += libmpeg2/idct_mmx.c             \
                                        libmpeg2/motion_comp_mmx.c
SRCS_LIBMPEG2-INTERNAL-$(HAVE_VIS)   += libmpeg2/motion_comp_vis.c
SRCS_COMMON-$(LIBMPEG2_INTERNAL)     += libmpeg2/alloc.c                \
                                        libmpeg2/cpu_accel.c            \
                                        libmpeg2/cpu_state.c            \
                                        libmpeg2/decode.c               \
                                        libmpeg2/header.c               \
                                        libmpeg2/idct.c                 \
                                        libmpeg2/motion_comp.c          \
                                        libmpeg2/slice.c                \
                                        $(SRCS_LIBMPEG2-INTERNAL-yes)

SRCS_COMMON-$(LIBNEMESI)             += libmpdemux/demux_nemesi.c       \
                                        stream/stream_nemesi.c
SRCS_COMMON-$(LIBNUT)                += libmpdemux/demux_nut.c
SRCS_COMMON-$(LIBSMBCLIENT)          += stream/stream_smb.c
SRCS_COMMON-$(LIBTHEORA)             += libmpcodecs/vd_theora.c
SRCS_COMMON-$(LIVE555)               += libmpdemux/demux_rtp.cpp        \
                                        libmpdemux/demux_rtp_codec.cpp  \
                                        stream/stream_live555.c
SRCS_COMMON-$(MACOSX_FINDER)         += osdep/macosx_finder_args.c
SRCS_COMMON-$(MNG)                   += libmpdemux/demux_mng.c
SRCS_COMMON-$(MPG123)                += libmpcodecs/ad_mpg123.c

SRCS_COMMON-$(MUSEPACK)              += libmpcodecs/ad_mpc.c            \
                                        libmpdemux/demux_mpc.c
SRCS_COMMON-$(NATIVE_RTSP)           += stream/stream_rtsp.c            \
                                        stream/freesdp/common.c         \
                                        stream/freesdp/errorlist.c      \
                                        stream/freesdp/parser.c         \
                                        stream/librtsp/rtsp.c           \
                                        stream/librtsp/rtsp_rtp.c       \
                                        stream/librtsp/rtsp_session.c   \

SRCS_COMMON-$(NETWORKING)            += stream/stream_netstream.c       \
                                        stream/asf_mmst_streaming.c     \
                                        stream/asf_streaming.c          \
                                        stream/cookies.c                \
                                        stream/http.c                   \
                                        stream/network.c                \
                                        stream/pnm.c                    \
                                        stream/rtp.c                    \
                                        stream/udp.c                    \
                                        stream/tcp.c                    \
                                        stream/stream_rtp.c             \
                                        stream/stream_udp.c             \
                                        stream/librtsp/rtsp.c           \
                                        stream/realrtsp/asmrp.c         \
                                        stream/realrtsp/real.c          \
                                        stream/realrtsp/rmff.c          \
                                        stream/realrtsp/sdpplin.c       \
                                        stream/realrtsp/xbuffer.c       \

SRCS_COMMON-$(PNG)                   += libmpcodecs/vd_mpng.c
SRCS_COMMON-$(POSTPROC)              += libmpcodecs/vf_pp.c
SRCS_COMMON-$(PRIORITY)              += osdep/priority.c
SRCS_COMMON-$(PVR)                   += stream/stream_pvr.c
SRCS_COMMON-$(QTX_CODECS)            += libmpcodecs/ad_qtaudio.c        \
                                        libmpcodecs/vd_qtvideo.c
SRCS_COMMON-$(RADIO)                 += stream/stream_radio.c
SRCS_COMMON-$(RADIO_CAPTURE)         += stream/audio_in.c
SRCS_COMMON-$(REAL_CODECS)           += libmpcodecs/ad_realaud.c        \
                                        libmpcodecs/vd_realvid.c
SRCS_COMMON-$(SPEEX)                 += libmpcodecs/ad_speex.c
SRCS_COMMON-$(STREAM_CACHE)          += stream/cache2.c
SRCS_COMMON-$(TV)                    += stream/frequencies.c            \
                                        stream/stream_tv.c              \
                                        stream/tv.c                     \
                                        stream/tvi_dummy.c
SRCS_COMMON-$(TV_BSDBT848)           += stream/tvi_bsdbt848.c
SRCS_COMMON-$(TV_DSHOW)              += stream/tvi_dshow.c              \
                                        loader/dshow/guids.c            \
                                        loader/dshow/mediatype.c        \

SRCS_COMMON-$(TV_V4L1)               += stream/tvi_v4l.c                \
                                        stream/audio_in.c
SRCS_COMMON-$(TV_V4L2)               += stream/tvi_v4l2.c               \
                                        stream/audio_in.c
SRCS_COMMON-$(UNRAR_EXEC)            += sub/unrar_exec.c
SRCS_COMMON-$(VCD)                   += stream/stream_vcd.c
SRCS_COMMON-$(VORBIS)                += libmpcodecs/ad_libvorbis.c      \
                                        libmpdemux/demux_ogg.c
SRCS_COMMON-$(VSTREAM)               += stream/stream_vstream.c
SRCS_QTX_EMULATION                   += loader/wrapper.S
SRCS_COMMON-$(QTX_EMULATION)         += $(SRCS_QTX_EMULATION)
SRCS_WIN32_EMULATION                 += loader/elfdll.c                 \
                                        loader/ext.c                    \
                                        loader/ldt_keeper.c             \
                                        loader/module.c                 \
                                        loader/pe_image.c               \
                                        loader/pe_resource.c            \
                                        loader/registry.c               \
                                        loader/resource.c               \
                                        loader/win32.c                  \

SRCS_COMMON-$(WIN32_EMULATION)       += $(SRCS_WIN32_EMULATION)

SRCS_COMMON-$(WIN32DLL)              += libmpcodecs/ad_acm.c            \
                                        libmpcodecs/ad_dmo.c            \
                                        libmpcodecs/ad_dshow.c          \
                                        libmpcodecs/ad_twin.c           \
                                        libmpcodecs/vd_dmo.c            \
                                        libmpcodecs/vd_dshow.c          \
                                        libmpcodecs/vd_vfw.c            \
                                        libmpcodecs/vd_vfwex.c          \
                                        libmpdemux/demux_avs.c          \
                                        loader/afl.c                    \
                                        loader/drv.c                    \
                                        loader/vfl.c                    \
                                        loader/dshow/DS_AudioDecoder.c  \
                                        loader/dshow/DS_Filter.c        \
                                        loader/dshow/DS_VideoDecoder.c  \
                                        loader/dshow/allocator.c        \
                                        loader/dshow/cmediasample.c     \
                                        loader/dshow/graph.c            \
                                        loader/dshow/guids.c            \
                                        loader/dshow/inputpin.c         \
                                        loader/dshow/mediatype.c        \
                                        loader/dshow/outputpin.c        \
                                        loader/dmo/DMO_AudioDecoder.c   \
                                        loader/dmo/DMO_VideoDecoder.c   \
                                        loader/dmo/buffer.c             \
                                        loader/dmo/dmo.c                \
                                        loader/dmo/dmo_guids.c          \

SRCS_COMMON-$(XANIM_CODECS)          += libmpcodecs/vd_xanim.c
SRCS_COMMON-$(XMMS_PLUGINS)          += libmpdemux/demux_xmms.c
SRCS_COMMON-$(XVID4)                 += libmpcodecs/vd_xvid4.c
SRCS_COMMON-$(ZR)                    += libmpcodecs/vd_zrmjpeg.c        \
                                        libmpcodecs/vf_zrmjpeg.c

SRCS_COMMON = asxparser.c                       \
              bstr.c                            \
              codec-cfg.c                       \
              cpudetect.c                       \
              edl.c                             \
              fmt-conversion.c                  \
              m_config.c                        \
              m_option.c                        \
              m_struct.c                        \
              mp_msg.c                          \
              mp_strings.c                      \
              mpcommon.c                        \
              parser-cfg.c                      \
              path.c                            \
              playtree.c                        \
              playtreeparser.c                  \
              subopt-helper.c                   \
              libaf/af.c                        \
              libaf/af_center.c                 \
              libaf/af_channels.c               \
              libaf/af_comp.c                   \
              libaf/af_delay.c                  \
              libaf/af_dummy.c                  \
              libaf/af_equalizer.c              \
              libaf/af_extrastereo.c            \
              libaf/af_format.c                 \
              libaf/af_gate.c                   \
              libaf/af_hrtf.c                   \
              libaf/af_karaoke.c                \
              libaf/af_pan.c                    \
              libaf/af_resample.c               \
              libaf/af_scaletempo.c             \
              libaf/af_sinesuppress.c           \
              libaf/af_stats.c                  \
              libaf/af_sub.c                    \
              libaf/af_surround.c               \
              libaf/af_sweep.c                  \
              libaf/af_tools.c                  \
              libaf/af_volnorm.c                \
              libaf/af_volume.c                 \
              libaf/filter.c                    \
              libaf/format.c                    \
              libaf/reorder_ch.c                \
              libaf/window.c                    \
              libmpcodecs/ad.c                  \
              libmpcodecs/ad_alaw.c             \
              libmpcodecs/ad_dk3adpcm.c         \
              libmpcodecs/ad_dvdpcm.c           \
              libmpcodecs/ad_hwac3.c            \
              libmpcodecs/ad_hwmpa.c            \
              libmpcodecs/ad_imaadpcm.c         \
              libmpcodecs/ad_msadpcm.c          \
              libmpcodecs/ad_pcm.c              \
              libmpcodecs/dec_audio.c           \
              libmpcodecs/dec_teletext.c        \
              libmpcodecs/dec_video.c           \
              libmpcodecs/img_format.c          \
              libmpcodecs/mp_image.c            \
              libmpcodecs/pullup.c              \
              libmpcodecs/vd.c                  \
              libmpcodecs/vd_hmblck.c           \
              libmpcodecs/vd_lzo.c              \
              libmpcodecs/vd_mpegpes.c          \
              libmpcodecs/vd_mtga.c             \
              libmpcodecs/vd_null.c             \
              libmpcodecs/vd_raw.c              \
              libmpcodecs/vd_sgi.c              \
              libmpcodecs/vd_black.c            \
              libmpcodecs/vf.c                  \
              libmpcodecs/vf_1bpp.c             \
              libmpcodecs/vf_2xsai.c            \
              libmpcodecs/vf_blackframe.c       \
              libmpcodecs/vf_boxblur.c          \
              libmpcodecs/vf_crop.c             \
              libmpcodecs/vf_cropdetect.c       \
              libmpcodecs/vf_decimate.c         \
              libmpcodecs/vf_delogo.c           \
              libmpcodecs/vf_denoise3d.c        \
              libmpcodecs/vf_detc.c             \
              libmpcodecs/vf_dint.c             \
              libmpcodecs/vf_divtc.c            \
              libmpcodecs/vf_down3dright.c      \
              libmpcodecs/vf_dsize.c            \
              libmpcodecs/vf_dvbscale.c         \
              libmpcodecs/vf_eq.c               \
              libmpcodecs/vf_eq2.c              \
              libmpcodecs/vf_expand.c           \
              libmpcodecs/vf_field.c            \
              libmpcodecs/vf_fil.c              \
              libmpcodecs/vf_filmdint.c         \
              libmpcodecs/vf_fixpts.c           \
              libmpcodecs/vf_flip.c             \
              libmpcodecs/vf_format.c           \
              libmpcodecs/vf_framestep.c        \
              libmpcodecs/vf_gradfun.c          \
              libmpcodecs/vf_halfpack.c         \
              libmpcodecs/vf_harddup.c          \
              libmpcodecs/vf_hqdn3d.c           \
              libmpcodecs/vf_hue.c              \
              libmpcodecs/vf_il.c               \
              libmpcodecs/vf_ilpack.c           \
              libmpcodecs/vf_ivtc.c             \
              libmpcodecs/vf_kerndeint.c        \
              libmpcodecs/vf_mirror.c           \
              libmpcodecs/vf_noformat.c         \
              libmpcodecs/vf_noise.c            \
              libmpcodecs/vf_ow.c               \
              libmpcodecs/vf_palette.c          \
              libmpcodecs/vf_perspective.c      \
              libmpcodecs/vf_phase.c            \
              libmpcodecs/vf_pp7.c              \
              libmpcodecs/vf_pullup.c           \
              libmpcodecs/vf_rectangle.c        \
              libmpcodecs/vf_remove_logo.c      \
              libmpcodecs/vf_rgbtest.c          \
              libmpcodecs/vf_rotate.c           \
              libmpcodecs/vf_sab.c              \
              libmpcodecs/vf_scale.c            \
              libmpcodecs/vf_smartblur.c        \
              libmpcodecs/vf_softpulldown.c     \
              libmpcodecs/vf_stereo3d.c         \
              libmpcodecs/vf_softskip.c         \
              libmpcodecs/vf_swapuv.c           \
              libmpcodecs/vf_telecine.c         \
              libmpcodecs/vf_test.c             \
              libmpcodecs/vf_tfields.c          \
              libmpcodecs/vf_tile.c             \
              libmpcodecs/vf_tinterlace.c       \
              libmpcodecs/vf_unsharp.c          \
              libmpcodecs/vf_vo.c               \
              libmpcodecs/vf_yadif.c            \
              libmpcodecs/vf_yuvcsp.c           \
              libmpcodecs/vf_yvu9.c             \
              libmpdemux/aac_hdr.c              \
              libmpdemux/asfheader.c            \
              libmpdemux/aviheader.c            \
              libmpdemux/aviprint.c             \
              libmpdemux/demuxer.c              \
              libmpdemux/demux_aac.c            \
              libmpdemux/demux_asf.c            \
              libmpdemux/demux_audio.c          \
              libmpdemux/demux_avi.c            \
              libmpdemux/demux_demuxers.c       \
              libmpdemux/demux_film.c           \
              libmpdemux/demux_fli.c            \
              libmpdemux/demux_lmlm4.c          \
              libmpdemux/demux_mf.c             \
              libmpdemux/demux_mkv.c            \
              libmpdemux/demux_mov.c            \
              libmpdemux/demux_mpg.c            \
              libmpdemux/demux_nsv.c            \
              libmpdemux/demux_pva.c            \
              libmpdemux/demux_rawaudio.c       \
              libmpdemux/demux_rawvideo.c       \
              libmpdemux/demux_realaud.c        \
              libmpdemux/demux_real.c           \
              libmpdemux/demux_roq.c            \
              libmpdemux/demux_smjpeg.c         \
              libmpdemux/demux_ts.c             \
              libmpdemux/demux_ty.c             \
              libmpdemux/demux_ty_osd.c         \
              libmpdemux/demux_viv.c            \
              libmpdemux/demux_vqf.c            \
              libmpdemux/demux_y4m.c            \
              libmpdemux/ebml.c                 \
              libmpdemux/extension.c            \
              libmpdemux/mf.c                   \
              libmpdemux/mp3_hdr.c              \
              libmpdemux/mp_taglists.c          \
              libmpdemux/mpeg_hdr.c             \
              libmpdemux/mpeg_packetizer.c      \
              libmpdemux/parse_es.c             \
              libmpdemux/parse_mp4.c            \
              libmpdemux/video.c                \
              libmpdemux/yuv4mpeg.c             \
              libmpdemux/yuv4mpeg_ratio.c       \
              osdep/$(GETCH)                    \
              osdep/$(TIMER)                    \
              stream/open.c                     \
              stream/stream.c                   \
              stream/stream_bd.c                \
              stream/stream_cue.c               \
              stream/stream_file.c              \
              stream/stream_mf.c                \
              stream/stream_null.c              \
              stream/url.c                      \
              sub/eosd.c                        \
              sub/find_sub.c                    \
              sub/osd.c                         \
              sub/spudec.c                      \
              sub/sub.c                         \
              sub/sub_cc.c                      \
              sub/subreader.c                   \
              sub/vobsub.c                      \
              $(SRCS_COMMON-yes)                \
              $(OS_FEATURE-no)

# MPlayer
SRCS_MPLAYER-$(3DFX)         += libvo/vo_3dfx.c
SRCS_MPLAYER-$(AA)           += libvo/vo_aa.c
SRCS_MPLAYER-$(ALSA)         += libao2/ao_alsa.c
SRCS_MPLAYER-$(APPLE_IR)     += input/appleir.c
SRCS_MPLAYER-$(APPLE_REMOTE) += input/ar.c
SRCS_MPLAYER-$(ARTS)         += libao2/ao_arts.c
SRCS_MPLAYER-$(BL)           += libvo/vo_bl.c
SRCS_MPLAYER-$(CACA)         += libvo/vo_caca.c
SRCS_MPLAYER-$(COREAUDIO)    += libao2/ao_coreaudio.c
SRCS_MPLAYER-$(COREVIDEO)    += libvo/vo_corevideo.m libvo/osx_common.c \
                                libvo/osx_objc_common.m
SRCS_MPLAYER-$(DART)         += libao2/ao_dart.c
SRCS_MPLAYER-$(DGA)          += libvo/vo_dga.c
SRCS_MPLAYER-$(DIRECT3D)     += libvo/vo_direct3d.c libvo/w32_common.c
SRCS_MPLAYER-$(DIRECTFB)     += libvo/vo_directfb2.c libvo/vo_dfbmga.c
SRCS_MPLAYER-$(DIRECTX)      += libao2/ao_dsound.c libvo/vo_directx.c
SRCS_MPLAYER-$(DXR2)         += libao2/ao_dxr2.c libvo/vo_dxr2.c
SRCS_MPLAYER-$(DXR3)         += libvo/vo_dxr3.c
SRCS_MPLAYER-$(ESD)          += libao2/ao_esd.c
SRCS_MPLAYER-$(FBDEV)        += libvo/vo_fbdev.c libvo/vo_fbdev2.c
SRCS_MPLAYER-$(FFMPEG)       += libvo/vo_png.c
SRCS_MPLAYER-$(GGI)          += libvo/vo_ggi.c
SRCS_MPLAYER-$(GIF)          += libvo/vo_gif89a.c
SRCS_MPLAYER-$(GL)           += libvo/gl_common.c libvo/vo_gl.c         \
                                libvo/csputils.c
SRCS_MPLAYER-$(GL_OSX)       += libvo/osx_common.c libvo/osx_objc_common.m
SRCS_MPLAYER-$(GL_SDL)       += libvo/sdl_common.c
SRCS_MPLAYER-$(GL_WIN32)     += libvo/w32_common.c libvo/vo_gl_tiled.c
SRCS_MPLAYER-$(GL_X11)       += libvo/x11_common.c libvo/vo_gl_tiled.c
SRCS_MPLAYER-$(GUI)          += gui/util/bitmap.c                       \
                                gui/util/list.c                         \
                                gui/util/string.c
SRCS_MPLAYER-$(GUI_GTK)      += gui/app/app.c                           \
                                gui/app/cfg.c                           \
                                gui/dialog/about.c                      \
                                gui/dialog/dialog.c                     \
                                gui/dialog/equalizer.c                  \
                                gui/dialog/fileselect.c                 \
                                gui/dialog/menu.c                       \
                                gui/dialog/msgbox.c                     \
                                gui/dialog/playlist.c                   \
                                gui/dialog/preferences.c                \
                                gui/dialog/skinbrowser.c                \
                                gui/dialog/tools.c                      \
                                gui/dialog/url.c                        \
                                gui/interface.c                         \
                                gui/skin/font.c                         \
                                gui/skin/skin.c                         \
                                gui/ui/actions.c                        \
                                gui/ui/main.c                           \
                                gui/ui/menu.c                           \
                                gui/ui/playbar.c                        \
                                gui/ui/render.c                         \
                                gui/ui/video.c                          \
                                gui/util/cut.c                          \
                                gui/wm/ws.c                             \
                                gui/wm/wsxdnd.c                         \

SRCS_MPLAYER-$(GUI_WIN32)    += gui/win32/dialogs.c                     \
                                gui/win32/gui.c                         \
                                gui/win32/interface.c                   \
                                gui/win32/playlist.c                    \
                                gui/win32/preferences.c                 \
                                gui/win32/skinload.c                    \
                                gui/win32/widgetrender.c                \
                                gui/win32/wincfg.c                      \

SRCS_MPLAYER-$(IVTV)         += libao2/ao_ivtv.c libvo/vo_ivtv.c
SRCS_MPLAYER-$(JACK)         += libao2/ao_jack.c
SRCS_MPLAYER-$(JOYSTICK)     += input/joystick.c
SRCS_MPLAYER-$(JPEG)         += libvo/vo_jpeg.c
SRCS_MPLAYER-$(KAI)          += libao2/ao_kai.c
SRCS_MPLAYER-$(KVA)          += libvo/vo_kva.c
SRCS_MPLAYER-$(LIBMENU)      += libmenu/menu.c                          \
                                libmenu/menu_chapsel.c                  \
                                libmenu/menu_cmdlist.c                  \
                                libmenu/menu_console.c                  \
                                libmenu/menu_filesel.c                  \
                                libmenu/menu_list.c                     \
                                libmenu/menu_param.c                    \
                                libmenu/menu_pt.c                       \
                                libmenu/menu_txt.c                      \
                                libmenu/vf_menu.c                       \

SRCS_MPLAYER-$(LIBMENU_DVBIN) += libmenu/menu_dvbin.c
SRCS_MPLAYER-$(LIRC)          += input/lirc.c
SRCS_MPLAYER-$(MATRIXVIEW)    += libvo/vo_matrixview.c                  \
                                 libvo/matrixview.c
SRCS_MPLAYER-$(MD5SUM)        += libvo/vo_md5sum.c
SRCS_MPLAYER-$(MGA)           += libvo/vo_mga.c
SRCS_MPLAYER-$(MNG)           += libvo/vo_mng.c
SRCS_MPLAYER-$(NAS)           += libao2/ao_nas.c
SRCS_MPLAYER-$(NETWORKING)    += udp_sync.c
SRCS_MPLAYER-$(OPENAL)        += libao2/ao_openal.c
SRCS_MPLAYER-$(OSS)           += libao2/ao_oss.c
SRCS_MPLAYER-$(PNM)           += libvo/vo_pnm.c
SRCS_MPLAYER-$(PULSE)         += libao2/ao_pulse.c
SRCS_MPLAYER-$(QUARTZ)        += libvo/vo_quartz.c libvo/osx_common.c
SRCS_MPLAYER-$(S3FB)          += libvo/vo_s3fb.c
SRCS_MPLAYER-$(SDL)           += libao2/ao_sdl.c                        \
                                 libvo/vo_sdl.c                         \
                                 libvo/sdl_common.c
SRCS_MPLAYER-$(SGIAUDIO)      += libao2/ao_sgi.c
SRCS_MPLAYER-$(SUNAUDIO)      += libao2/ao_sun.c
SRCS_MPLAYER-$(SVGA)          += libvo/vo_svga.c
SRCS_MPLAYER-$(TDFXFB)        += libvo/vo_tdfxfb.c
SRCS_MPLAYER-$(TDFXVID)       += libvo/vo_tdfx_vid.c
SRCS_MPLAYER-$(TGA)           += libvo/vo_tga.c
SRCS_MPLAYER-$(V4L2)          += libvo/vo_v4l2.c
SRCS_MPLAYER-$(V4L2)          += libao2/ao_v4l2.c
SRCS_MPLAYER-$(VDPAU)         += libvo/vo_vdpau.c
SRCS_MPLAYER-$(VESA)          += libvo/gtf.c                            \
                                 libvo/vo_vesa.c                        \
                                 libvo/vesa_lvo.c
SRCS_MPLAYER-$(VIDIX)         += libvo/vo_cvidix.c                      \
                                 libvo/vosub_vidix.c                    \
                                 vidix/vidix.c                          \
                                 vidix/drivers.c                        \
                                 vidix/dha.c                            \
                                 vidix/mtrr.c                           \
                                 vidix/pci.c                            \
                                 vidix/pci_names.c                      \
                                 vidix/pci_dev_ids.c                    \

SRCS_MPLAYER-$(VIDIX_CYBERBLADE)    += vidix/cyberblade_vid.c
SRCS_MPLAYER-$(VIDIX_IVTV)          += vidix/ivtv_vid.c
SRCS_MPLAYER-$(VIDIX_MACH64)        += vidix/mach64_vid.c
SRCS_MPLAYER-$(VIDIX_MGA)           += vidix/mga_vid.c
SRCS_MPLAYER-$(VIDIX_MGA_CRTC2)     += vidix/mga_crtc2_vid.c
SRCS_MPLAYER-$(VIDIX_NVIDIA)        += vidix/nvidia_vid.c
SRCS_MPLAYER-$(VIDIX_PM2)           += vidix/pm2_vid.c
SRCS_MPLAYER-$(VIDIX_PM3)           += vidix/pm3_vid.c
SRCS_MPLAYER-$(VIDIX_RADEON)        += vidix/radeon_vid.c
SRCS_MPLAYER-$(VIDIX_RAGE128)       += vidix/rage128_vid.c
SRCS_MPLAYER-$(VIDIX_S3)            += vidix/s3_vid.c
SRCS_MPLAYER-$(VIDIX_SH_VEU)        += vidix/sh_veu_vid.c
SRCS_MPLAYER-$(VIDIX_SIS)           += vidix/sis_vid.c vidix/sis_bridge.c
SRCS_MPLAYER-$(VIDIX_UNICHROME)     += vidix/unichrome_vid.c
SRCS_MPLAYER-$(WII)           += libvo/vo_wii.c
SRCS_MPLAYER-$(WIN32WAVEOUT)  += libao2/ao_win32.c
SRCS_MPLAYER-$(WINVIDIX)      += libvo/vo_winvidix.c
SRCS_MPLAYER-$(X11)           += libvo/vo_x11.c libvo/vo_xover.c        \
                                 libvo/x11_common.c
SRCS_MPLAYER-$(XMGA)          += libvo/vo_xmga.c
SRCS_MPLAYER-$(XV)            += libvo/vo_xv.c
SRCS_MPLAYER-$(XVIDIX)        += libvo/vo_xvidix.c
SRCS_MPLAYER-$(XVMC)          += libvo/vo_xvmc.c
SRCS_MPLAYER-$(XVR100)        += libvo/vo_xvr100.c
SRCS_MPLAYER-$(YUV4MPEG)      += libvo/vo_yuv4mpeg.c
SRCS_MPLAYER-$(ZR)            += libvo/jpeg_enc.c libvo/vo_zr.c libvo/vo_zr2.c

SRCS_MPLAYER = command.c                \
               m_property.c             \
               mixer.c                  \
               mp_fifo.c                \
               mplayer.c                \
               parser-mpcmd.c           \
               pnm_loader.c             \
               input/input.c            \
               libao2/ao_mpegpes.c      \
               libao2/ao_null.c         \
               libao2/ao_pcm.c          \
               libao2/audio_out.c       \
               libvo/aspect.c           \
               libvo/geometry.c         \
               libvo/video_out.c        \
               libvo/vo_mpegpes.c       \
               libvo/vo_null.c          \
               sub/spuenc.c             \
               $(SRCS_MPLAYER-yes)

#MEncoder
SRCS_MENCODER-$(FAAC)             += libmpcodecs/ae_faac.c
SRCS_MENCODER-$(FFMPEG)           += libmpcodecs/ae_lavc.c              \
                                     libmpcodecs/ve_lavc.c              \
                                     libmpdemux/muxer_lavf.c
SRCS_MENCODER-$(LIBDV)            += libmpcodecs/ve_libdv.c
SRCS_MENCODER-$(LIBLZO)           += libmpcodecs/ve_nuv.c               \
                                     libmpcodecs/native/rtjpegn.c
SRCS_MENCODER-$(MP3LAME)          += libmpcodecs/ae_lame.c
SRCS_MENCODER-$(QTX_CODECS_WIN32) += libmpcodecs/ve_qtvideo.c
SRCS_MENCODER-$(TOOLAME)          += libmpcodecs/ae_toolame.c
SRCS_MENCODER-$(TWOLAME)          += libmpcodecs/ae_twolame.c
SRCS_MENCODER-$(WIN32DLL)         += libmpcodecs/ve_vfw.c
SRCS_MENCODER-$(X264)             += libmpcodecs/ve_x264.c
SRCS_MENCODER-$(XVID4)            += libmpcodecs/ve_xvid4.c

SRCS_MENCODER = mencoder.c                      \
                parser-mecmd.c                  \
                xvid_vbr.c                      \
                libmpcodecs/ae.c                \
                libmpcodecs/ae_pcm.c            \
                libmpcodecs/ve.c                \
                libmpcodecs/ve_raw.c            \
                libmpdemux/muxer.c              \
                libmpdemux/muxer_avi.c          \
                libmpdemux/muxer_mpeg.c         \
                libmpdemux/muxer_rawaudio.c     \
                libmpdemux/muxer_rawvideo.c     \
                $(SRCS_MENCODER-yes)

# (linking) order matters for these libraries
FFMPEGPARTS_ALL = libpostproc libavfilter libavformat libavcodec libswscale libswresample libavutil
FFMPEGPARTS = $(foreach part, $(FFMPEGPARTS_ALL), $(if $(wildcard ffmpeg/$(part)), $(part)))
FFMPEGLIBS  = $(foreach part, $(FFMPEGPARTS), ffmpeg/$(part)/$(part).a)
FFMPEGFILES = $(foreach part, $(FFMPEGPARTS), $(wildcard $(addprefix ffmpeg/$(part)/,*.[chS] /*/*.[chS] /*/*.asm)))

COMMON_LIBS-$(FFMPEG_A)           += $(FFMPEGLIBS)
COMMON_LIBS                       += $(COMMON_LIBS-yes)

OBJS_COMMON                   += $(addsuffix .o, $(basename $(SRCS_COMMON)))
OBJS_MENCODER                 += $(addsuffix .o, $(basename $(SRCS_MENCODER)))
OBJS_MPLAYER                  += $(addsuffix .o, $(basename $(SRCS_MPLAYER)))
OBJS_MPLAYER-$(PE_EXECUTABLE) += osdep/mplayer-rc.o
OBJS_MPLAYER                  += $(OBJS_MPLAYER-yes)

MENCODER_DEPS = $(OBJS_MENCODER) $(OBJS_COMMON) $(COMMON_LIBS)
MPLAYER_DEPS  = $(OBJS_MPLAYER)  $(OBJS_COMMON) $(COMMON_LIBS)
DEP_FILES     = $(SRCS_COMMON) $(SRCS_MPLAYER) $(SRCS_MENCODER)
$(foreach suffix,.c .cpp .m .S,$(eval DEP_FILES := $(DEP_FILES:$(suffix)=.d)))

ALL_PRG-$(MPLAYER)  += mplayer$(EXESUF)
ALL_PRG-$(MENCODER) += mencoder$(EXESUF)

INSTALL_TARGETS-$(GUI)      += install-gui
INSTALL_TARGETS-$(MENCODER) += install-mencoder install-mencoder-man
INSTALL_TARGETS-$(MPLAYER)  += install-mplayer  install-mplayer-man

DIRS =  .                       \
        gui                     \
        gui/app                 \
        gui/dialog              \
        gui/skin                \
        gui/ui                  \
        gui/util                \
        gui/win32               \
        gui/wm                  \
        input                   \
        libaf                   \
        libao2                  \
        libmenu                 \
        libmpcodecs             \
        libmpcodecs/native      \
        libmpdemux              \
        libvo                   \
        loader                  \
        loader/dmo              \
        loader/dshow            \
        loader/wine             \
        mp3lib                  \
        osdep                   \
        stream                  \
        stream/freesdp          \
        stream/librtsp          \
        stream/realrtsp         \
        sub                     \
        TOOLS                   \
        vidix                   \

ALL_DIRS = $(DIRS)              \
           libass               \
           libdvdcss            \
           libdvdnav            \
           libdvdnav/vm         \
           libdvdread4          \
           libmpeg2             \
           tremor               \

ALLHEADERS = $(foreach dir,$(DIRS),$(wildcard $(dir)/*.h))

ADDSUFFIXES     = $(foreach suf,$(1),$(addsuffix $(suf),$(2)))
ADD_ALL_DIRS    = $(call ADDSUFFIXES,$(1),$(ALL_DIRS))
ADD_ALL_EXESUFS = $(1) $(call ADDSUFFIXES,$(EXESUFS_ALL),$(1))

GUI_ICONSIZES = 16x16 22x22 24x24 32x32 48x48 256x256



###### generic rules #######

all: $(ALL_PRG-yes)

%.o: %.S
	$(CC) $(CC_DEPFLAGS) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CC_DEPFLAGS) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CC) $(CC_DEPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o: %.m
	$(CC) $(CC_DEPFLAGS) $(CFLAGS) -c -o $@ $<

%-rc.o: %.rc
	$(WINDRES) -I. $< -o $@

$(FFMPEGLIBS): $(FFMPEGFILES) config.h
	$(MAKE) -C ffmpeg $(@:ffmpeg/%=%)

mencoder$(EXESUF): $(MENCODER_DEPS)
mencoder$(EXESUF): EXTRALIBS += $(EXTRALIBS_MENCODER)
mplayer$(EXESUF): $(MPLAYER_DEPS)
mplayer$(EXESUF): EXTRALIBS += $(EXTRALIBS_MPLAYER)
mencoder$(EXESUF) mplayer$(EXESUF):
	$(CC) -o $@ $^ $(EXTRALIBS)

codec-cfg-test$(EXESUF): HOSTCFLAGS := $(HOSTCFLAGS) -DTESTING
codec-cfg$(EXESUF) codecs2html$(EXESUF):  HOSTCFLAGS := $(HOSTCFLAGS) -DCODECS2HTML
codec-cfg$(EXESUF) codec-cfg-test$(EXESUF) codecs2html$(EXESUF): codec-cfg.c codec-cfg.h help_mp.h
	$(HOST_CC) $(HOSTCFLAGS) -o $@ $<

codecs.conf.h: codec-cfg$(EXESUF) etc/codecs.conf
	./$^ > $@

checksums: $(MPLAYER_DEPS) $(MENCODER_DEPS) mplayer$(EXESUF) mencoder$(EXESUF)
	md5sum $^ > checksums

check_checksums: $(MPLAYER_DEPS) $(MENCODER_DEPS) mplayer$(EXESUF) mencoder$(EXESUF)
	md5sum -c checksums

# ./configure must be rerun if it changed
config.mak: configure
	@echo "############################################################"
	@echo "####### Please run ./configure again - it's changed! #######"
	@echo "############################################################"

checkhelp: help/help_mp*.h
	help/help_check.sh $(CC) $^

help_mp.h: help/help_mp-en.h $(HELP_FILE)
	help/help_create.sh $(HELP_FILE) $(CHARSET)

# rebuild version.h each time the working copy is updated
version.h: version.sh $(wildcard .svn/entries .git/logs/HEAD)
	./$< `$(CC) -dumpversion`

%$(EXESUF): %.c
	$(CC) $(CC_DEPFLAGS) $(CFLAGS) -o $@ $^ $(LIBS)

%.ho: %.h
	$(CC) $(CFLAGS) -Wno-unused -c -o $@ -x c $<

checkheaders: $(ALLHEADERS:.h=.ho)



###### XML documentation ######

doc: html-chunked html-single

html-chunked: $(addprefix html-chunked-,$(DOC_LANGS))
html-single:  $(addprefix html-single-,$(DOC_LANGS))

xmllint: $(addprefix xmllint-,$(DOC_LANGS))

define lang-def
html-chunked-$(lang): DOCS/HTML/$(lang)/dummy.html
html-single-$(lang):  DOCS/HTML/$(lang)/MPlayer.html
DOCS/HTML/$(lang)/dummy.html DOCS/HTML/$(lang)/MPlayer.html: DOCS/xml/$(lang)/main.xml $(wildcard DOCS/xml/$(lang)/*.xml) DOCS/xml/html-common.xsl DOCS/HTML/$(lang)/default.css

DOCS/HTML/$(lang)/default.css:
	mkdir -p $$(@D)
	cp -f DOCS/xml/default.css $$(@D)

DOCS/HTML/$(lang)/dummy.html:
	SGML_CATALOG_FILES=$(CATALOG) $(XSLT_COMMAND) $$@ DOCS/xml/html-chunk.xsl $$<

DOCS/HTML/$(lang)/MPlayer.html:
	SGML_CATALOG_FILES=$(CATALOG) $(XSLT_COMMAND) $$@ DOCS/xml/html-single.xsl $$<

xmllint-$(lang):
	SGML_CATALOG_FILES=$(CATALOG) $(XMLLINT_COMMAND) DOCS/xml/$(lang)/main.xml
endef

$(foreach lang, $(DOC_LANG_ALL),$(eval $(lang-def)))



###### dependency declarations / specific CFLAGS ######

# Make sure all generated header files are created.
codec-cfg.o codec-cfg-test$(EXESUF): codecs.conf.h
$(DEP_FILES) $(MENCODER_DEPS) $(MPLAYER_DEPS): help_mp.h
mpcommon.o osdep/mplayer-rc.o gui/dialog/about.o gui/win32/gui.o: version.h

osdep/mplayer-rc.o: osdep/mplayer.exe.manifest

gui/%: CFLAGS += -Wno-strict-prototypes

libdvdcss/%:   CFLAGS := $(CFLAGS_LIBDVDCSS) $(CFLAGS)
libdvdnav/%:   CFLAGS := -Ilibdvdnav -DHAVE_CONFIG_H -DVERSION=\"MPlayer-custom\" $(CFLAGS)
libdvdread4/%: CFLAGS := -Ilibdvdread4 $(CFLAGS_LIBDVDCSS_DVDREAD) $(CFLAGS)

loader/%: CFLAGS += -fno-omit-frame-pointer $(CFLAGS_NO_OMIT_LEAF_FRAME_POINTER)
#loader/%: CFLAGS += -Ddbg_printf=__vprintf -DTRACE=__vprintf -DDETAILED_OUT
loader/win32%: CFLAGS += $(CFLAGS_STACKREALIGN)

mp3lib/decode_i586%: CFLAGS += -fomit-frame-pointer

stream/stream_dvdnav%: CFLAGS := $(CFLAGS_LIBDVDNAV) $(CFLAGS)

tremor/%: CFLAGS += $(CFLAGS_TREMOR_LOW)

vidix/%: CFLAGS += $(CFLAGS_DHAHELPER) $(CFLAGS_SVGALIB_HELPER)

VIDIX_PCI_FILES = vidix/pci_dev_ids.c vidix/pci_ids.h vidix/pci_names.c \
                  vidix/pci_vendors.h

$(VIDIX_PCI_FILES): vidix/pci_db2c.awk vidix/pci.db
	awk -f $^ $(VIDIX_PCIDB)

VIDIX_OBJS = $(filter vidix/%,$(SRCS_MPLAYER:.c=.o))

$(VIDIX_OBJS): $(VIDIX_PCI_FILES)



###### installation / clean / generic rules #######

install: $(INSTALL_TARGETS-yes)

install-dirs:
	$(INSTALL) -d $(BINDIR) $(CONFDIR) $(LIBDIR)

install-%: %$(EXESUF) install-dirs
	$(INSTALL) -m 755 $(INSTALLSTRIP) $< $(BINDIR)

install-gui: install-mplayer install-gui-icons
	-ln -sf mplayer$(EXESUF) $(BINDIR)/gmplayer$(EXESUF)
	$(INSTALL) -d $(DATADIR)/skins $(prefix)/share/applications
	$(INSTALL) -m 644 etc/mplayer.desktop $(prefix)/share/applications/

install-gui-icons:    $(foreach size,$(GUI_ICONSIZES),install-gui-icon-$(size))
install-gui-man:      $(foreach lang,$(MAN_LANGS),install-gui-man-$(lang))
install-mencoder-man: $(foreach lang,$(MAN_LANGS),install-mencoder-man-$(lang))
install-mplayer-man:  $(foreach lang,$(MAN_LANGS),install-mplayer-man-$(lang))

install-gui-man-en: install-mplayer-man-en
	cd $(MANDIR)/man1/ && ln -sf mplayer.1 gmplayer.1

install-mencoder-man-en: install-mplayer-man-en
	cd $(MANDIR)/man1 && ln -sf mplayer.1 mencoder.1

install-mplayer-man-en:
	$(INSTALL) -d $(MANDIR)/man1
	$(INSTALL) -m 644 DOCS/man/en/mplayer.1 $(MANDIR)/man1/

define GUI_ICON_RULE
install-gui-icon-$(size):
	$(INSTALL) -d $(prefix)/share/icons/hicolor/$(size)/apps
	$(INSTALL) -m 644 etc/mplayer$(size).png $(prefix)/share/icons/hicolor/$(size)/apps/mplayer.png
endef

define GUI_MAN_RULE
install-gui-man-$(lang): install-mplayer-man-$(lang)
	cd $(MANDIR)/$(lang)/man1/ && ln -sf mplayer.1 gmplayer.1
endef

define MENCODER_MAN_RULE
install-mencoder-man-$(lang): install-mplayer-man-$(lang)
	cd $(MANDIR)/$(lang)/man1 && ln -sf mplayer.1 mencoder.1
endef

define MPLAYER_MAN_RULE
install-mplayer-man-$(lang):
	$(INSTALL) -d $(MANDIR)/$(lang)/man1
	$(INSTALL) -m 644 DOCS/man/$(lang)/mplayer.1 $(MANDIR)/$(lang)/man1/
endef

$(foreach size,$(GUI_ICONSIZES),$(eval $(GUI_ICON_RULE)))
$(foreach lang,$(filter-out en,$(MAN_LANG_ALL)),$(eval $(GUI_MAN_RULE)))
$(foreach lang,$(filter-out en,$(MAN_LANG_ALL)),$(eval $(MENCODER_MAN_RULE)))
$(foreach lang,$(filter-out en,$(MAN_LANG_ALL)),$(eval $(MPLAYER_MAN_RULE)))

uninstall:
	rm -f $(BINDIR)/mplayer$(EXESUF) $(BINDIR)/gmplayer$(EXESUF)
	rm -f $(BINDIR)/mencoder$(EXESUF)
	rm -f $(MANDIR)/man1/mencoder.1 $(MANDIR)/man1/mplayer.1
	rm -f $(foreach size,$(GUI_ICONSIZES),$(prefix)/share/icons/hicolor/$(size)/apps/mplayer.png)
	rm -f $(prefix)/share/applications/mplayer.desktop
	rm -f $(MANDIR)/man1/mplayer.1 $(MANDIR)/man1/mencoder.1
	rm -f $(foreach lang,$(MAN_LANGS),$(foreach man,mplayer.1 mencoder.1,$(MANDIR)/$(lang)/man1/$(man)))

clean: testsclean toolsclean driversclean dhahelperclean
	-$(MAKE) -C ffmpeg $@
	-rm -rf tests/res
	-rm -f $(call ADD_ALL_DIRS,/*.o /*.d /*.a /*.ho /*~)
	-rm -f $(call ADD_ALL_EXESUFS,mplayer mencoder)
	-rm -f $(VIDIX_PCI_FILES)
	-rm -f $(call ADD_ALL_EXESUFS,codec-cfg cpuinfo)
	-rm -f codecs.conf.h help_mp.h version.h
	-rm -rf DOCS/tech/doxygen DOCS/HTML

distclean: clean
	-$(MAKE) -C ffmpeg $@
	-rm -f DOCS/xml/html-chunk.xsl DOCS/xml/html-single.xsl
	-rm -f $(foreach lang,$(DOC_LANG_ALL),DOCS/xml/$(lang)/main.xml)
	-rm -f config.* TAGS tags

doxygen:
	doxygen DOCS/tech/Doxyfile

TAGS:
	rm -f $@; find . -name '*.[chS]' -o -name '*.asm' | xargs etags -a

tags:
	rm -f $@; find . -name '*.[chS]' -o -name '*.asm' | xargs ctags -a



###### regression tests #######

BROKEN_SAMPLES =                         \
    h264-conformance/FM1_BT_B.h264       \
    h264-conformance/FM1_FT_E.264        \
    h264-conformance/FM2_SVA_B.264       \
    pva/PVA_test-partial.pva             \
    wmv8/wmv_drm.wmv                     \
    wtv/law-and-order-partial.wtv        \

AUDIO_ONLY_SAMPLES =                                               \
    aac/% ac3/% amrnb/% amrwb/% atrac1/% atrac3/% bink/binkaudio%  \
    creative/% dts/% duck/%-audio-only.avi eac3/% gsm/% imc/%      \
    lossless-audio/% mp3-conformance/% musepack/% nellymoser/%     \
    qcp/%                                                          \
    qt-surge-suite/% real/ra% sipr/% truespeech/% vorbis/%         \
    vqf/% w64/% wmapro/% wmavoice/%                                \

# running wildcard with empty FATE_SAMPLES seems to cause a lot of issues
ifdef FATE_SAMPLES
ALLSAMPLES_FULLPATH = $(wildcard $(FATE_SAMPLES)/*/*.*)
ALLSAMPLES          = $(patsubst $(FATE_SAMPLES)/%,%,$(ALLSAMPLES_FULLPATH))
SAMPLES := $(filter-out $(BROKEN_SAMPLES),$(ALLSAMPLES))
SAMPLES := $(filter-out $(AUDIO_ONLY_SAMPLES),$(SAMPLES))
RESULTS  = $(patsubst %,tests/res/%.md5,$(SAMPLES))

fatetest: $(RESULTS)

tests/res/%.md5: mplayer$(EXESUF) $(FATE_SAMPLES)/%
	@tests/faterun.sh $*
else
fatetest:
	@echo "You need to set FATE_SAMPLES for fatetest to work"
endif



###### tests / tools #######

MP_MSG_OBJS = mp_msg.o mp_fifo.o osdep/$(GETCH) osdep/$(TIMER)

libvo/aspecttest$(EXESUF): libvo/aspect.o libvo/geometry.o $(MP_MSG_OBJS)
libvo/aspecttest$(EXESUF): LIBS = $(MP_MSG_LIBS)

LOADER_TEST_OBJS = $(SRCS_WIN32_EMULATION:.c=.o) $(SRCS_QTX_EMULATION:.S=.o) ffmpeg/libavutil/libavutil.a osdep/mmap_anon.o cpudetect.o path.o $(MP_MSG_OBJS)

loader/qtx/list$(EXESUF) loader/qtx/qtxload$(EXESUF): CFLAGS += -g
loader/qtx/list$(EXESUF) loader/qtx/qtxload$(EXESUF): $(LOADER_TEST_OBJS)

TESTS-$(QTX_EMULATION) += loader/qtx/list loader/qtx/qtxload

TESTS := codecs2html codec-cfg-test libvo/aspecttest $(TESTS-yes)

TESTS_DEP_FILES = $(addsuffix .d,$(TESTS))

tests: $(addsuffix $(EXESUF),$(TESTS))

testsclean:
	-rm -f $(call ADD_ALL_EXESUFS,$(TESTS) $(TESTS-no))

TOOLS-$(ARCH_X86)               += fastmemcpybench
TOOLS-$(HAVE_WINDOWS_H)         += vfw2menc
TOOLS-$(SDL_IMAGE)              += bmovl-test
TOOLS-$(UNRAR_EXEC)             += subrip
TOOLS-$(WIN32_EMULATION)        += modify_reg

TOOLS := $(addprefix TOOLS/,alaw-gen asfinfo avi-fix avisubdump compare dump_mp4 movinfo netstream vivodump $(TOOLS-yes))

TOOLS_DEP_FILES = $(addsuffix .d,$(TOOLS))

tools: $(addsuffix $(EXESUF),$(TOOLS))

toolsclean:
	-rm -f $(call ADD_ALL_EXESUFS,$(TOOLS) $(TOOLS-no))
	-rm -f TOOLS/realcodecs/*.so.6.0

TOOLS/bmovl-test$(EXESUF): LIBS = -lSDL_image
TOOLS/vfw2menc$(EXESUF):   LIBS = -lwinmm -lole32
TOOLS/subrip$(EXESUF):     LIBS = $(MP_MSG_LIBS) -lm
TOOLS/subrip$(EXESUF): path.o sub/vobsub.o sub/spudec.o sub/unrar_exec.o \
    ffmpeg/libswscale/libswscale.a ffmpeg/libavutil/libavutil.a $(MP_MSG_OBJS)

mplayer-nomain.o: mplayer.c
	$(CC) $(CFLAGS) -DDISABLE_MAIN -c -o $@ $<

TOOLS/netstream$(EXESUF): TOOLS/netstream.c
TOOLS/vivodump$(EXESUF): TOOLS/vivodump.c
TOOLS/netstream$(EXESUF) TOOLS/vivodump$(EXESUF): $(subst mplayer.o,mplayer-nomain.o,$(OBJS_MPLAYER)) $(filter-out %mencoder.o,$(OBJS_MENCODER)) $(OBJS_COMMON) $(COMMON_LIBS)
	$(CC) $(CC_DEPFLAGS) $(CFLAGS) -o $@ $^ $(EXTRALIBS_MPLAYER) $(EXTRALIBS_MENCODER) $(EXTRALIBS)

REAL_SRCS    = $(wildcard TOOLS/realcodecs/*.c)
REAL_TARGETS = $(REAL_SRCS:.c=.so.6.0)

realcodecs: $(REAL_TARGETS)
realcodecs: CFLAGS += -g

%.so.6.0: %.o
	ld -shared -o $@ $< -ldl -lc



###### drivers #######

KERNEL_INC = /lib/modules/`uname -r`/build/include
KERNEL_VERSION = $(shell grep RELEASE $(KERNEL_INC)/linux/version.h | cut -d'"' -f2)
KERNEL_CFLAGS = -O2 -D__KERNEL__ -DMODULE -Wall -I$(KERNEL_INC) -include $(KERNEL_INC)/linux/modversions.h
KERNEL_OBJS = $(addprefix drivers/, mga_vid.o tdfx_vid.o radeon_vid.o rage128_vid.o)
MODULES_DIR = /lib/modules/$(KERNEL_VERSION)/misc
DRIVER_OBJS = $(KERNEL_OBJS) drivers/mga_vid_test drivers/tdfx_vid_test
DRIVER_DEP_FILES = $(KERNEL_OBJS:.o=.d) drivers/mga_vid_test.d drivers/tdfx_vid_test.d

drivers: $(DRIVER_OBJS)

$(DRIVER_OBJS): CFLAGS = $(KERNEL_CFLAGS)
drivers/radeon_vid.o drivers/rage128_vid.o: CFLAGS += -fomit-frame-pointer -fno-strict-aliasing -fno-common -ffast-math

install-drivers: $(DRIVER_OBJS)
	-mkdir -p $(MODULES_DIR)
	$(INSTALL) -m 644 $(KERNEL_OBJS) $(MODULES_DIR)
	depmod -a
	-mknod /dev/mga_vid    c 178 0
	-mknod /dev/tdfx_vid   c 178 0
	-mknod /dev/radeon_vid c 178 0
	-ln -s /dev/radeon_vid /dev/rage128_vid

driversclean:
	-rm -f $(DRIVER_OBJS) drivers/*~

DHAHELPER_DEP_FILES = vidix/dhahelper/dhahelper.d vidix/dhahelper/test.d vidix/dhahelperwin/dhahelper.d vidix/dhahelperwin/dhasetup.d
dhahelper: vidix/dhahelper/dhahelper.o vidix/dhahelper/test

vidix/dhahelper/dhahelper.o vidix/dhahelper/test: CFLAGS = $(KERNEL_CFLAGS)

install-dhahelper: vidix/dhahelper/dhahelper.o
	-mkdir -p $(MODULES_DIR)
	$(INSTALL) -m 644 $< $(MODULES_DIR)
	depmod -a
	-mknod /dev/dhahelper c 180 0

dhahelperwin: vidix/dhahelperwin/dhasetup.exe vidix/dhahelperwin/dhahelper.sys

vidix/dhahelperwin/dhahelper-rc.o: vidix/dhahelperwin/common.ver vidix/dhahelperwin/ntverp.h

vidix/dhahelperwin/base.tmp: vidix/dhahelperwin/dhahelper.o vidix/dhahelperwin/dhahelper-rc.o
	$(CC) -Wl,--base-file,$@ -Wl,--entry,_DriverEntry@8 -nostartfiles \
            -nostdlib -o $(@D)/junk.tmp $^ -lntoskrnl
	-rm -f $(@D)/junk.tmp

vidix/dhahelperwin/temp.exp: vidix/dhahelperwin/base.tmp
	dlltool --dllname $(@D)/dhahelper.sys --base-file $< --output-exp $@

vidix/dhahelperwin/dhahelper.sys: vidix/dhahelperwin/temp.exp vidix/dhahelperwin/dhahelper.o vidix/dhahelperwin/dhahelper-rc.o
	$(CC) -Wl,--subsystem,native -Wl,--image-base,0x10000 \
            -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 \
            -Wl,--entry,_DriverEntry@8 -Wl,$< -mdll -nostartfiles -nostdlib \
            -o $@ $(@:.sys=.o) $(@:.sys=-rc.o) -lntoskrnl
	strip $@

install-dhahelperwin:
	vidix/dhahelperwin/dhasetup.exe install

dhahelperclean:
	-rm -f vidix/dhahelper/*.o vidix/dhahelper/*~ vidix/dhahelper/test
	-rm -f $(addprefix vidix/dhahelperwin/,*.o *~ dhahelper.sys dhasetup.exe base.tmp temp.exp)


-include $(DEP_FILES) $(DRIVER_DEP_FILES) $(TESTS_DEP_FILES) $(TOOLS_DEP_FILES) $(DHAHELPER_DEP_FILES)

.PHONY: all doxygen *install* *tools drivers dhahelper*
.PHONY: checkheaders *clean tests check_checksums fatetest checkhelp
.PHONY: doc html-chunked* html-single* xmllint*

# Disable suffix rules.  Most of the builtin rules are suffix rules,
# so this saves some time on slow systems.
.SUFFIXES:
