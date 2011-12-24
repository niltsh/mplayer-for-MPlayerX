/*
 * Generic libav* helpers
 *
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "mp_msg.h"
#include "av_helpers.h"

int avcodec_initialized;
int avformat_initialized;

static void mp_msp_av_log_callback(void *ptr, int level, const char *fmt,
                                   va_list vl)
{
    static int print_prefix=1;
    AVClass *avc= ptr ? *(AVClass **)ptr : NULL;
    int type= MSGT_FIXME;
    int mp_level;

    switch(level){
    case AV_LOG_VERBOSE: mp_level = MSGL_V ; break;
    case AV_LOG_DEBUG:  mp_level= MSGL_V   ; break;
    case AV_LOG_INFO :  mp_level= MSGL_INFO; break;
    case AV_LOG_ERROR:  mp_level= MSGL_ERR ; break;
    default          :  mp_level= level > AV_LOG_DEBUG ? MSGL_DBG2 : MSGL_ERR; break;
    }

    if (ptr && !avc)
        mp_msg(MSGT_DECVIDEO, MSGL_ERR, "libav* called av_log with context containing a broken AVClass!\n");
    if (avc) {
        if(!strcmp(avc->class_name, "AVCodecContext")){
            AVCodecContext *s= ptr;
            if(s->codec){
                if(s->codec->type == AVMEDIA_TYPE_AUDIO){
                    if(s->codec->decode)
                        type= MSGT_DECAUDIO;
                }else if(s->codec->type == AVMEDIA_TYPE_VIDEO){
                    if(s->codec->decode)
                        type= MSGT_DECVIDEO;
                }
                //FIXME subtitles, encoders (what msgt for them? there is no appropriate ...)
            }
        }else if(!strcmp(avc->class_name, "AVFormatContext")){
            AVFormatContext *s= ptr;
            if(s->iformat)
                type= MSGT_DEMUXER;
            else if(s->oformat)
                type= MSGT_MUXER;
        }
    }

    if (!mp_msg_test(type, mp_level)) return;

    if(print_prefix && avc) {
        mp_msg(type, mp_level, "[%s @ %p]", avc->item_name(ptr), avc);
    }

    print_prefix= strchr(fmt, '\n') != NULL;
    mp_msg_va(type, mp_level, fmt, vl);
}

static void show_av_version(int type, const char *name,
                            int header_ver, int ver, const char *conf)
{
#ifdef CONFIG_FFMPEG_SO
#define FFMPEG_TYPE "external"
#else
#define FFMPEG_TYPE "internal"
#endif
    mp_msg(type, MSGL_INFO, "%s version %d.%d.%d (" FFMPEG_TYPE ")\n",
           name, ver >> 16, (ver >> 8) & 0xFF, ver & 0xFF);
    if (header_ver != ver)
        mp_msg(type, MSGL_INFO, "Mismatching header version %d.%d.%d\n",
               header_ver >> 16, (header_ver >> 8) & 0xFF, header_ver & 0xFF);
    mp_msg(type, MSGL_V, "Configuration: %s\n", conf);
}

void init_avcodec(void)
{
    if (!avcodec_initialized) {
        show_av_version(MSGT_DECVIDEO, "libavcodec", LIBAVCODEC_VERSION_INT,
                        avcodec_version(), avcodec_configuration());
        avcodec_register_all();
        avcodec_initialized = 1;
        av_log_set_callback(mp_msp_av_log_callback);
    }
}

void init_avformat(void)
{
    if (!avformat_initialized) {
        show_av_version(MSGT_DEMUX, "libavformat", LIBAVFORMAT_VERSION_INT,
                        avformat_version(), avformat_configuration());
        av_register_all();
        avformat_initialized = 1;
        av_log_set_callback(mp_msp_av_log_callback);
    }
}
