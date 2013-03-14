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
#include "sub/subreader.h"
#include "libmpdemux/demuxer.h"
#include "sub/ass_mp.h"

#define ROUND(x) ((int)((x) < 0 ? (x) - 0.5 : (x) + 0.5))

struct stream;
struct sh_audio;
struct sh_video;
struct sh_sub;

extern double sub_last_pts;
extern ASS_Track *ass_track;
extern subtitle *vo_sub_last;
extern char *spudec_ifo;
extern int forced_subs_only;

extern int sub_auto;
extern float sub_delay;
extern float sub_fps;
extern char **sub_name;
extern char **sub_paths;
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
void init_vo_spudec(struct stream *stream, struct sh_video *sh_video, struct sh_sub *sh_sub);
void update_subtitles(struct sh_video *sh_video, double refpts, demux_stream_t *d_dvdsub, int reset);
void update_teletext(struct sh_video *sh_video, demuxer_t *demuxer, int reset);
int select_audio(demuxer_t* demuxer, int audio_id, char* audio_lang);
void set_osd_subtitle(subtitle *subs);

int cfg_inc_verbose(m_option_t *conf);
int cfg_include(m_option_t *conf, const char *filename);

void common_preinit(int *argc_ptr, char **argv_ptr[]);
int common_init(void);

double calc_a_pts(struct sh_audio *sh_audio, demux_stream_t *d_audio);

#endif /* MPLAYER_MPCOMMON_H */
