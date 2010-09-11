/*
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

#ifndef MPLAYER_MPCOMMON_H
#define MPLAYER_MPCOMMON_H

#include "m_config.h"
#include "m_option.h"
#include "subreader.h"
#include "libmpdemux/demuxer.h"
#include "libmpdemux/stheader.h"
#include "libass/ass_mp.h"

extern double sub_last_pts;
extern ASS_Track *ass_track;
extern subtitle *vo_sub_last;

extern int sub_auto;
extern float sub_delay;
extern float sub_fps;
extern char **sub_name;
extern char  *font_name;
extern char  *sub_font_name;
extern char  *audio_lang;
extern char  *dvdsub_lang;

extern float  font_factor;
extern double force_fps;
extern double video_time_usage;
extern double vout_time_usage;

extern int disable_system_conf;
extern int disable_user_conf;
extern int audio_id;
extern int video_id;
extern int dvdsub_id;
extern int vobsub_id;

extern int stream_cache_size;
extern float stream_cache_min_percent;
extern float stream_cache_seek_min_percent;

extern const char *mencoder_version;
extern const char *mplayer_version;

extern m_config_t *mconfig;
extern const m_option_t noconfig_opts[];

void print_version(const char* name);
void update_subtitles(sh_video_t *sh_video, double refpts, demux_stream_t *d_dvdsub, int reset);
void update_teletext(sh_video_t *sh_video, demuxer_t *demuxer, int reset);
int select_audio(demuxer_t* demuxer, int audio_id, char* audio_lang);
void set_osd_subtitle(subtitle *subs);

#endif /* MPLAYER_MPCOMMON_H */
