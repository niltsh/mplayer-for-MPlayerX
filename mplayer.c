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

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#if defined(__MINGW32__) || defined(__CYGWIN__)
#define _UWIN 1  /*disable Non-underscored versions of non-ANSI functions as otherwise int eof would conflict with eof()*/
#include <windows.h>
#endif

#ifndef __MINGW32__
#include <sys/ioctl.h>
#include <sys/wait.h>
#else
#define SIGHUP  1       /* hangup */
#define SIGQUIT 3       /* quit */
#define SIGKILL 9       /* kill (cannot be caught or ignored) */
#define SIGBUS  10      /* bus error */
#define SIGPIPE 13      /* broken pipe */
#endif

#ifdef HAVE_RTC
#ifdef __linux__
#include <linux/rtc.h>
#else
#include <rtc.h>
#define RTC_IRQP_SET RTCIO_IRQP_SET
#define RTC_PIE_ON   RTCIO_PIE_ON
#endif /* __linux__ */
#endif /* HAVE_RTC */

/*
 * In Mac OS X the SDL-lib is built upon Cocoa. The easiest way to
 * make it all work is to use the builtin SDL-bootstrap code, which
 * will be done automatically by replacing our main() if we include SDL.h.
 */
#if defined(__APPLE__) && defined(CONFIG_SDL)
#ifdef CONFIG_SDL_SDL_H
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif
#endif

#include "gui/interface.h"
#include "input/input.h"
#include "libao2/audio_out.h"
#include "libavutil/avstring.h"
#include "libavutil/intreadwrite.h"
#include "libmenu/menu.h"
#include "libmpcodecs/dec_audio.h"
#include "libmpcodecs/dec_video.h"
#include "libmpcodecs/mp_image.h"
#include "libmpcodecs/vd.h"
#include "libmpcodecs/vf.h"
#include "libmpdemux/demuxer.h"
#include "libmpdemux/stheader.h"
#include "sub/font_load.h"
#include "sub/sub.h"
#include "libvo/video_out.h"
#include "stream/cache2.h"
#include "stream/stream.h"
#include "stream/stream_bd.h"
#include "stream/stream_dvdnav.h"
#include "stream/stream_radio.h"
#include "stream/tv.h"
#include "access_mpcontext.h"
#include "sub/ass_mp.h"
#include "cfg-mplayer-def.h"
#include "codec-cfg.h"
#include "command.h"
#include "edl.h"
#include "help_mp.h"
#include "m_config.h"
#include "m_option.h"
#include "m_property.h"
#include "m_struct.h"
#include "metadata.h"
#include "mixer.h"
#include "mp_core.h"
#include "mp_fifo.h"
#include "mp_msg.h"
#include "mp_strings.h"
#include "mpcommon.h"
#include "mplayer.h"
#include "osdep/getch2.h"
#include "osdep/timer.h"
#include "parser-cfg.h"
#include "parser-mpcmd.h"
#include "path.h"
#include "playtree.h"
#include "playtreeparser.h"
#include "sub/spudec.h"
#include "sub/subreader.h"
#include "sub/vobsub.h"
#include "sub/eosd.h"
#include "osdep/getch2.h"
#include "osdep/timer.h"

#include "udp_sync.h"

#ifdef CONFIG_X11
#include "libvo/x11_common.h"
#endif
#ifdef CONFIG_DVBIN
#include "stream/dvbin.h"
#endif
#ifdef CONFIG_DVDREAD
#include "stream/stream_dvd.h"
#endif

int slave_mode;
int player_idle_mode;
int quiet;
int enable_mouse_movements;
float start_volume = -1;
double start_pts   = MP_NOPTS_VALUE;
char *heartbeat_cmd;
float heartbeat_interval = 30.0;
static int max_framesize;

int noconsolecontrols;
//**************************************************************************//

// Not all functions in mplayer.c take the context as an argument yet
static MPContext mpctx_s = {
    .osd_function   = OSD_PLAY,
    .begin_skip     = MP_NOPTS_VALUE,
    .play_tree_step = 1,
    .global_sub_pos = -1,
    .set_of_sub_pos = -1,
    .file_format    = DEMUXER_TYPE_UNKNOWN,
    .loop_times     = -1,
#ifdef CONFIG_DVBIN
    .last_dvb_step  = 1,
#endif
};

static MPContext *mpctx = &mpctx_s;

int fixed_vo;

// benchmark:
double video_time_usage;
double vout_time_usage;
static double audio_time_usage;
static int total_time_usage_start;
static int total_frame_cnt;
static int drop_frame_cnt; // total number of dropped frames
int benchmark;

// options:
#define DEFAULT_STARTUP_DECODE_RETRY 8
int auto_quality;
static int output_quality;

float playback_speed = 1.0;

int use_gui;

#ifdef CONFIG_GUI
int enqueue;
#endif

static int list_properties;

int osd_level = 1;
// if nonzero, hide current OSD contents when GetTimerMS() reaches this
unsigned int osd_visible;
int osd_duration = 1000;
int osd_fractions; // determines how fractions of seconds are displayed
                   // on OSD

int term_osd = 1;
static char *term_osd_esc = "\x1b[A\r\x1b[K";
static char *playing_msg;
// seek:
static double seek_to_sec = MP_NOPTS_VALUE;
static off_t seek_to_byte;
static off_t step_sec;
static int loop_seek;

static m_time_size_t end_at = { .type = END_AT_NONE, .pos = 0 };

// A/V sync:
int autosync;        // 30 might be a good default value.

// may be changed by GUI:  (FIXME!)
float rel_seek_secs;
int abs_seek_pos;

// codecs:
char **audio_codec_list; // override audio codec
char **video_codec_list; // override video codec
char **audio_fm_list;    // override audio codec family
char **video_fm_list;    // override video codec family

// streaming:
int audio_id  = -1;
int video_id  = -1;
int dvdsub_id = -1;
// this dvdsub_id was selected via slang
// use this to allow dvdnav to follow -slang across stream resets,
// in particular the subtitle ID for a language changes
int dvdsub_lang_id;
int vobsub_id = -1;
char *audio_lang;
char *dvdsub_lang;
char *filename;
int file_filter = 1;

// cache2:
int stream_cache_size = -1;
#ifdef CONFIG_STREAM_CACHE
float stream_cache_min_percent      = 20.0;
float stream_cache_seek_min_percent = 50.0;
#endif

// dump:
char *stream_dump_name = "stream.dump";
int stream_dump_type;
uint64_t stream_dump_count;
unsigned stream_dump_start_time;
unsigned stream_dump_last_print_time;
int capture_dump;

// A-V sync:
static float default_max_pts_correction = -1;
static float max_pts_correction; //default_max_pts_correction;
static float c_total;
float audio_delay;
static int ignore_start;

static int softsleep;

double force_fps;
static int force_srate;
static int audio_output_format = AF_FORMAT_UNKNOWN;
int frame_dropping;        // option  0=no drop  1= drop vo  2= drop decode
static int play_n_frames    = -1;
static int play_n_frames_mf = -1;

// screen info:
char **video_driver_list;
char **audio_driver_list;

// sub:
char *font_name;
char *sub_font_name;
float font_factor = 0.75;
char **sub_name;
char **sub_paths;
float sub_delay;
float sub_fps;
int sub_auto = 1;
char *vobsub_name;
int subcc_enabled;
int suboverlap_enabled = 1;

char *current_module; // for debugging

#ifdef CONFIG_MENU
static const vf_info_t *const libmenu_vfs[] = {
    &vf_info_menu,
    NULL
};
static vf_instance_t *vf_menu;
int use_menu;
static char *menu_cfg;
static char *menu_root = "main";
#endif

#ifdef HAVE_RTC
static int nortc = 1;
static char *rtc_device;
#endif

edl_record_ptr edl_records;     ///< EDL entries memory area
edl_record_ptr next_edl_record; ///< only for traversing edl_records
short edl_decision;             ///< 1 when an EDL operation has been made.
short edl_needs_reset;          ///< 1 if we need to reset EDL next pointer
short edl_backward;             ///< 1 if we need to skip to the beginning of the next EDL record
FILE *edl_fd;                   ///< fd to write to when in -edlout mode.
// Number of seconds to add to the seek when jumping out
// of EDL scene in backward direction. This is needed to
// have some time after the seek to decide what to do next
// (next seek, pause,...), otherwise after the seek it will
// enter the same scene again and skip forward immediately
float edl_backward_delay = 2;
int edl_start_pts;              ///< Automatically add/sub this from EDL start/stop pos
int use_filedir_conf;
int use_filename_title;

static unsigned int initialized_flags;

int volstep = 3; ///< step size of mixer changes

#ifdef CONFIG_CRASH_DEBUG
static char *prog_path;
static int crash_debug;
#endif

static int allow_playlist_parsing;

/* This header requires all the global variable declarations. */
#include "cfg-mplayer.h"

const void *mpctx_get_video_out(MPContext *mpctx)
{
    return mpctx->video_out;
}

const void *mpctx_get_audio_out(MPContext *mpctx)
{
    return mpctx->audio_out;
}

void *mpctx_get_demuxer(MPContext *mpctx)
{
    return mpctx->demuxer;
}

void *mpctx_get_playtree_iter(MPContext *mpctx)
{
    return mpctx->playtree_iter;
}

void *mpctx_get_mixer(MPContext *mpctx)
{
    return &mpctx->mixer;
}

int mpctx_get_global_sub_size(MPContext *mpctx)
{
    return mpctx->global_sub_size;
}

int mpctx_get_osd_function(MPContext *mpctx)
{
    return mpctx->osd_function;
}

static int is_valid_metadata_type(metadata_t type)
{
    switch (type) {
    /* check for valid video stream */
    case META_VIDEO_CODEC:
    case META_VIDEO_BITRATE:
    case META_VIDEO_RESOLUTION:
        if (!mpctx->sh_video)
            return 0;
        break;

    /* check for valid audio stream */
    case META_AUDIO_CODEC:
    case META_AUDIO_BITRATE:
    case META_AUDIO_SAMPLES:
        if (!mpctx->sh_audio)
            return 0;
        break;

    /* check for valid demuxer */
    case META_INFO_TITLE:
    case META_INFO_ARTIST:
    case META_INFO_ALBUM:
    case META_INFO_YEAR:
    case META_INFO_COMMENT:
    case META_INFO_TRACK:
    case META_INFO_GENRE:
        if (!mpctx->demuxer)
            return 0;
        break;

    default:
        break;
    }

    return 1;
}

static char *get_demuxer_info(char *tag)
{
    char **info = mpctx->demuxer->info;
    int n;

    if (!info || !tag)
        return NULL;

    for (n = 0; info[2 * n] != NULL; n++)
        if (!strcasecmp(info[2 * n], tag))
            break;

    return info[2 * n + 1] ? strdup(info[2 * n + 1]) : NULL;
}

char *get_metadata(metadata_t type)
{
    sh_audio_t *const sh_audio = mpctx->sh_audio;
    sh_video_t *const sh_video = mpctx->sh_video;

    if (!is_valid_metadata_type(type))
        return NULL;

    switch (type) {
    case META_NAME:
        return strdup(mp_basename(filename));

    case META_VIDEO_CODEC:
        if (sh_video->format == 0x10000001)
            return strdup("mpeg1");
        else if (sh_video->format == 0x10000002)
            return strdup("mpeg2");
        else if (sh_video->format == 0x10000004)
            return strdup("mpeg4");
        else if (sh_video->format == 0x10000005)
            return strdup("h264");
        else if (sh_video->format >= 0x20202020)
            return mp_asprintf("%.4s", (char *)&sh_video->format);
        return mp_asprintf("0x%08X", sh_video->format);

    case META_VIDEO_BITRATE:
        return mp_asprintf("%d kbps", (int)(sh_video->i_bps * 8 / 1024));

    case META_VIDEO_RESOLUTION:
        return mp_asprintf("%d x %d", sh_video->disp_w, sh_video->disp_h);

    case META_AUDIO_CODEC:
        if (sh_audio->codec && sh_audio->codec->name)
            return strdup(sh_audio->codec->name);
        break;

    case META_AUDIO_BITRATE:
        return mp_asprintf("%d kbps", (int)(sh_audio->i_bps * 8 / 1000));

    case META_AUDIO_SAMPLES:
        return mp_asprintf("%d Hz, %d ch.", sh_audio->samplerate, sh_audio->channels);

    /* check for valid demuxer */
    case META_INFO_TITLE:
        return get_demuxer_info("Title");

    case META_INFO_ARTIST:
        return get_demuxer_info("Artist");

    case META_INFO_ALBUM:
        return get_demuxer_info("Album");

    case META_INFO_YEAR:
        return get_demuxer_info("Year");

    case META_INFO_COMMENT:
        return get_demuxer_info("Comment");

    case META_INFO_TRACK:
        return get_demuxer_info("Track");

    case META_INFO_GENRE:
        return get_demuxer_info("Genre");

    default:
        break;
    }

    return NULL;
}

static void print_file_properties(const MPContext *mpctx, const char *filename)
{
    double video_start_pts = MP_NOPTS_VALUE;
    mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_FILENAME=%s\n",
           filename_recode(filename));
    mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_DEMUXER=%s\n", mpctx->demuxer->desc->name);
    if (mpctx->sh_video) {
        /* Assume FOURCC if all bytes >= 0x20 (' ') */
        if (mpctx->sh_video->format >= 0x20202020)
            mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_VIDEO_FORMAT=%.4s\n", (char *)&mpctx->sh_video->format);
        else
            mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_VIDEO_FORMAT=0x%08X\n", mpctx->sh_video->format);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_VIDEO_BITRATE=%d\n",   mpctx->sh_video->i_bps * 8);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_VIDEO_WIDTH=%d\n",     mpctx->sh_video->disp_w);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_VIDEO_HEIGHT=%d\n",    mpctx->sh_video->disp_h);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_VIDEO_FPS=%5.3f\n",    mpctx->sh_video->fps);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_VIDEO_ASPECT=%1.4f\n", mpctx->sh_video->aspect);
        video_start_pts = ds_get_next_pts(mpctx->d_video);
    }
    if (mpctx->sh_audio) {
        /* Assume FOURCC if all bytes >= 0x20 (' ') */
        if (mpctx->sh_audio->format >= 0x20202020)
            mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_AUDIO_FORMAT=%.4s\n", (char *)&mpctx->sh_audio->format);
        else
            mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_AUDIO_FORMAT=%d\n", mpctx->sh_audio->format);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_AUDIO_BITRATE=%d\n", mpctx->sh_audio->i_bps * 8);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_AUDIO_RATE=%d\n",    mpctx->sh_audio->samplerate);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_AUDIO_NCH=%d\n",     mpctx->sh_audio->channels);
        start_pts = ds_get_next_pts(mpctx->d_audio);
    }
    if (video_start_pts != MP_NOPTS_VALUE) {
        if (start_pts == MP_NOPTS_VALUE || !mpctx->sh_audio ||
            (mpctx->sh_video && video_start_pts < start_pts))
            start_pts = video_start_pts;
    }
    if (start_pts != MP_NOPTS_VALUE)
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_START_TIME=%.2f\n", start_pts);
    else
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_START_TIME=unknown\n");
    mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_LENGTH=%.2f\n", demuxer_get_time_length(mpctx->demuxer));
    mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_SEEKABLE=%d\n",
           mpctx->stream->seek && (!mpctx->demuxer || mpctx->demuxer->seekable));
    if (mpctx->demuxer) {
        if (mpctx->demuxer->num_chapters == 0)
            stream_control(mpctx->demuxer->stream, STREAM_CTRL_GET_NUM_CHAPTERS, &mpctx->demuxer->num_chapters);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_CHAPTERS=%d\n", mpctx->demuxer->num_chapters);
    }
}

#ifdef CONFIG_DVDNAV
static void mp_dvdnav_context_free(MPContext *ctx)
{
    if (ctx->nav_smpi)
        free_mp_image(ctx->nav_smpi);
    ctx->nav_smpi = NULL;
    free(ctx->nav_buffer);
    ctx->nav_buffer  = NULL;
    ctx->nav_start   = NULL;
    ctx->nav_in_size = 0;
}

#endif

void uninit_player(unsigned int mask)
{
    mask &= initialized_flags;

    mp_msg(MSGT_CPLAYER, MSGL_DBG2, "\n*** uninit(0x%X)\n", mask);

    if (mask & INITIALIZED_ACODEC) {
        initialized_flags &= ~INITIALIZED_ACODEC;
        current_module     = "uninit_acodec";
        if (mpctx->sh_audio)
            uninit_audio(mpctx->sh_audio);
#ifdef CONFIG_GUI
        if (use_gui)
            gui(GUI_SET_AFILTER, NULL);
#endif
        mpctx->sh_audio      = NULL;
        mpctx->mixer.afilter = NULL;
    }

    if (mask & INITIALIZED_VCODEC) {
        initialized_flags &= ~INITIALIZED_VCODEC;
        current_module     = "uninit_vcodec";
        if (mpctx->sh_video)
            uninit_video(mpctx->sh_video);
        mpctx->sh_video = NULL;
#ifdef CONFIG_MENU
        vf_menu = NULL;
#endif
    }

    if (mask & INITIALIZED_DEMUXER) {
        initialized_flags &= ~INITIALIZED_DEMUXER;
        current_module     = "free_demuxer";
        if (mpctx->demuxer)
            free_demuxer(mpctx->demuxer);
        mpctx->demuxer = NULL;
    }

    // kill the cache process:
    if (mask & INITIALIZED_STREAM) {
        initialized_flags &= ~INITIALIZED_STREAM;
        current_module     = "uninit_stream";
        if (mpctx->stream)
            free_stream(mpctx->stream);
        mpctx->stream = NULL;
    }

    if (mask & INITIALIZED_VO) {
        initialized_flags &= ~INITIALIZED_VO;
        current_module     = "uninit_vo";
        mpctx->video_out->uninit();
        mpctx->video_out = NULL;
#ifdef CONFIG_DVDNAV
        mp_dvdnav_context_free(mpctx);
#endif
        if (vo_spudec) {
            current_module = "uninit_spudec";
            spudec_free(vo_spudec);
            vo_spudec = NULL;
        }
    }

    // Must be after libvo uninit, as few vo drivers (svgalib) have tty code.
    if (mask & INITIALIZED_GETCH2) {
        initialized_flags &= ~INITIALIZED_GETCH2;
        current_module     = "uninit_getch2";
        mp_msg(MSGT_CPLAYER, MSGL_DBG2, "\n[[[uninit getch2]]]\n");
        // restore terminal:
        getch2_disable();
    }

    if (mask & INITIALIZED_SUBS) {
        initialized_flags &= ~INITIALIZED_SUBS;
        if (mpctx->set_of_sub_size > 0) {
            int i;
            current_module = "sub_free";
            for (i = 0; i < mpctx->set_of_sub_size; ++i) {
                sub_free(mpctx->set_of_subtitles[i]);
#ifdef CONFIG_ASS
                if (mpctx->set_of_ass_tracks[i])
                    ass_free_track(mpctx->set_of_ass_tracks[i]);
#endif
            }
            mpctx->set_of_sub_size = 0;
        }
        vo_sub_last = vo_sub = NULL;
        subdata     = NULL;
#ifdef CONFIG_ASS
        ass_track = NULL;
        if (ass_library)
            ass_clear_fonts(ass_library);
#endif
    }

    if (mask & INITIALIZED_VOBSUB) {
        initialized_flags &= ~INITIALIZED_VOBSUB;
        current_module     = "uninit_vobsub";
        if (vo_vobsub)
            vobsub_close(vo_vobsub);
        vo_vobsub = NULL;
    }

    if (mask & INITIALIZED_AO) {
        initialized_flags &= ~INITIALIZED_AO;
        current_module     = "uninit_ao";
        if (mpctx->edl_muted)
            mixer_mute(&mpctx->mixer);
        if (mpctx->audio_out)
            mpctx->audio_out->uninit(mpctx->eof ? 0 : 1);
        mpctx->audio_out = NULL;
    }

#ifdef CONFIG_GUI
    if (mask & INITIALIZED_GUI) {
        initialized_flags &= ~INITIALIZED_GUI;
        current_module     = "uninit_gui";
        guiDone();
    }
#endif

    if (mask & INITIALIZED_INPUT) {
        initialized_flags &= ~INITIALIZED_INPUT;
        current_module     = "uninit_input";
        mp_input_uninit();
#ifdef CONFIG_MENU
        if (use_menu)
            menu_uninit();
#endif
    }

    current_module = NULL;
}

void exit_player_with_rc(enum exit_reason how, int rc)
{
#ifdef CONFIG_NETWORKING
    if (udp_master)
        send_udp(udp_ip, udp_port, "bye");
#endif /* CONFIG_NETWORKING */

    if (mpctx->user_muted && !mpctx->edl_muted)
        mixer_mute(&mpctx->mixer);
    uninit_player(INITIALIZED_ALL);
#if defined(__MINGW32__) || defined(__CYGWIN__)
    timeEndPeriod(1);
#endif
#ifdef CONFIG_X11
#ifdef CONFIG_GUI
    if (!use_gui)
#endif
    vo_uninit(); // Close the X11 connection (if any is open).
#endif

#ifdef CONFIG_FREETYPE
    current_module = "uninit_font";
    if (sub_font && sub_font != vo_font)
        free_font_desc(sub_font);
    sub_font = NULL;
    if (vo_font)
        free_font_desc(vo_font);
    vo_font = NULL;
    done_freetype();
#endif
    free_osd_list();

#ifdef CONFIG_ASS
    ass_library_done(ass_library);
    ass_library = NULL;
#endif

    current_module = "exit_player";

    if (mpctx->playtree_iter)
        play_tree_iter_free(mpctx->playtree_iter);
    mpctx->playtree_iter = NULL;
    if (mpctx->playtree)
        play_tree_free(mpctx->playtree, 1);
    mpctx->playtree = NULL;

    free(edl_records); // free mem allocated for EDL
    edl_records = NULL;
    switch (how) {
    case EXIT_QUIT:
        mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_ExitingHow, MSGTR_Exit_quit);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_EXIT=QUIT\n");
        break;
    case EXIT_EOF:
        mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_ExitingHow, MSGTR_Exit_eof);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_EXIT=EOF\n");
        break;
    case EXIT_ERROR:
        mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_ExitingHow, MSGTR_Exit_error);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_EXIT=ERROR\n");
        break;
    default:
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_EXIT=NONE\n");
    }
    mp_msg(MSGT_CPLAYER, MSGL_DBG2, "max framesize was %d bytes\n", max_framesize);

    // must be last since e.g. mp_msg uses option values
    // that will be freed by this.
    if (mconfig)
        m_config_free(mconfig);
    mconfig = NULL;

    exit(rc);
}

void exit_player(enum exit_reason how)
{
    exit_player_with_rc(how, 1);
}

#ifndef __MINGW32__
static void child_sighandler(int x)
{
    pid_t pid;
    do {
        pid = waitpid(-1, NULL, WNOHANG);
    } while (pid > 0);
}

#endif

static void exit_sighandler(int x)
{
    static int sig_count;
#ifdef CONFIG_CRASH_DEBUG
    if (!crash_debug || x != SIGTRAP)
#endif
    ++sig_count;
    if (initialized_flags == 0 && sig_count > 1)
        exit(1);
    if (sig_count == 5) {
        /* We're crashing bad and can't uninit cleanly :(
         * by popular request, we make one last (dirty)
         * effort to restore the user's
         * terminal. */
        getch2_disable();
        exit(1);
    }
    if (sig_count == 6)
        exit(1);
    if (sig_count > 6) {
        // can't stop :(
#ifndef __MINGW32__
        kill(getpid(), SIGKILL);
#endif
    }
    mp_msg(MSGT_CPLAYER, MSGL_FATAL, "\n" MSGTR_IntBySignal, x,
           current_module ? current_module : MSGTR_Unknown
           );
    mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_SIGNAL=%d\n", x);
    if (sig_count <= 1)
        switch (x) {
        case SIGINT:
        case SIGPIPE:
        case SIGQUIT:
        case SIGTERM:
        case SIGKILL:
            async_quit_request = 1;
            return; // killed from keyboard (^C) or killed [-9]
        case SIGILL:
#if CONFIG_RUNTIME_CPUDETECT
            mp_msg(MSGT_CPLAYER, MSGL_FATAL, MSGTR_Exit_SIGILL_RTCpuSel);
#else
            mp_msg(MSGT_CPLAYER, MSGL_FATAL, MSGTR_Exit_SIGILL);
#endif
        case SIGFPE:
        case SIGSEGV:
            mp_msg(MSGT_CPLAYER, MSGL_FATAL, MSGTR_Exit_SIGSEGV_SIGFPE);
        default:
            mp_msg(MSGT_CPLAYER, MSGL_FATAL, MSGTR_Exit_SIGCRASH);
#ifdef CONFIG_CRASH_DEBUG
            if (crash_debug) {
                int gdb_pid;
                mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_Forking);
                gdb_pid = fork();
                mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_Forked);
                if (gdb_pid == 0) { // We are the child
                    char spid[20];
                    snprintf(spid, sizeof(spid), "%i", getppid());
                    getch2_disable(); // allow terminal to work properly with gdb
                    if (execlp("gdb", "gdb", prog_path, spid, "-ex", "bt", NULL) == -1)
                        mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_CouldntStartGdb);
                } else if (gdb_pid < 0)
                    mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_CouldntFork);
                else {
                    waitpid(gdb_pid, NULL, 0);
                }
                if (x == SIGTRAP)
                    return;
            }
#endif
        }
    getch2_disable();
    exit(1);
}

static void parse_cfgfiles(m_config_t *conf)
{
    char *conffile;
    int conffile_fd;
    if (!disable_system_conf &&
        m_config_parse_config_file(conf, MPLAYER_CONFDIR "/mplayer.conf", 1) < 0)
        exit_player(EXIT_NONE);
    if ((conffile = get_path("")) == NULL) {
        mp_msg(MSGT_CPLAYER, MSGL_WARN, MSGTR_NoHomeDir);
    } else {
#ifdef __MINGW32__
        mkdir(conffile);
#else
        mkdir(conffile, 0777);
#endif
        free(conffile);
        if ((conffile = get_path("config")) == NULL) {
            mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_GetpathProblem);
        } else {
            if ((conffile_fd = open(conffile, O_CREAT | O_EXCL | O_WRONLY, 0666)) != -1) {
                mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_CreatingCfgFile, conffile);
                write(conffile_fd, default_config, strlen(default_config));
                close(conffile_fd);
            }
            if (!disable_user_conf &&
                m_config_parse_config_file(conf, conffile, 1) < 0)
                exit_player(EXIT_NONE);
            free(conffile);
        }
    }
}

#define PROFILE_CFG_PROTOCOL "protocol."

static void load_per_protocol_config(m_config_t *conf, const char *const file)
{
    char *str;
    char protocol[strlen(PROFILE_CFG_PROTOCOL) + strlen(file) + 1];
    m_profile_t *p;

    /* does filename actually uses a protocol ? */
    str = strstr(file, "://");
    if (!str)
        return;

    sprintf(protocol, "%s%s", PROFILE_CFG_PROTOCOL, file);
    protocol[strlen(PROFILE_CFG_PROTOCOL) + strlen(file) - strlen(str)] = '\0';
    p = m_config_get_profile(conf, protocol);
    if (p) {
        mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_LoadingProtocolProfile, protocol);
        m_config_set_profile(conf, p);
    }
}

#define PROFILE_CFG_EXTENSION "extension."

static void load_per_extension_config(m_config_t *conf, const char *const file)
{
    char *str;
    char extension[strlen(PROFILE_CFG_EXTENSION) + 8];
    m_profile_t *p;

    /* does filename actually have an extension ? */
    str = strrchr(filename, '.');
    if (!str)
        return;

    sprintf(extension, PROFILE_CFG_EXTENSION);
    strncat(extension, ++str, 7);
    p = m_config_get_profile(conf, extension);
    if (p) {
        mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_LoadingExtensionProfile, extension);
        m_config_set_profile(conf, p);
    }
}

#define PROFILE_CFG_VO "vo."
#define PROFILE_CFG_AO "ao."

static void load_per_output_config(m_config_t *conf, char *cfg, char *out)
{
    char profile[strlen(cfg) + strlen(out) + 1];
    m_profile_t *p;

    sprintf(profile, "%s%s", cfg, out);
    p = m_config_get_profile(conf, profile);
    if (p) {
        mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_LoadingExtensionProfile, profile);
        m_config_set_profile(conf, p);
    }
}

/**
 * @brief Tries to load a config file.
 * @return 0 if file was not found, 1 otherwise
 */
static int try_load_config(m_config_t *conf, const char *file)
{
    struct stat st;
    if (stat(file, &st))
        return 0;
    mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_LoadingConfig, file);
    m_config_parse_config_file(conf, file, 0);
    return 1;
}

static void load_per_file_config(m_config_t *conf, const char *const file)
{
    char *confpath;
    char cfg[PATH_MAX];
    const char *name;

    if (strlen(file) > PATH_MAX - 14) {
        mp_msg(MSGT_CPLAYER, MSGL_WARN, MSGTR_FilenameTooLong);
        return;
    }
    sprintf(cfg, "%s.conf", file);

    name = mp_basename(cfg);
    if (use_filedir_conf) {
        char dircfg[PATH_MAX];
        strcpy(dircfg, cfg);
        strcpy(dircfg + (name - cfg), "mplayer.conf");
        try_load_config(conf, dircfg);

        if (try_load_config(conf, cfg))
            return;
    }

    if ((confpath = get_path(name)) != NULL) {
        try_load_config(conf, confpath);

        free(confpath);
    }
}

static int load_profile_config(m_config_t *conf, const char *const file)
{
    if (file) {
        load_per_protocol_config(conf, file);
        load_per_extension_config(conf, file);
        load_per_file_config(conf, file);
    }

    return file != NULL;
}

/* When libmpdemux performs a blocking operation (network connection or
 * cache filling) if the operation fails we use this function to check
 * if it was interrupted by the user.
 * The function returns a new value for eof. */
static int libmpdemux_was_interrupted(int eof)
{
    mp_cmd_t *cmd;
    if ((cmd = mp_input_get_cmd(0, 0, 0)) != NULL) {
        switch (cmd->id) {
        case MP_CMD_QUIT:
            run_command(mpctx, cmd);
        case MP_CMD_PLAY_TREE_STEP:
            eof = (cmd->args[0].v.i > 0) ? PT_NEXT_ENTRY : PT_PREV_ENTRY;
            mpctx->play_tree_step = (cmd->args[0].v.i == 0) ? 1 : cmd->args[0].v.i;
            break;
        case MP_CMD_PLAY_TREE_UP_STEP:
            eof = (cmd->args[0].v.i > 0) ? PT_UP_NEXT : PT_UP_PREV;
            break;
        case MP_CMD_PLAY_ALT_SRC_STEP:
            eof = (cmd->args[0].v.i > 0) ? PT_NEXT_SRC : PT_PREV_SRC;
            break;
        }
        mp_cmd_free(cmd);
    }
    return eof;
}

static int playtree_add_playlist(play_tree_t *entry)
{
    play_tree_add_bpf(entry, filename);

#ifdef CONFIG_GUI
    if (use_gui) {
        if (entry) {
            guiPlaylist(GUI_PLAYLIST_ADD, entry, mconfig, 0);
            play_tree_free_list(entry, 1);
        }
    } else
#endif
    {
        if (!entry) {
            entry = mpctx->playtree_iter->tree;
            if (play_tree_iter_step(mpctx->playtree_iter, 1, 0) != PLAY_TREE_ITER_ENTRY)
                return PT_NEXT_ENTRY;
            // Loop with a single file
            if (mpctx->playtree_iter->tree == entry &&
                play_tree_iter_up_step(mpctx->playtree_iter, 1, 0) != PLAY_TREE_ITER_ENTRY)
                return PT_NEXT_ENTRY;
            play_tree_remove(entry, 1, 1);
            return PT_NEXT_SRC;
        }
        play_tree_insert_entry(mpctx->playtree_iter->tree, entry);
        play_tree_set_params_from(entry, mpctx->playtree_iter->tree);
        entry = mpctx->playtree_iter->tree;
        if (play_tree_iter_step(mpctx->playtree_iter, 1, 0) != PLAY_TREE_ITER_ENTRY)
            return PT_NEXT_ENTRY;
        play_tree_remove(entry, 1, 1);
    }
    return PT_NEXT_SRC;
}

void add_subtitles(char *filename, float fps, int noerr)
{
    sub_data *subd;
#ifdef CONFIG_ASS
    ASS_Track *asst = 0;
#endif

    if (filename == NULL || mpctx->set_of_sub_size >= MAX_SUBTITLE_FILES)
        return;

    subd = sub_read_file(filename, fps);
#ifdef CONFIG_ASS
    if (ass_enabled)
#ifdef CONFIG_ICONV
        asst = ass_read_stream(ass_library, filename, sub_cp);
#else
        asst = ass_read_stream(ass_library, filename, 0);
#endif
    if (ass_enabled && subd && !asst)
        asst = ass_read_subdata(ass_library, subd, fps);

    if (!asst && !subd)
#else
    if (!subd)
#endif
        mp_msg(MSGT_CPLAYER, noerr ? MSGL_WARN : MSGL_ERR, MSGTR_CantLoadSub,
               filename_recode(filename));

#ifdef CONFIG_ASS
    if (!asst && !subd)
        return;
    mpctx->set_of_ass_tracks[mpctx->set_of_sub_size] = asst;
#else
    if (!subd)
        return;
#endif
    mpctx->set_of_subtitles[mpctx->set_of_sub_size] = subd;
    mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_FILE_SUB_ID=%d\n", mpctx->set_of_sub_size);
    mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_FILE_SUB_FILENAME=%s\n",
           filename_recode(filename));
    ++mpctx->set_of_sub_size;
    mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_AddedSubtitleFile, mpctx->set_of_sub_size,
           filename_recode(filename));
}

static int add_vob_subtitle(const char *vobname, const char *const ifo, int force, void *spu)
{
    if (!vobname)
        return 0;

    assert(!vo_vobsub);

    vo_vobsub = vobsub_open(vobname, ifo, force, spu);

    if (!vo_vobsub && force)
        mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_CantLoadSub,
               filename_recode(vobname));

    return !!vo_vobsub;
}

// FIXME: if/when the GUI calls this, global sub numbering gets (potentially) broken.
void update_set_of_subtitles(void)
{
    // subdata was changed, set_of_sub... have to be updated.
    sub_data **const set_of_subtitles = mpctx->set_of_subtitles;
    int i;
    if (mpctx->set_of_sub_size > 0 && subdata == NULL) { // *subdata was deleted
        for (i = mpctx->set_of_sub_pos + 1; i < mpctx->set_of_sub_size; ++i)
            set_of_subtitles[i - 1] = set_of_subtitles[i];
        set_of_subtitles[mpctx->set_of_sub_size - 1] = NULL;
        --mpctx->set_of_sub_size;
        if (mpctx->set_of_sub_size > 0)
            subdata = set_of_subtitles[mpctx->set_of_sub_pos = 0];
    } else if (mpctx->set_of_sub_size > 0 && subdata != NULL) { // *subdata was changed
        set_of_subtitles[mpctx->set_of_sub_pos] = subdata;
    } else if (mpctx->set_of_sub_size <= 0 && subdata != NULL) { // *subdata was added
        set_of_subtitles[mpctx->set_of_sub_pos = mpctx->set_of_sub_size] = subdata;
        ++mpctx->set_of_sub_size;
    }
}

/**
 * @brief Append a formatted string.
 * @param buf buffer to print into
 * @param pos position of terminating 0 in buf
 * @param len maximum number of characters in buf, not including terminating 0
 * @param format printf format string
 */
static void saddf(char *buf, unsigned *pos, int len, const char *format, ...)
{
    va_list va;
    va_start(va, format);
    *pos += vsnprintf(&buf[*pos], len - *pos, format, va);
    va_end(va);
    if (*pos >= len) {
        buf[len] = 0;
        *pos     = len;
    }
}

/**
 * @brief Append time in the hh:mm:ss.f format.
 * @param buf buffer to print into
 * @param pos position of terminating 0 in buf
 * @param len maximum number of characters in buf, not including terminating 0
 * @param time time value to convert/append
 */
static void sadd_hhmmssf(char *buf, unsigned *pos, int len, float time)
{
    int64_t tenths = 10 * time;
    int f1 = tenths % 10;
    int ss = (tenths / 10) % 60;
    int mm = (tenths / 600) % 60;
    int hh = tenths / 36000;
    if (time <= 0) {
        saddf(buf, pos, len, "unknown");
        return;
    }
    if (hh > 0)
        saddf(buf, pos, len, "%2d:", hh);
    if (hh > 0 || mm > 0)
        saddf(buf, pos, len, "%02d:", mm);
    saddf(buf, pos, len, "%02d.%1d", ss, f1);
}

/**
 * @brief Print the status line.
 * @param a_pos audio position
 * @param a_v A-V desynchronization
 * @param corr amount out A-V synchronization
 */
static void print_status(float a_pos, float a_v, float corr)
{
    sh_video_t *const sh_video = mpctx->sh_video;
    int width;
    char *line;
    unsigned pos = 0;
    get_screen_size();
    if (screen_width > 0)
        width = screen_width;
    else
        width = 80;
#if defined(__MINGW32__) || defined(__CYGWIN__) || defined(__OS2__)
    /* Windows command line is broken (MinGW's rxvt works, but we
     * should not depend on that). */
    width--;
#endif
    line = malloc(width + 1); // one additional char for the terminating null

    // Audio time
    if (mpctx->sh_audio) {
        saddf(line, &pos, width, "A:%6.1f ", a_pos);
        if (!sh_video) {
            float len = demuxer_get_time_length(mpctx->demuxer);
            saddf(line, &pos, width, "(");
            sadd_hhmmssf(line, &pos, width, a_pos);
            saddf(line, &pos, width, ") of %.1f (", len);
            sadd_hhmmssf(line, &pos, width, len);
            saddf(line, &pos, width, ") ");
        }
    }

    // Video time
    if (sh_video)
        saddf(line, &pos, width, "V:%6.1f ", sh_video->pts);

    // A-V sync
    if (mpctx->sh_audio && sh_video)
        saddf(line, &pos, width, "A-V:%7.3f ct:%7.3f ", a_v, corr);

    // Video stats
    if (sh_video)
        saddf(line, &pos, width, "%3d/%3d ",
              (int)sh_video->num_frames,
              (int)sh_video->num_frames_decoded);

    // CPU usage
    if (sh_video) {
        if (sh_video->timer > 0.5)
            saddf(line, &pos, width, "%2d%% %2d%% %4.1f%% ",
                  (int)(100.0 * video_time_usage * playback_speed / (double)sh_video->timer),
                  (int)(100.0 * vout_time_usage * playback_speed / (double)sh_video->timer),
                  (100.0 * audio_time_usage * playback_speed / (double)sh_video->timer));
        else
            saddf(line, &pos, width, "??%% ??%% ??,?%% ");
    } else if (mpctx->sh_audio) {
        if (mpctx->delay > 0.5)
            saddf(line, &pos, width, "%4.1f%% ",
                  100.0 * audio_time_usage / (double)mpctx->delay);
        else
            saddf(line, &pos, width, "??,?%% ");
    }

    // VO stats
    if (sh_video)
        saddf(line, &pos, width, "%d %d ", drop_frame_cnt, output_quality);

#ifdef CONFIG_STREAM_CACHE
    // cache stats
    if (stream_cache_size > 0)
        saddf(line, &pos, width, "%d%% ", cache_fill_status(mpctx->stream));
#endif

    // other
    if (playback_speed != 1)
        saddf(line, &pos, width, "%4.2fx ", playback_speed);

    // end
    if (erase_to_end_of_line) {
        line[pos] = 0;
        mp_msg(MSGT_STATUSLINE, MSGL_STATUS, "%s%s\r", line, erase_to_end_of_line);
    } else {
        memset(&line[pos], ' ', width - pos);
        line[width] = 0;
        mp_msg(MSGT_STATUSLINE, MSGL_STATUS, "%s\r", line);
    }
    free(line);
}

static void stream_dump_progress_start(void)
{
    stream_dump_start_time = stream_dump_last_print_time = GetTimerMS();
}

static void stream_dump_progress(uint64_t len, stream_t *stream)
{
    unsigned t = GetTimerMS();
    uint64_t start = stream->start_pos;
    uint64_t end   = stream->end_pos;
    uint64_t pos   = stream->pos;

    stream_dump_count += len;
    if (t - stream_dump_last_print_time - 1000 > UINT_MAX / 2)
        return;
    stream_dump_last_print_time = t;
    /* TODO: pretty print sizes; ETA */
    if (end > start && pos >= start && pos <= end) {
        mp_msg(MSGT_STATUSLINE, MSGL_STATUS, MSGTR_DumpBytesWrittenPercent,
               stream_dump_count, 100.0 * (pos - start) / (end - start));
    } else {
        mp_msg(MSGT_STATUSLINE, MSGL_STATUS, MSGTR_DumpBytesWritten,
               stream_dump_count);
    }
}

static void stream_dump_progress_end(void)
{
    mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_DumpBytesWrittenTo,
           stream_dump_count, stream_dump_name);
}

/**
 * @brief Build a chain of audio filters that converts the input format.
 * to the ao's format, taking into account the current playback_speed.
 * @param sh_audio describes the requested input format of the chain.
 * @param ao_data describes the requested output format of the chain.
 */
static int build_afilter_chain(sh_audio_t *sh_audio, ao_data_t *ao_data)
{
    int new_srate;
    int result;
    if (!sh_audio) {
#ifdef CONFIG_GUI
        if (use_gui)
            gui(GUI_SET_AFILTER, NULL);
#endif
        mpctx->mixer.afilter = NULL;
        return 0;
    }
    if (af_control_any_rev(sh_audio->afilter,
                           AF_CONTROL_PLAYBACK_SPEED | AF_CONTROL_SET,
                           &playback_speed)) {
        new_srate = sh_audio->samplerate;
    } else {
        new_srate = sh_audio->samplerate * playback_speed;
        if (new_srate != ao_data->samplerate) {
            // limits are taken from libaf/af_resample.c
            if (new_srate < 8000)
                new_srate = 8000;
            if (new_srate > 192000)
                new_srate = 192000;
            playback_speed = (float)new_srate / (float)sh_audio->samplerate;
        }
    }
    result = init_audio_filters(sh_audio, new_srate,
                                &ao_data->samplerate, &ao_data->channels, &ao_data->format);
    mpctx->mixer.afilter = sh_audio->afilter;
#ifdef CONFIG_GUI
    if (use_gui)
        gui(GUI_SET_AFILTER, sh_audio->afilter);
#endif
    return result;
}

typedef struct mp_osd_msg mp_osd_msg_t;
struct mp_osd_msg {
    mp_osd_msg_t *prev; ///< Previous message on the stack
    char msg[128];      ///< Message text
    int id, level, started;
    unsigned time;      ///< Display duration in ms
};

static mp_osd_msg_t *osd_msg_stack; ///< OSD message stack

/**
 * @brief Add a message on the OSD message stack.
 *
 * If a message with the same id is already present in the stack
 * it is pulled on top of the stack, otherwise a new message is created.
 */
void set_osd_msg(int id, int level, int time, const char *fmt, ...)
{
    mp_osd_msg_t *msg, *last = NULL;
    va_list va;
    int r;

    // look if the id is already in the stack
    for (msg = osd_msg_stack; msg && msg->id != id;
         last = msg, msg = msg->prev) ;
    // not found: alloc it
    if (!msg) {
        msg           = calloc(1, sizeof(mp_osd_msg_t));
        msg->prev     = osd_msg_stack;
        osd_msg_stack = msg;
    } else if (last) { // found, but it's not on top of the stack
        last->prev    = msg->prev;
        msg->prev     = osd_msg_stack;
        osd_msg_stack = msg;
    }
    // write the msg
    va_start(va, fmt);
    r = vsnprintf(msg->msg, 128, fmt, va);
    va_end(va);
    if (r >= 128)
        msg->msg[127] = 0;
    // set id and time
    msg->id    = id;
    msg->level = level;
    msg->time  = time;
}

/**
 * @brief Remove a message from the OSD stack.
 *
 * This function can be used to get rid of a message right away.
 */
void rm_osd_msg(int id)
{
    mp_osd_msg_t *msg, *last = NULL;

    // Search for the msg
    for (msg = osd_msg_stack; msg && msg->id != id;
         last = msg, msg = msg->prev) ;
    if (!msg)
        return;

    // Detach it from the stack and free it
    if (last)
        last->prev = msg->prev;
    else
        osd_msg_stack = msg->prev;
    free(msg);
}

/**
 * @brief Remove all messages from the OSD stack.
 */
static void clear_osd_msgs(void)
{
    mp_osd_msg_t *msg = osd_msg_stack, *prev = NULL;
    while (msg) {
        prev = msg->prev;
        free(msg);
        msg = prev;
    }
    osd_msg_stack = NULL;
}

/**
 * @brief Get the current message from the OSD stack.
 *
 * This function decrements the message timer and destroys the old ones.
 * The message that should be displayed is returned (if any).
 */
static mp_osd_msg_t *get_osd_msg(void)
{
    mp_osd_msg_t *msg, *prev, *last = NULL;
    static unsigned last_update;
    unsigned now = GetTimerMS();
    unsigned diff;
    char hidden_dec_done = 0;

    if (osd_visible) {
        // 36000000 means max timed visibility is 1 hour into the future, if
        // the difference is greater assume it's wrapped around from below 0
        if (osd_visible - now > 36000000) {
            osd_visible = 0;
            vo_osd_progbar_type = -1; // disable
            vo_osd_changed(OSDTYPE_PROGBAR);
            if (mpctx->osd_function != OSD_PAUSE)
                mpctx->osd_function = OSD_PLAY;
        }
    }

    if (!last_update)
        last_update = now;
    diff = now >= last_update ? now - last_update : 0;

    last_update = now;

    // Look for the first message in the stack with high enough level.
    for (msg = osd_msg_stack; msg; last = msg, msg = prev) {
        prev = msg->prev;
        if (msg->level > osd_level && hidden_dec_done)
            continue;
        // The message has a high enough level or it is the first hidden one
        // in both cases we decrement the timer or kill it.
        if (!msg->started || msg->time > diff) {
            if (msg->started)
                msg->time -= diff;
            else
                msg->started = 1;
            // display it
            if (msg->level <= osd_level)
                return msg;
            hidden_dec_done = 1;
            continue;
        }
        // kill the message
        free(msg);
        if (last) {
            last->prev = prev;
            msg = last;
        } else {
            osd_msg_stack = prev;
            msg = NULL;
        }
    }
    // Nothing found
    return NULL;
}

/**
 * @brief Display the OSD bar.
 *
 * Display the OSD bar or fall back on a simple message.
 */
void set_osd_bar(int type, const char *name, double min, double max, double val)
{
    if (osd_level < 1)
        return;

    if (mpctx->sh_video) {
        osd_visible = (GetTimerMS() + 1000) | 1;
        vo_osd_progbar_type  = type;
        vo_osd_progbar_value = 256 * (val - min) / (max - min);
        vo_osd_changed(OSDTYPE_PROGBAR);
        return;
    }

    set_osd_msg(OSD_MSG_BAR, 1, osd_duration, "%s: %d %%",
                name, ROUND(100 * (val - min) / (max - min)));
}

/**
 * @brief Display text subtitles on the OSD.
 */
void set_osd_subtitle(subtitle *subs)
{
    int i;
    vo_sub = subs;
    vo_osd_changed(OSDTYPE_SUBTITLE);
    if (!mpctx->sh_video) {
        // reverse order, since newest set_osd_msg is displayed first
        for (i = SUB_MAX_TEXT - 1; i >= 0; i--) {
            if (!subs || i >= subs->lines || !subs->text[i])
                rm_osd_msg(OSD_MSG_SUB_BASE + i);
            else {
                // HACK: currently display time for each sub line except the last is set to 2 seconds.
                int display_time = i == subs->lines - 1 ? 180000 : 2000;
                set_osd_msg(OSD_MSG_SUB_BASE + i, 1, display_time, "%s", subs->text[i]);
            }
        }
    }
}

/**
 * @brief Update the OSD message line.
 *
 * This function displays the current message on the vo OSD or on the term.
 * If the stack is empty and the OSD level is high enough the timer
 * is displayed (only on the vo OSD).
 */
static void update_osd_msg(void)
{
    mp_osd_msg_t *msg;
    static char osd_text[128] = "";
    static char osd_text_timer[128];

    // we need some mem for vo_osd_text
    vo_osd_text = (unsigned char *)osd_text;

    // Look if we have a msg
    if ((msg = get_osd_msg())) {
        if (strcmp(osd_text, msg->msg)) {
            strncpy((char *)osd_text, msg->msg, 127);
            if (mpctx->sh_video)
                vo_osd_changed(OSDTYPE_OSD);
            else if (term_osd)
                mp_msg(MSGT_CPLAYER, MSGL_STATUS, "%s%s\n", term_osd_esc, msg->msg);
        }
        return;
    }

    if (mpctx->sh_video) {
        // fallback on the timer
        if (osd_level >= 2) {
            int len = demuxer_get_time_length(mpctx->demuxer);
            int percentage = -1;
            char percentage_text[10];
            char fractions_text[4];
            double pts = demuxer_get_current_time(mpctx->demuxer);
            int pts_seconds = pts;

            if (mpctx->osd_show_percentage)
                percentage = demuxer_get_percent_pos(mpctx->demuxer);

            if (percentage >= 0)
                snprintf(percentage_text, 9, " (%d%%)", percentage);
            else
                percentage_text[0] = 0;

            if (osd_fractions == 1) {
                // print fractions as sub-second timestamp
                snprintf(fractions_text, sizeof(fractions_text), ".%02d",
                         (int)((pts - pts_seconds) * 100) % 100);
            } else if (osd_fractions == 2) {
                // print fractions by estimating the frame count within the
                // second

                // rounding or cutting off numbers after the decimal point
                // causes problems because of float's precision and movies,
                // whose first frame is not exactly at timestamp 0. Therefore,
                // we add 0.2 and cut off at the decimal point, which proved
                // as good heuristic
                snprintf(fractions_text, sizeof(fractions_text), ".%02d",
                         (int)((pts - pts_seconds) *
                               mpctx->sh_video->fps + 0.2));
            } else {
                // do not print fractions
                fractions_text[0] = 0;
            }

            if (osd_level == 3)
                snprintf(osd_text_timer, 63,
                         "%c %02d:%02d:%02d%s / %02d:%02d:%02d%s",
                         mpctx->osd_function, pts_seconds / 3600, (pts_seconds / 60) % 60, pts_seconds % 60,
                         fractions_text, len / 3600, (len / 60) % 60, len % 60,
                         percentage_text);
            else
                snprintf(osd_text_timer, 63, "%c %02d:%02d:%02d%s%s",
                         mpctx->osd_function, pts_seconds / 3600, (pts_seconds / 60) % 60,
                         pts_seconds % 60, fractions_text, percentage_text);
        } else
            osd_text_timer[0] = 0;

        // always decrement the percentage timer
        if (mpctx->osd_show_percentage)
            mpctx->osd_show_percentage--;

        if (strcmp(osd_text, osd_text_timer)) {
            strncpy(osd_text, osd_text_timer, 63);
            vo_osd_changed(OSDTYPE_OSD);
        }
        return;
    }

    // Clear the term osd line
    if (term_osd && osd_text[0]) {
        osd_text[0] = 0;
        printf("%s\n", term_osd_esc);
    }
}

void reinit_audio_chain(void)
{
    if (!mpctx->sh_audio)
        return;
    if (!(initialized_flags & INITIALIZED_ACODEC)) {
        current_module = "init_audio_codec";
        mp_msg(MSGT_CPLAYER, MSGL_INFO, "==========================================================================\n");
        if (!init_best_audio_codec(mpctx->sh_audio, audio_codec_list, audio_fm_list))
            goto init_error;
        initialized_flags |= INITIALIZED_ACODEC;
        mp_msg(MSGT_CPLAYER, MSGL_INFO, "==========================================================================\n");
    }

    if (!(initialized_flags & INITIALIZED_AO)) {
        current_module     = "af_preinit";
        ao_data.samplerate = force_srate;
        ao_data.channels   = 0;
        ao_data.format     = audio_output_format;
        // first init to detect best values
        if (!init_audio_filters(mpctx->sh_audio,  // preliminary init
                                // input:
                                mpctx->sh_audio->samplerate,
                                // output:
                                &ao_data.samplerate, &ao_data.channels, &ao_data.format)) {
            mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_AudioFilterChainPreinitError);
            exit_player(EXIT_ERROR);
        }
        current_module   = "ao2_init";
        mpctx->audio_out = init_best_audio_out(audio_driver_list,
                                               0, // plugin flag
                                               ao_data.samplerate,
                                               ao_data.channels,
                                               ao_data.format, 0);
        if (!mpctx->audio_out) {
            mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_CannotInitAO);
            goto init_error;
        }
        initialized_flags |= INITIALIZED_AO;
        mp_msg(MSGT_CPLAYER, MSGL_INFO, "AO: [%s] %dHz %dch %s (%d bytes per sample)\n",
               mpctx->audio_out->info->short_name,
               ao_data.samplerate, ao_data.channels,
               af_fmt2str_short(ao_data.format),
               af_fmt2bits(ao_data.format) / 8);
        mp_msg(MSGT_CPLAYER, MSGL_V, "AO: Description: %s\nAO: Author: %s\n",
               mpctx->audio_out->info->name, mpctx->audio_out->info->author);
        if (strlen(mpctx->audio_out->info->comment) > 0)
            mp_msg(MSGT_CPLAYER, MSGL_V, "AO: Comment: %s\n", mpctx->audio_out->info->comment);
    }

    // init audio filters:
    current_module = "af_init";
    if (!build_afilter_chain(mpctx->sh_audio, &ao_data)) {
        mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_NoMatchingFilter);
        goto init_error;
    }
    mpctx->mixer.audio_out = mpctx->audio_out;
    mpctx->mixer.volstep   = volstep;
    return;

init_error:
    uninit_player(INITIALIZED_ACODEC | INITIALIZED_AO); // close codec and possibly AO
    mpctx->sh_audio    = mpctx->d_audio->sh = NULL; // -> nosound
    mpctx->d_audio->id = -2;
}

// Return pts value corresponding to the end point of audio written to the
// ao so far.
static double written_audio_pts(sh_audio_t *sh_audio, demux_stream_t *d_audio)
{
    double buffered_output;
    // first calculate the end pts of audio that has been output by decoder
    double a_pts = calc_a_pts(sh_audio, d_audio);
    // Now a_pts hopefully holds the pts for end of audio from decoder.
    // Substract data in buffers between decoder and audio out.

    // Decoded but not filtered
    a_pts -= sh_audio->a_buffer_len / (double)sh_audio->o_bps;

    // Data buffered in audio filters, measured in bytes of "missing" output
    buffered_output = af_calc_delay(sh_audio->afilter);

    // Data that was ready for ao but was buffered because ao didn't fully
    // accept everything to internal buffers yet
    buffered_output += sh_audio->a_out_buffer_len;

    // Filters divide audio length by playback_speed, so multiply by it
    // to get the length in original units without speedup or slowdown
    a_pts -= buffered_output * playback_speed / ao_data.bps;

    return a_pts;
}

// Return pts value corresponding to currently playing audio.
double playing_audio_pts(sh_audio_t *sh_audio, demux_stream_t *d_audio,
                         const ao_functions_t *audio_out)
{
    return written_audio_pts(sh_audio, d_audio) - playback_speed *
           audio_out->get_delay();
}

static int is_at_end(MPContext *mpctx, m_time_size_t *end_at, double pts)
{
    switch (end_at->type) {
    case END_AT_TIME: return pts != MP_NOPTS_VALUE && end_at->pos <= pts;
    case END_AT_SIZE: return end_at->pos <= stream_tell(mpctx->stream);
    }
    return 0;
}

static int check_framedrop(double frame_time)
{
    // check for frame-drop:
    current_module = "check_framedrop";
    if (mpctx->sh_audio && !mpctx->d_audio->eof) {
        static int dropped_frames;
        float delay = playback_speed * mpctx->audio_out->get_delay();
        float d     = delay - mpctx->delay;
        ++total_frame_cnt;
        // we should avoid dropping too many frames in sequence unless we
        // are too late. and we allow 100ms A-V delay here:
        if (d < -dropped_frames * frame_time - 0.100 &&
            mpctx->osd_function != OSD_PAUSE) {
            ++drop_frame_cnt;
            ++dropped_frames;
            return frame_dropping;
        } else
            dropped_frames = 0;
    }
    return 0;
}

static int generate_video_frame(sh_video_t *sh_video, demux_stream_t *d_video)
{
    unsigned char *start;
    int in_size;
    int hit_eof = 0;
    double pts;

    while (1) {
        int drop_frame = check_framedrop(sh_video->frametime);
        void *decoded_frame;
        current_module = "decode video";
        // XXX Time used in this call is not counted in any performance
        // timer now, OSD is not updated correctly for filter-added frames
        if (vf_output_queued_frame(sh_video->vfilter))
            break;
        current_module = "video_read_frame";
        in_size = ds_get_packet_pts(d_video, &start, &pts);
        if (in_size < 0) {
            // try to extract last frames in case of decoder lag
            in_size = 0;
            start   = NULL;
            pts     = MP_NOPTS_VALUE;
            hit_eof = 1;
            drop_frame = 0;
        }
        if (in_size > max_framesize)
            max_framesize = in_size;
        current_module = "decode video";
        decoded_frame  = decode_video(sh_video, start, in_size, drop_frame, pts, NULL);
        if (decoded_frame) {
            update_subtitles(sh_video, sh_video->pts, mpctx->d_sub, 0);
            update_teletext(sh_video, mpctx->demuxer, 0);
            update_osd_msg();
            current_module = "filter video";
            if (filter_video(sh_video, decoded_frame, sh_video->pts))
                break;
        } else if (drop_frame)
            return -1;
        if (hit_eof)
            return 0;
    }
    return 1;
}

#ifdef HAVE_RTC
int rtc_fd = -1;
#endif

static float timing_sleep(float time_frame)
{
#ifdef HAVE_RTC
    if (rtc_fd >= 0) {
        // -------- RTC -----------
        current_module = "sleep_rtc";
        while (time_frame > 0.000) {
            unsigned long rtc_ts;
            if (read(rtc_fd, &rtc_ts, sizeof(rtc_ts)) <= 0)
                mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_LinuxRTCReadError, strerror(errno));
            time_frame -= GetRelativeTime();
        }
    } else
#endif
    {
        // assume kernel HZ=100 for softsleep, works with larger HZ but with
        // unnecessarily high CPU usage
        float margin = softsleep ? 0.011 : 0;
        current_module = "sleep_timer";
        while (time_frame > margin) {
            usec_sleep(1000000 * (time_frame - margin));
            time_frame -= GetRelativeTime();
        }
        if (softsleep) {
            current_module = "sleep_soft";
            if (time_frame < 0)
                mp_msg(MSGT_AVSYNC, MSGL_WARN, MSGTR_SoftsleepUnderflow);
            while (time_frame > 0)
                time_frame -= GetRelativeTime();  // burn the CPU
        }
    }
    return time_frame;
}

static int select_subtitle(MPContext *mpctx)
{
    // find the best sub to use
    int id;
    int found = 0;
    mpctx->global_sub_pos = -1; // no subs by default
    if (vobsub_id >= 0) {
        // if user asks for a vobsub id, use that first.
        id    = vobsub_id;
        found = mp_property_do("sub_vob", M_PROPERTY_SET, &id, mpctx) == M_PROPERTY_OK;
    }

    if (!found && dvdsub_id >= 0) {
        // if user asks for a dvd sub id, use that next.
        id    = dvdsub_id;
        found = mp_property_do("sub_demux", M_PROPERTY_SET, &id, mpctx) == M_PROPERTY_OK;
    }

    if (!found) {
        // if there are text subs to use, use those.  (autosubs come last here)
        id    = 0;
        found = mp_property_do("sub_file", M_PROPERTY_SET, &id, mpctx) == M_PROPERTY_OK;
    }

    if (!found && dvdsub_id == -1) {
        // finally select subs by language and container hints
        if (dvdsub_id == -1 && dvdsub_lang)
            dvdsub_id = demuxer_sub_track_by_lang(mpctx->demuxer, dvdsub_lang);
        if (dvdsub_id == -1)
            dvdsub_id = demuxer_default_sub_track(mpctx->demuxer);
        if (dvdsub_id >= 0) {
            id    = dvdsub_id;
            found = mp_property_do("sub_demux", M_PROPERTY_SET, &id, mpctx) == M_PROPERTY_OK;
        }
    }
    return found;
}

#ifdef CONFIG_DVDNAV
#ifndef FF_B_TYPE
#define FF_B_TYPE 3
#endif
/**
 * @brief Store decoded video image.
 */
static mp_image_t *mp_dvdnav_copy_mpi(mp_image_t *to_mpi,
                                      mp_image_t *from_mpi)
{
    mp_image_t *mpi;

    // do not store B-frames
    if (from_mpi->pict_type == FF_B_TYPE)
        return to_mpi;

    if (to_mpi &&
        to_mpi->w == from_mpi->w &&
        to_mpi->h == from_mpi->h &&
        to_mpi->imgfmt == from_mpi->imgfmt)
        mpi = to_mpi;
    else {
        if (to_mpi)
            free_mp_image(to_mpi);
        if (from_mpi->w == 0 || from_mpi->h == 0)
            return NULL;
        mpi = alloc_mpi(from_mpi->w, from_mpi->h, from_mpi->imgfmt);
    }

    copy_mpi(mpi, from_mpi);
    return mpi;
}

static void mp_dvdnav_reset_stream(MPContext *ctx)
{
    if (ctx->sh_video) {
        // clear video pts
        ctx->d_video->pts       = 0.0f;
        ctx->sh_video->pts      = 0.0f;
        ctx->sh_video->i_pts    = 0.0f;
        ctx->sh_video->last_pts = 0.0f;
        ctx->sh_video->num_buffered_pts   = 0;
        ctx->sh_video->num_frames         = 0;
        ctx->sh_video->num_frames_decoded = 0;
        ctx->sh_video->timer = 0.0f;
        ctx->sh_video->stream_delay = 0.0f;
        ctx->sh_video->timer     = 0;
        ctx->demuxer->stream_pts = MP_NOPTS_VALUE;
    }

    if (ctx->sh_audio) {
        // free audio packets and reset
        ds_free_packs(ctx->d_audio);
        audio_delay -= ctx->sh_audio->stream_delay;
        ctx->delay   = -audio_delay;
        ctx->audio_out->reset();
        resync_audio_stream(ctx->sh_audio);
    }

    audio_delay = 0.0f;
    mpctx->sub_counts[SUB_SOURCE_DEMUX] = mp_dvdnav_number_of_subs(mpctx->stream);
    if (dvdsub_lang && dvdsub_id == dvdsub_lang_id) {
        dvdsub_lang_id = mp_dvdnav_sid_from_lang(ctx->stream, dvdsub_lang);
        if (dvdsub_lang_id != dvdsub_id) {
            dvdsub_id = dvdsub_lang_id;
            select_subtitle(ctx);
        }
    }

    // clear all EOF related flags
    ctx->d_video->eof = ctx->d_audio->eof = ctx->stream->eof = 0;
}

/**
 * @brief Restore last decoded DVDNAV (still frame).
 */
static mp_image_t *mp_dvdnav_restore_smpi(int *in_size,
                                          unsigned char **start,
                                          mp_image_t *decoded_frame)
{
    if (mpctx->stream->type != STREAMTYPE_DVDNAV)
        return decoded_frame;

    // a change occurred in dvdnav stream
    if (mp_dvdnav_cell_has_changed(mpctx->stream, 0)) {
        mp_dvdnav_read_wait(mpctx->stream, 1, 1);
        mp_dvdnav_context_free(mpctx);
        mp_dvdnav_reset_stream(mpctx);
        mp_dvdnav_read_wait(mpctx->stream, 0, 1);
        mp_dvdnav_cell_has_changed(mpctx->stream, 1);
    }

    if (*in_size < 0) {
        float len;

        // display still frame, if any
        if (mpctx->nav_smpi && !mpctx->nav_buffer)
            decoded_frame = mpctx->nav_smpi;

        // increment video frame: continue playing after still frame
        len = demuxer_get_time_length(mpctx->demuxer);
        if (mpctx->sh_video->pts >= len &&
            mpctx->sh_video->pts > 0.0 && len > 0.0) {
            mp_dvdnav_skip_still(mpctx->stream);
            mp_dvdnav_skip_wait(mpctx->stream);
        }
        mpctx->sh_video->pts += 1 / mpctx->sh_video->fps;

        if (mpctx->nav_buffer) {
            *start   = mpctx->nav_start;
            *in_size = mpctx->nav_in_size;
            if (mpctx->nav_start)
                memcpy(*start, mpctx->nav_buffer, mpctx->nav_in_size);
        }
    }

    return decoded_frame;
}

/**
 * @brief Save last decoded DVDNAV (still frame).
 */
static void mp_dvdnav_save_smpi(int in_size,
                                unsigned char *start,
                                mp_image_t *decoded_frame)
{
    if (mpctx->stream->type != STREAMTYPE_DVDNAV)
        return;

    free(mpctx->nav_buffer);
    mpctx->nav_buffer  = NULL;
    mpctx->nav_start   = NULL;
    mpctx->nav_in_size = -1;

    if (in_size > 0)
        mpctx->nav_buffer = malloc(in_size);
    if (mpctx->nav_buffer) {
        mpctx->nav_start   = start;
        mpctx->nav_in_size = in_size;
        memcpy(mpctx->nav_buffer, start, in_size);
    }

    if (decoded_frame && mpctx->nav_smpi != decoded_frame)
        mpctx->nav_smpi = mp_dvdnav_copy_mpi(mpctx->nav_smpi, decoded_frame);
}

#endif /* CONFIG_DVDNAV */

static void adjust_sync_and_print_status(int between_frames, float timing_error)
{
    current_module = "av_sync";

    if (mpctx->sh_audio) {
        double a_pts, v_pts;

        if (autosync)
            /*
             * If autosync is enabled, the value for delay must be calculated
             * a bit differently.  It is set only to the difference between
             * the audio and video timers.  Any attempt to include the real
             * or corrected delay causes the pts_correction code below to
             * try to correct for the changes in delay which autosync is
             * trying to measure.  This keeps the two from competing, but still
             * allows the code to correct for PTS drift *only*.  (Using a delay
             * value here, even a "corrected" one, would be incompatible with
             * autosync mode.)
             */
            a_pts = written_audio_pts(mpctx->sh_audio, mpctx->d_audio) - mpctx->delay;
        else
            a_pts = playing_audio_pts(mpctx->sh_audio, mpctx->d_audio, mpctx->audio_out);

        v_pts = mpctx->sh_video->pts;

        {
            static int drop_message;
            double AV_delay = a_pts - audio_delay - v_pts;
            double x;
            // not a good idea to do A-V correction with with bogus values
            if (a_pts == MP_NOPTS_VALUE || v_pts == MP_NOPTS_VALUE)
                AV_delay = 0;
            if (AV_delay > 0.5 && drop_frame_cnt > 50 && drop_message == 0) {
                ++drop_message;
                mp_msg(MSGT_AVSYNC, MSGL_WARN, MSGTR_SystemTooSlow);
            }
            if (AV_delay > 0.5 && correct_pts && mpctx->delay < -audio_delay - 30) {
                // This case means that we are supposed to stop video for a long
                // time, even though audio is already ahead.
                // This happens e.g. when initial audio pts is 10000, video
                // starts at 0 but suddenly jumps to match audio.
                // This is common in ogg streams.
                // Only check for -correct-pts since this case does not cause
                // issues with -nocorrect-pts.
                mp_msg(MSGT_AVSYNC, MSGL_WARN, "Timing looks severely broken, resetting\n");
                AV_delay = 0;
                timing_error = 0;
                mpctx->delay = -audio_delay;
            }
            if (autosync)
                x = AV_delay * 0.1f;
            else
                /* Do not correct target time for the next frame if this frame
                 * was late not because of wrong target time but because the
                 * target time could not be met */
                x = (AV_delay + timing_error * playback_speed) * 0.1f;
            if (x < -max_pts_correction)
                x = -max_pts_correction;
            else if (x > max_pts_correction)
                x = max_pts_correction;
            if (default_max_pts_correction >= 0)
                max_pts_correction = default_max_pts_correction;
            else
                max_pts_correction = mpctx->sh_video->frametime * 0.10;  // +-10% of time
            if (!between_frames) {
                mpctx->delay += x;
                c_total      += x;
            }
            if (!quiet)
                print_status(a_pts - audio_delay, AV_delay, c_total);
        }
    } else {
        // No audio:

        if (!quiet)
            print_status(0, 0, 0);
    }
}

static int fill_audio_out_buffers(void)
{
    unsigned int t;
    double tt;
    int playsize;
    int playflags = 0;
    int audio_eof = 0;
    int bytes_to_write;
    int format_change = 0;
    int timeout = 0;
    sh_audio_t *const sh_audio = mpctx->sh_audio;

    current_module = "play_audio";

    while (1) {
        int sleep_time;
        // all the current uses of ao_data.pts seem to be in aos that handle
        // sync completely wrong; there should be no need to use ao_data.pts
        // in get_space()
        ao_data.pts    = ((mpctx->sh_video ? mpctx->sh_video->timer : 0) + mpctx->delay) * 90000.0;
        bytes_to_write = mpctx->audio_out->get_space();
        if (mpctx->sh_video || bytes_to_write >= ao_data.outburst)
            break;
        if (timeout++ > 10) {
            mp_msg(MSGT_CPLAYER, MSGL_WARN, MSGTR_AudioDeviceStuck);
            break;
        }

        // handle audio-only case:
        // this is where mplayer sleeps during audio-only playback
        // to avoid 100% CPU use
        sleep_time = (ao_data.outburst - bytes_to_write) * 1000 / ao_data.bps;
        if (sleep_time < 10)
            sleep_time = 10;                  // limit to 100 wakeups per second
        usec_sleep(sleep_time * 1000);
    }

    while (bytes_to_write) {
        int res;
        playsize = bytes_to_write;
        if (playsize > MAX_OUTBURST)
            playsize = MAX_OUTBURST;
        bytes_to_write -= playsize;

        // Fill buffer if needed:
        current_module = "decode_audio";
        t = GetTimer();
        if (!format_change) {
            res = mp_decode_audio(sh_audio, playsize);
            format_change = res == -2;
        }
        if (!format_change && res < 0) // EOF or error
            if (mpctx->d_audio->eof) {
                audio_eof = 1;
                if (sh_audio->a_out_buffer_len == 0)
                    return 0;
            }
        t  = GetTimer() - t;
        tt = t * 0.000001f;
        audio_time_usage += tt;
        if (playsize > sh_audio->a_out_buffer_len) {
            playsize = sh_audio->a_out_buffer_len;
            if (audio_eof || format_change)
                playflags |= AOPLAY_FINAL_CHUNK;
        }
        if (!playsize)
            break;

        // play audio:
        current_module = "play_audio";

        // Is this pts value actually useful for the aos that access it?
        // They're obviously badly broken in the way they handle av sync;
        // would not having access to this make them more broken?
        ao_data.pts = ((mpctx->sh_video ? mpctx->sh_video->timer : 0) + mpctx->delay) * 90000.0;
        playsize    = mpctx->audio_out->play(sh_audio->a_out_buffer, playsize, playflags);

        if (playsize > 0) {
            sh_audio->a_out_buffer_len -= playsize;
            memmove(sh_audio->a_out_buffer, &sh_audio->a_out_buffer[playsize],
                    sh_audio->a_out_buffer_len);
            mpctx->delay += playback_speed * playsize / (double)ao_data.bps;
        } else if ((format_change || audio_eof) && mpctx->audio_out->get_delay() < .04) {
            // Sanity check to avoid hanging in case current ao doesn't output
            // partial chunks and doesn't check for AOPLAY_FINAL_CHUNK
            mp_msg(MSGT_CPLAYER, MSGL_WARN, MSGTR_AudioOutputTruncated);
            sh_audio->a_out_buffer_len = 0;
        }
    }
    if (format_change) {
        uninit_player(INITIALIZED_AO);
        reinit_audio_chain();
    }
    return 1;
}

static void handle_udp_master(double time)
{
#ifdef CONFIG_NETWORKING
    if (udp_master) {
        char current_time[256];
        snprintf(current_time, sizeof(current_time), "%f", time);
        send_udp(udp_ip, udp_port, current_time);
    }
#endif /* CONFIG_NETWORKING */
}

static int sleep_until_update(float *time_frame, float *aq_sleep_time)
{
    int frame_time_remaining = 0;
    current_module = "calc_sleep_time";

#ifdef CONFIG_NETWORKING
    if (udp_slave) {
        int udp_master_exited = udp_slave_sync(mpctx);
        if (udp_master_exited > 0) {
            mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_MasterQuit);
            exit_player(EXIT_QUIT);
        } else if (udp_master_exited == 0)
            return 0;
    }
#endif /* CONFIG_NETWORKING */

    *time_frame -= GetRelativeTime(); // reset timer

    if (mpctx->sh_audio && !mpctx->d_audio->eof) {
        float delay = mpctx->audio_out->get_delay();
        mp_dbg(MSGT_AVSYNC, MSGL_DBG2, "delay=%f\n", delay);

        if (autosync) {
            /*
             * Adjust this raw delay value by calculating the expected
             * delay for this frame and generating a new value which is
             * weighted between the two.  The higher autosync is, the
             * closer to the delay value gets to that which "-nosound"
             * would have used, and the longer it will take for A/V
             * sync to settle at the right value (but it eventually will.)
             * This settling time is very short for values below 100.
             */
            float predicted  = mpctx->delay / playback_speed + *time_frame;
            float difference = delay - predicted;
            delay = predicted + difference / (float)autosync;
        }

        *time_frame = delay - mpctx->delay / playback_speed;

        // delay = amount of audio buffered in soundcard/driver
        if (delay > 0.25)
            delay = 0.25;
        else if (delay < 0.10)
            delay = 0.10;
        if (*time_frame > delay * 0.6) {
            // sleep time too big - may cause audio drops (buffer underrun)
            frame_time_remaining = 1;
            *time_frame = delay * 0.5;
        }
    } else {
        // If we're lagging more than 200 ms behind the right playback rate,
        // don't try to "catch up".
        // If benchmark is set always output frames as fast as possible
        // without sleeping.
        if (*time_frame < -0.2 || benchmark)
            *time_frame = 0;
    }

    *aq_sleep_time += *time_frame;

    //============================== SLEEP: ===================================

    // flag 256 means: libvo driver does its timing (dvb card)
    if (!(vo_flags & 256)) {
        if (*time_frame > 0.3) {
            // Avoid sleeping too long without reacting to user input
            usec_sleep(200000);
            *time_frame -= GetRelativeTime();
            frame_time_remaining = 1;
	} else if (*time_frame > 0.001)
            *time_frame = timing_sleep(*time_frame);
    }

    handle_udp_master(mpctx->sh_video->pts);

    return frame_time_remaining;
}

int reinit_video_chain(void)
{
    sh_video_t *const sh_video = mpctx->sh_video;
    double ar = -1.0;
    //================== Init VIDEO (codec & libvo) ==========================
    if (!fixed_vo || !(initialized_flags & INITIALIZED_VO)) {
        current_module = "preinit_libvo";

        //shouldn't we set dvideo->id=-2 when we fail?
        vo_config_count = 0;
        //if((mpctx->video_out->preinit(vo_subdevice))!=0){
        if (!(mpctx->video_out = init_best_video_out(video_driver_list))) {
            mp_msg(MSGT_CPLAYER, MSGL_FATAL, MSGTR_ErrorInitializingVODevice);
            goto err_out;
        }
        initialized_flags |= INITIALIZED_VO;
    }

    if (stream_control(mpctx->demuxer->stream, STREAM_CTRL_GET_ASPECT_RATIO, &ar) != STREAM_UNSUPPORTED)
        mpctx->sh_video->stream_aspect = ar;
    current_module = "init_video_filters";
    {
        char *vf_arg[] = { "_oldargs_", (char *)mpctx->video_out, NULL };
        sh_video->vfilter = vf_open_filter(NULL, "vo", vf_arg);
    }
#ifdef CONFIG_MENU
    if (use_menu) {
        char *vf_arg[] = { "_oldargs_", menu_root, NULL };
        vf_menu = vf_open_plugin(libmenu_vfs, sh_video->vfilter, "menu", vf_arg);
        if (!vf_menu) {
            mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_CantOpenLibmenuFilterWithThisRootMenu, menu_root);
            use_menu = 0;
        }
    }
    if (vf_menu)
        sh_video->vfilter = vf_menu;
#endif

#ifdef CONFIG_ASS
    if (ass_enabled) {
        int i;
        int insert = 1;
        if (vf_settings)
            for (i = 0; vf_settings[i].name; ++i)
                if (strcmp(vf_settings[i].name, "ass") == 0) {
                    insert = 0;
                    break;
                }
        if (insert) {
            char *vf_arg[] = { "auto", "1", NULL };
            vf_instance_t *vf_ass = vf_open_filter(sh_video->vfilter, "ass", vf_arg);
            if (vf_ass)
                sh_video->vfilter = vf_ass;
            else
                mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_ASSCannotAddVideoFilter);
        }
    }
#endif

    sh_video->vfilter = append_filters(sh_video->vfilter);
    eosd_init(sh_video->vfilter);

#ifdef CONFIG_ASS
    if (ass_enabled)
        eosd_ass_init(ass_library);
#endif

    current_module = "init_video_codec";

    mp_msg(MSGT_CPLAYER, MSGL_INFO, "==========================================================================\n");
    init_best_video_codec(sh_video, video_codec_list, video_fm_list);
    mp_msg(MSGT_CPLAYER, MSGL_INFO, "==========================================================================\n");

    if (!sh_video->initialized) {
        if (!fixed_vo)
            uninit_player(INITIALIZED_VO);
        goto err_out;
    }

    initialized_flags |= INITIALIZED_VCODEC;

    if (sh_video->codec)
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_VIDEO_CODEC=%s\n", sh_video->codec->name);

    sh_video->last_pts = MP_NOPTS_VALUE;
    sh_video->num_buffered_pts = 0;
    sh_video->next_frame_time  = 0;

    if (auto_quality > 0) {
        // Auto quality option enabled
        output_quality = get_video_quality_max(sh_video);
        if (auto_quality > output_quality)
            auto_quality = output_quality;
        else
            output_quality = auto_quality;
        mp_msg(MSGT_CPLAYER, MSGL_V, "AutoQ: setting quality to %d.\n", output_quality);
        set_video_quality(sh_video, output_quality);
    }

    // ========== Init display (sh_video->disp_w*sh_video->disp_h/out_fmt) ============

    current_module = "init_vo";

    return 1;

err_out:
    mpctx->sh_video = mpctx->d_video->sh = NULL;
    return 0;
}

static double update_video(int *blit_frame)
{
    sh_video_t *const sh_video = mpctx->sh_video;
    //--------------------  Decode a frame: -----------------------
    double frame_time;
    *blit_frame = 0; // Don't blit if we hit EOF
    if (!correct_pts) {
        unsigned char *start = NULL;
        void *decoded_frame  = NULL;
        int drop_frame       = 0;
        int in_size;
        int full_frame;

        do {
            int flush;
            current_module = "video_read_frame";
            frame_time     = sh_video->next_frame_time;
            in_size = video_read_frame(sh_video, &sh_video->next_frame_time,
                                       &start, force_fps);
#ifdef CONFIG_DVDNAV
            // wait, still frame or EOF
            if (mpctx->stream->type == STREAMTYPE_DVDNAV && in_size < 0) {
                if (mp_dvdnav_is_eof(mpctx->stream))
                    return -1;
                if (mpctx->d_video)
                    mpctx->d_video->eof = 0;
                if (mpctx->d_audio)
                    mpctx->d_audio->eof = 0;
                mpctx->stream->eof = 0;
            }
#endif
            flush = in_size < 0 && mpctx->d_video->eof;
            if (flush) {
                start = NULL;
                in_size = 0;
            }
            if (mpctx->stream->type != STREAMTYPE_DVDNAV && in_size < 0)
                return -1;
            if (in_size > max_framesize)
                max_framesize = in_size;  // stats
            drop_frame     = check_framedrop(frame_time);
            current_module = "decode_video";
#ifdef CONFIG_DVDNAV
            full_frame    = 1;
            decoded_frame = mp_dvdnav_restore_smpi(&in_size, &start, decoded_frame);
#endif
            // still frame has been reached, no need to decode
            if ((in_size > 0 || flush) && !decoded_frame)
            decoded_frame = decode_video(sh_video, start, in_size, drop_frame,
                                         sh_video->pts, &full_frame);

            if (flush && !decoded_frame)
                return -1;

            if (full_frame) {
                sh_video->timer += frame_time;
                if (mpctx->sh_audio)
                    mpctx->delay -= frame_time;
                // video_read_frame can change fps (e.g. for ASF video)
                vo_fps = sh_video->fps;
                update_subtitles(sh_video, sh_video->pts, mpctx->d_sub, 0);
                update_teletext(sh_video, mpctx->demuxer, 0);
                update_osd_msg();
            }
#ifdef CONFIG_DVDNAV
            // save back last still frame for future display
            mp_dvdnav_save_smpi(in_size, start, decoded_frame);
#endif
        } while (!full_frame);

        current_module = "filter_video";
        *blit_frame    = (decoded_frame && filter_video(sh_video, decoded_frame,
                                                        sh_video->pts));
    } else {
        int res = generate_video_frame(sh_video, mpctx->d_video);
        if (!res)
            return -1;
        ((vf_instance_t *)sh_video->vfilter)->control(sh_video->vfilter,
                                                      VFCTRL_GET_PTS, &sh_video->pts);
        if (sh_video->pts == MP_NOPTS_VALUE) {
            mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_PtsAfterFiltersMissing);
            sh_video->pts = sh_video->last_pts;
        }
        if (sh_video->last_pts == MP_NOPTS_VALUE)
            sh_video->last_pts = sh_video->pts;
        else if (sh_video->last_pts > sh_video->pts) {
            // make a guess whether this is some kind of discontinuity
            // we should jump along with or some wrong timestamps we
            // should replace instead
            if (sh_video->pts < sh_video->last_pts - 20 * sh_video->frametime)
                sh_video->last_pts = sh_video->pts;
            else
                sh_video->pts = sh_video->last_pts + sh_video->frametime;
            mp_msg(MSGT_CPLAYER, MSGL_V, "pts value < previous\n");
        }
        frame_time = sh_video->pts - sh_video->last_pts;
        if (!frame_time)
            frame_time = sh_video->frametime;
        sh_video->last_pts = sh_video->pts;
        sh_video->timer   += frame_time;
        if (mpctx->sh_audio)
            mpctx->delay -= frame_time;
        *blit_frame = res > 0;
    }
    return frame_time;
}

static void pause_loop(void)
{
    mp_cmd_t *cmd;
#ifdef CONFIG_STREAM_CACHE
    int old_cache_fill = stream_cache_size > 0 ? cache_fill_status(mpctx->stream) : 0;
#endif
    if (!quiet) {
        if (term_osd && !mpctx->sh_video) {
            set_osd_msg(OSD_MSG_PAUSE, 1, 0, MSGTR_Paused);
            update_osd_msg();
        } else
            mp_msg(MSGT_CPLAYER, MSGL_STATUS, "\n"MSGTR_Paused "\r");
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_PAUSED\n");
    }
#ifdef CONFIG_GUI
    if (use_gui)
        gui(GUI_SET_STATE, (void *)GUI_PAUSE);
#endif
    if (mpctx->video_out && mpctx->sh_video && vo_config_count)
        mpctx->video_out->control(VOCTRL_PAUSE, NULL);

    if (mpctx->audio_out && mpctx->sh_audio)
        mpctx->audio_out->pause();  // pause audio, keep data if possible

    while ((cmd = mp_input_get_cmd(20, 1, 1)) == NULL || cmd->pausing == 4) {
        if (cmd) {
            cmd = mp_input_get_cmd(0, 1, 0);
            run_command(mpctx, cmd);
            mp_cmd_free(cmd);
            continue;
        }
        if (mpctx->sh_video && mpctx->video_out && vo_config_count)
            mpctx->video_out->check_events();
#ifdef CONFIG_GUI
        if (use_gui) {
            gui(GUI_REDRAW, 0);
            if (guiInfo.Playing != GUI_PAUSE || (rel_seek_secs || abs_seek_pos))
                break;
        }
#endif
#ifdef CONFIG_MENU
        if (vf_menu)
            vf_menu_pause_update(vf_menu);
#endif
#ifdef CONFIG_STREAM_CACHE
        if (!quiet && stream_cache_size > 0) {
            int new_cache_fill = cache_fill_status(mpctx->stream);
            if (new_cache_fill != old_cache_fill) {
                if (term_osd && !mpctx->sh_video) {
                    set_osd_msg(OSD_MSG_PAUSE, 1, 0, MSGTR_Paused " %d%%",
                                new_cache_fill);
                    update_osd_msg();
                } else
                    mp_msg(MSGT_CPLAYER, MSGL_STATUS, MSGTR_Paused " %d%%\r",
                           new_cache_fill);
                old_cache_fill = new_cache_fill;
            }
        }
#endif
        if (mpctx->sh_video)
            handle_udp_master(mpctx->sh_video->pts);
        usec_sleep(20000);
    }
    if (cmd && cmd->id == MP_CMD_PAUSE) {
        cmd = mp_input_get_cmd(0, 1, 0);
        mp_cmd_free(cmd);
    }
    mpctx->osd_function = OSD_PLAY;
    if (mpctx->audio_out && mpctx->sh_audio) {
        if (mpctx->eof) // do not play remaining audio if we e.g.  switch to the next file
            mpctx->audio_out->reset();
        else
            mpctx->audio_out->resume();  // resume audio
    }
    if (mpctx->video_out && mpctx->sh_video && vo_config_count)
        mpctx->video_out->control(VOCTRL_RESUME, NULL);  // resume video
    (void)GetRelativeTime(); // ignore time that passed during pause
#ifdef CONFIG_GUI
    if (use_gui) {
        if (guiInfo.Playing == GUI_STOP)
            mpctx->eof = 1;
        else
            gui(GUI_SET_STATE, (void *)GUI_PLAY);
    }
#endif
}

static void edl_loadfile(void)
{
    if (edl_filename) {
        if (edl_records) {
            free_edl(edl_records);
            edl_needs_reset = 1;
        }
        next_edl_record = edl_records = edl_parse_file();
    }
}

// Execute EDL command for the current position if one exists
static void edl_update(MPContext *mpctx)
{
    double pts;
    if (!edl_records)
        return;

    if (!mpctx->sh_video) {
        mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_EdlNOsh_video);
        free_edl(edl_records);
        next_edl_record = NULL;
        edl_records     = NULL;
        return;
    }

    pts = mpctx->sh_video->pts;
    // This indicates that we need to reset next EDL record according
    // to new PTS due to seek or other condition
    if (edl_needs_reset) {
        edl_needs_reset  = 0;
        mpctx->edl_muted = 0;
        next_edl_record  = edl_records;

        // Find next record, also skip immediately if we are already
        // inside any record
        while (next_edl_record) {
            if (next_edl_record->start_sec > pts)
                break;
            if (next_edl_record->stop_sec >= pts) {
                if (edl_backward) {
                    mpctx->osd_function = OSD_REW;
                    edl_decision  = 1;
                    abs_seek_pos  = 0;
                    rel_seek_secs = -(pts -
                                      next_edl_record->start_sec +
                                      edl_backward_delay);
                    mp_msg(MSGT_CPLAYER, MSGL_DBG4, "EDL_SKIP: pts [%f], "
                                                    "offset [%f], start [%f], stop [%f], length [%f]\n",
                           pts, rel_seek_secs,
                           next_edl_record->start_sec, next_edl_record->stop_sec,
                           next_edl_record->length_sec);
                    return;
                }
                break;
            }

            if (next_edl_record->action == EDL_MUTE)
                mpctx->edl_muted = !mpctx->edl_muted;

            next_edl_record = next_edl_record->next;
        }
        if ((mpctx->user_muted | mpctx->edl_muted) != mpctx->mixer.muted)
            mixer_mute(&mpctx->mixer);
    }

    if (next_edl_record &&
        pts >= next_edl_record->start_sec) {
        if (next_edl_record->action == EDL_SKIP) {
            mpctx->osd_function = OSD_FFW;
            edl_decision  = 1;
            abs_seek_pos  = 0;
            rel_seek_secs = next_edl_record->stop_sec - pts;
            mp_msg(MSGT_CPLAYER, MSGL_DBG4, "EDL_SKIP: pts [%f], offset [%f], "
                                            "start [%f], stop [%f], length [%f]\n",
                   pts, rel_seek_secs,
                   next_edl_record->start_sec, next_edl_record->stop_sec,
                   next_edl_record->length_sec);
        } else if (next_edl_record->action == EDL_MUTE) {
            mpctx->edl_muted = !mpctx->edl_muted;
            if ((mpctx->user_muted | mpctx->edl_muted) != mpctx->mixer.muted)
                mixer_mute(&mpctx->mixer);
            mp_msg(MSGT_CPLAYER, MSGL_DBG4, "EDL_MUTE: [%f]\n",
                   next_edl_record->start_sec);
        }
        next_edl_record = next_edl_record->next;
    }
}

// style & SEEK_ABSOLUTE == 0 means seek relative to current position, == 1 means absolute
// style & SEEK_FACTOR   == 0 means amount in seconds, == 2 means fraction of file length
// return -1 if seek failed (non-seekable stream?), 0 otherwise
static int seek(MPContext *mpctx, double amount, int style)
{
    current_module = "seek";
    if (demux_seek(mpctx->demuxer, amount, audio_delay, style) == 0)
        return -1;

    mpctx->startup_decode_retry = DEFAULT_STARTUP_DECODE_RETRY;
    if (mpctx->sh_video) {
        current_module = "seek_video_reset";
        if (vo_config_count)
            mpctx->video_out->control(VOCTRL_RESET, NULL);
        mpctx->num_buffered_frames = 0;
        mpctx->delay           = 0;
        mpctx->time_frame      = 0;
        // Not all demuxers set d_video->pts during seek, so this value
        // (which is used by at least vobsub and edl code below) may
        // be completely wrong (probably 0).
        mpctx->sh_video->pts = mpctx->d_video->pts;
        update_subtitles(mpctx->sh_video, mpctx->sh_video->pts, mpctx->d_sub, 1);
        update_teletext(mpctx->sh_video, mpctx->demuxer, 1);
    }

    if (mpctx->sh_audio) {
        current_module = "seek_audio_reset";
        mpctx->audio_out->reset(); // stop audio, throwing away buffered data
        if (!mpctx->sh_video)
            update_subtitles(NULL, mpctx->sh_audio->pts, mpctx->d_sub, 1);
    }

    if (vo_vobsub && mpctx->sh_video) {
        current_module = "seek_vobsub_reset";
        vobsub_seek(vo_vobsub, mpctx->sh_video->pts);
    }

#ifdef CONFIG_ASS
    if (ass_enabled && mpctx->d_sub->sh && ((sh_sub_t *)mpctx->d_sub->sh)->ass_track)
        ass_flush_events(((sh_sub_t *)mpctx->d_sub->sh)->ass_track);
#endif

    if (edl_records) {
        edl_needs_reset = 1;
        edl_backward    = amount < 0;
    }

    c_total = 0;
    max_pts_correction = 0.1;
    audio_time_usage   = 0;
    video_time_usage   = 0;
    vout_time_usage    = 0;
    drop_frame_cnt     = 0;

    current_module = NULL;
    return 0;
}

/* This preprocessor directive is a hack to generate a mplayer-nomain.o object
 * file for some tools to link against. */
#ifndef DISABLE_MAIN
int main(int argc, char *argv[])
{
    int opt_exit = 0; // Flag indicating whether MPlayer should exit without playing anything.
    int profile_config_loaded;
    int i;

    common_preinit(&argc, &argv);

    // Create the config context and register the options
    mconfig = m_config_new();
    m_config_register_options(mconfig, mplayer_opts);
    m_config_register_options(mconfig, common_opts);
    mp_input_register_options(mconfig);

    // Preparse the command line
    m_config_preparse_command_line(mconfig, argc, argv);

#ifdef CONFIG_TV
    stream_tv_defaults.immediate = 1;
#endif

    if (argc > 1 && argv[1] &&
        (!strcmp(argv[1], "-gui") || !strcmp(argv[1], "-nogui"))) {
        use_gui = !strcmp(argv[1], "-gui");
    } else if (argv[0] && strstr(mp_basename(argv[0]), gmplayer)) {
        use_gui = 1;
    }

    parse_cfgfiles(mconfig);

#ifdef CONFIG_GUI
    if (use_gui) {
        initialized_flags |= INITIALIZED_GUI;
        cfg_read();
    }
#endif

    mpctx->playtree = m_config_parse_mp_command_line(mconfig, argc, argv);
    if (mpctx->playtree == NULL) {
        opt_exit = 1;
    } else {
        mpctx->playtree = play_tree_cleanup(mpctx->playtree);
        if (mpctx->playtree) {
            mpctx->playtree_iter = play_tree_iter_new(mpctx->playtree, mconfig);
            if (mpctx->playtree_iter) {
                if (play_tree_iter_step(mpctx->playtree_iter, 0, 0) != PLAY_TREE_ITER_ENTRY) {
                    play_tree_iter_free(mpctx->playtree_iter);
                    mpctx->playtree_iter = NULL;
                }
                filename = play_tree_iter_get_file(mpctx->playtree_iter, 1);
            }
        }
    }

    print_version("MPlayer");
#if (defined(__MINGW32__) || defined(__CYGWIN__)) && defined(CONFIG_GUI)
    void *runningmplayer = FindWindow("MPlayer GUI for Windows", "MPlayer for Windows");
    if (runningmplayer && filename && use_gui) {
        COPYDATASTRUCT csData;
        char file[MAX_PATH];
        char *filepart = filename;
        if (GetFullPathName(filename, MAX_PATH, file, &filepart)) {
            csData.dwData = 0;
            csData.cbData = strlen(file) * 2;
            csData.lpData = file;
            SendMessage(runningmplayer, WM_COPYDATA, (WPARAM)runningmplayer, (LPARAM)&csData);
        }
    }
#endif
    if (!common_init())
        exit_player_with_rc(EXIT_NONE, 0);

#ifndef CONFIG_GUI
    if (use_gui) {
        mp_msg(MSGT_CPLAYER, MSGL_WARN, MSGTR_NoGui);
        use_gui = 0;
    }
#else
#if !defined(__MINGW32__) && !defined(__CYGWIN__)
    if (use_gui && !vo_init()) {
        mp_msg(MSGT_CPLAYER, MSGL_WARN, MSGTR_GuiNeedsX);
        use_gui = 0;
    }
#endif
    if (use_gui && mpctx->playtree_iter) {
        char cwd[PATH_MAX + 2];
        // Free playtree_iter as it's not used in connection with the GUI.
        play_tree_iter_free(mpctx->playtree_iter);
        mpctx->playtree_iter = NULL;

        if (getcwd(cwd, PATH_MAX) != (char *)NULL) {
            strcat(cwd, "/");
            // Prefix relative paths with current working directory
            play_tree_add_bpf(mpctx->playtree, cwd);
        }
        // Import initital playtree into GUI.
        guiPlaylist(GUI_PLAYLIST_INIT, mpctx->playtree, mconfig, enqueue);
    }
#endif /* CONFIG_GUI */

    if (video_driver_list && strcmp(video_driver_list[0], "help") == 0) {
        list_video_out();
        opt_exit = 1;
    }

    if (audio_driver_list && strcmp(audio_driver_list[0], "help") == 0) {
        list_audio_out();
        opt_exit = 1;
    }

    if (audio_codec_list && strcmp(audio_codec_list[0], "help") == 0) {
        mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_AvailableAudioCodecs);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_AUDIO_CODECS\n");
        list_codecs(1);
        mp_msg(MSGT_FIXME, MSGL_FIXME, "\n");
        opt_exit = 1;
    }
    if (video_codec_list && strcmp(video_codec_list[0], "help") == 0) {
        mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_AvailableVideoCodecs);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_VIDEO_CODECS\n");
        list_codecs(0);
        mp_msg(MSGT_FIXME, MSGL_FIXME, "\n");
        opt_exit = 1;
    }
    if (video_fm_list && strcmp(video_fm_list[0], "help") == 0) {
        vfm_help();
        mp_msg(MSGT_FIXME, MSGL_FIXME, "\n");
        opt_exit = 1;
    }
    if (audio_fm_list && strcmp(audio_fm_list[0], "help") == 0) {
        afm_help();
        mp_msg(MSGT_FIXME, MSGL_FIXME, "\n");
        opt_exit = 1;
    }
    if (af_cfg.list && strcmp(af_cfg.list[0], "help") == 0) {
        af_help();
        printf("\n");
        opt_exit = 1;
    }
#ifdef CONFIG_X11
    if (vo_fstype_list && strcmp(vo_fstype_list[0], "help") == 0) {
        fstype_help();
        mp_msg(MSGT_FIXME, MSGL_FIXME, "\n");
        opt_exit = 1;
    }
#endif
    if ((demuxer_name && strcmp(demuxer_name, "help") == 0) ||
        (audio_demuxer_name && strcmp(audio_demuxer_name, "help") == 0) ||
        (sub_demuxer_name && strcmp(sub_demuxer_name, "help") == 0)) {
        demuxer_help();
        mp_msg(MSGT_CPLAYER, MSGL_INFO, "\n");
        opt_exit = 1;
    }
    if (list_properties) {
        property_print_help();
        opt_exit = 1;
    }

    if (opt_exit)
        exit_player(EXIT_NONE);

    if (!filename && !player_idle_mode && !use_gui) {
        // no file/vcd/dvd -> show HELP:
        mp_msg(MSGT_CPLAYER, MSGL_INFO, help_text);
        exit_player_with_rc(EXIT_NONE, 0);
    }

    /* Display what configure line was used */
    mp_msg(MSGT_CPLAYER, MSGL_V, "Configuration: " CONFIGURATION "\n");

    // Many users forget to include command line in bugreports...
    if (mp_msg_test(MSGT_CPLAYER, MSGL_V)) {
        mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_CommandLine);
        for (i = 1; i < argc; i++)
            mp_msg(MSGT_CPLAYER, MSGL_INFO, " '%s'", argv[i]);
        mp_msg(MSGT_CPLAYER, MSGL_INFO, "\n");
    }

//------ load global data first ------

#ifdef HAVE_RTC
    if (!nortc) {
        // seteuid(0); /* Can't hurt to try to get root here */
        if ((rtc_fd = open(rtc_device ? rtc_device : "/dev/rtc", O_RDONLY)) < 0)
            mp_msg(MSGT_CPLAYER, MSGL_WARN, MSGTR_RTCDeviceNotOpenable,
                   rtc_device ? rtc_device : "/dev/rtc", strerror(errno));
        else {
            unsigned long irqp = 1024; /* 512 seemed OK. 128 is jerky. */

            if (ioctl(rtc_fd, RTC_IRQP_SET, irqp) < 0) {
                mp_msg(MSGT_CPLAYER, MSGL_WARN, MSGTR_LinuxRTCInitErrorIrqpSet, irqp, strerror(errno));
                mp_msg(MSGT_CPLAYER, MSGL_HINT, MSGTR_IncreaseRTCMaxUserFreq, irqp);
                close(rtc_fd);
                rtc_fd = -1;
            } else if (ioctl(rtc_fd, RTC_PIE_ON, 0) < 0) {
                /* variable only by the root */
                mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_LinuxRTCInitErrorPieOn, strerror(errno));
                close(rtc_fd);
                rtc_fd = -1;
            } else
                mp_msg(MSGT_CPLAYER, MSGL_V, "Using Linux hardware RTC timing (%ldHz).\n", irqp);
        }
    }
#ifdef CONFIG_GUI
    // breaks DGA and SVGAlib and VESA drivers:  --A'rpi
    // and now ? -- Pontscho
    if (use_gui)
        setuid(getuid());           // strongly test, please check this.
#endif
    if (rtc_fd < 0)
#endif /* HAVE_RTC */
    mp_msg(MSGT_CPLAYER, MSGL_V, "Using %s timing\n",
           softsleep ? "software" : timer_name);

#ifdef HAVE_TERMCAP
    if (!use_gui)
        load_termcap(NULL);           // load key-codes
#endif

// ========== Init keyboard FIFO (connection to libvo) ============

    // Init input system
    current_module = "init_input";
    mp_input_init();
    mp_input_add_key_fd(-1, 0, mplayer_get_key, NULL);
    if (slave_mode)
        mp_input_add_cmd_fd(0, USE_SELECT, MP_INPUT_SLAVE_CMD_FUNC, NULL);
    else if (!noconsolecontrols)
        mp_input_add_event_fd(0, getch2);
    // Set the libstream interrupt callback
    stream_set_interrupt_callback(mp_input_check_interrupt);

#ifdef CONFIG_MENU
    if (use_menu) {
        if (menu_cfg && menu_init(mpctx, menu_cfg))
            mp_msg(MSGT_CPLAYER, MSGL_V, "Menu initialized: %s\n", menu_cfg);
        else {
            menu_cfg = get_path("menu.conf");
            if (menu_init(mpctx, menu_cfg))
                mp_msg(MSGT_CPLAYER, MSGL_V, "Menu initialized: %s\n", menu_cfg);
            else {
                if (menu_init(mpctx, MPLAYER_CONFDIR "/menu.conf"))
                    mp_msg(MSGT_CPLAYER, MSGL_V,  "Menu initialized: %s\n", MPLAYER_CONFDIR "/menu.conf");
                else {
                    mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_MenuInitFailed);
                    use_menu = 0;
                }
            }
        }
    }
#endif

    initialized_flags |= INITIALIZED_INPUT;
    current_module     = NULL;

    // Catch signals
#ifndef __MINGW32__
    signal(SIGCHLD, child_sighandler);
#endif

#ifdef CONFIG_CRASH_DEBUG
    prog_path = argv[0];
#endif
    //========= Catch terminate signals: ================
    // terminate requests:
    signal(SIGTERM, exit_sighandler); // kill
    signal(SIGHUP, exit_sighandler); // kill -HUP  /  xterm closed

    signal(SIGINT, exit_sighandler); // Interrupt from keyboard

    signal(SIGQUIT, exit_sighandler); // Quit from keyboard
    signal(SIGPIPE, exit_sighandler); // Some window managers cause this
#ifdef CONFIG_SIGHANDLER
    // fatal errors:
    signal(SIGBUS, exit_sighandler); // bus error
#ifndef __WINE__                      // hack: the Wine executable will crash else
    signal(SIGSEGV, exit_sighandler); // segfault
#endif
    signal(SIGILL, exit_sighandler); // illegal instruction
    signal(SIGFPE, exit_sighandler); // floating point exc.
    signal(SIGABRT, exit_sighandler); // abort()
#ifdef CONFIG_CRASH_DEBUG
    if (crash_debug)
        signal(SIGTRAP, exit_sighandler);
#endif
#endif

#ifdef CONFIG_GUI
    if (use_gui) {
        guiInit();
        gui(GUI_SET_CONTEXT, mpctx);
        gui(GUI_SET_STATE, (void *)(filename ? GUI_PLAY : GUI_STOP));
    }
#endif

// ******************* Now, let's see the per-file stuff ********************

play_next_file:

    // init global sub numbers
    mpctx->global_sub_size = 0;
    memset(mpctx->sub_counts, 0, sizeof(mpctx->sub_counts));

    profile_config_loaded = load_profile_config(mconfig, filename);

    if (video_driver_list)
        load_per_output_config(mconfig, PROFILE_CFG_VO, video_driver_list[0]);
    if (audio_driver_list)
        load_per_output_config(mconfig, PROFILE_CFG_AO, audio_driver_list[0]);

    // We must enable getch2 here to be able to interrupt network connection
    // or cache filling
    if (!noconsolecontrols && !slave_mode) {
        if (initialized_flags & INITIALIZED_GETCH2)
            mp_msg(MSGT_CPLAYER, MSGL_WARN, MSGTR_Getch2InitializedTwice);
        else
            getch2_enable();  // prepare stdin for hotkeys...
        initialized_flags |= INITIALIZED_GETCH2;
        mp_msg(MSGT_CPLAYER, MSGL_DBG2, "\n[[[init getch2]]]\n");
    }

// =================== GUI idle loop (STOP state) ===========================
#ifdef CONFIG_GUI
    if (use_gui) {
        mpctx->file_format = DEMUXER_TYPE_UNKNOWN;
        while (guiInfo.Playing != GUI_PLAY) {
            mp_cmd_t *cmd;
            usec_sleep(20000);
            gui(GUI_REDRAW, 0);
            if ((cmd = mp_input_get_cmd(0, 0, 0)) != NULL) {
                if (cmd->id == MP_CMD_GUI)
                    gui(GUI_RUN_MESSAGE, cmd->args[0].v.s);
                else
                    gui(GUI_RUN_COMMAND, (void *)cmd->id);
                mp_cmd_free(cmd);
            }
        }
        gui(GUI_PREPARE, 0);
    }
#endif /* CONFIG_GUI */

    while (player_idle_mode && !filename) {
        play_tree_t *entry = NULL;
        mp_cmd_t *cmd;
        if (mpctx->video_out && vo_config_count)
            mpctx->video_out->control(VOCTRL_PAUSE, NULL);
        while (!(cmd = mp_input_get_cmd(0, 1, 0))) { // wait for command
            if (mpctx->video_out && vo_config_count)
                mpctx->video_out->check_events();
            usec_sleep(20000);
        }
        switch (cmd->id) {
        case MP_CMD_LOADFILE:
            // prepare a tree entry with the new filename
            entry = play_tree_new();
            play_tree_add_file(entry, cmd->args[0].v.s);
            // The entry is added to the main playtree after the switch().
            break;
        case MP_CMD_LOADLIST:
            entry = parse_playlist_file(cmd->args[0].v.s);
            break;
        case MP_CMD_QUIT:
            exit_player_with_rc(EXIT_QUIT, (cmd->nargs > 0) ? cmd->args[0].v.i : 0);
            break;
        case MP_CMD_VO_FULLSCREEN:
        case MP_CMD_GET_PROPERTY:
        case MP_CMD_SET_PROPERTY:
        case MP_CMD_STEP_PROPERTY:
            run_command(mpctx, cmd);
            break;
        }
        mpctx->osd_function = cmd->pausing ? OSD_PAUSE : OSD_PLAY;

        mp_cmd_free(cmd);

        if (entry) { // user entered a command that gave a valid entry
            if (mpctx->playtree) // the playtree is always a node with one child. let's clear it
                play_tree_free_list(mpctx->playtree->child, 1);
            else
                mpctx->playtree = play_tree_new();  // .. or make a brand new playtree

            if (!mpctx->playtree)
                continue;               // couldn't make playtree! wait for next command

            play_tree_set_child(mpctx->playtree, entry);

            /* Make iterator start at the top the of tree. */
            mpctx->playtree_iter = play_tree_iter_new(mpctx->playtree, mconfig);
            if (!mpctx->playtree_iter)
                continue;

            // find the first real item in the tree
            if (play_tree_iter_step(mpctx->playtree_iter, 0, 0) != PLAY_TREE_ITER_ENTRY) {
                // no items!
                play_tree_iter_free(mpctx->playtree_iter);
                mpctx->playtree_iter = NULL;
                continue; // wait for next command
            }
            filename = play_tree_iter_get_file(mpctx->playtree_iter, 1);
        }
    }

    if (!profile_config_loaded) load_profile_config(mconfig, filename);
//---------------------------------------------------------------------------

    if (mpctx->video_out && vo_config_count)
        mpctx->video_out->control(VOCTRL_RESUME, NULL);

    if (filename) {
        mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_Playing,
               filename_recode(filename));
        if (use_filename_title && vo_wintitle == NULL)
            vo_wintitle = strdup(mp_basename(filename));
    }

    edl_loadfile();

    if (edl_output_filename) {
        if (edl_fd)
            fclose(edl_fd);
        if ((edl_fd = fopen(edl_output_filename, "w")) == NULL) {
            mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_EdlCantOpenForWrite,
                   filename_recode(edl_output_filename));
        }
    }

//==================== Open VOB-Sub ============================

    current_module = "vobsub";
    load_vob_subtitle(filename, spudec_ifo, &vo_spudec, add_vob_subtitle);
    if (vo_vobsub) {
        initialized_flags |= INITIALIZED_VOBSUB;
        vobsub_set_from_lang(vo_vobsub, dvdsub_lang);
        mp_property_do("sub_forced_only", M_PROPERTY_SET, &forced_subs_only, mpctx);

        // setup global sub numbering
        mpctx->sub_counts[SUB_SOURCE_VOBSUB] = vobsub_get_indexes_count(vo_vobsub);
    }
#ifdef CONFIG_ASS
    // must be before demuxer open, since the settings are
    // used in generating the ASSTrack
    if (ass_enabled && ass_library)
        ass_mp_reset_config(ass_library);
#endif

//============ Open & Sync STREAM --- fork cache2 ====================

    mpctx->stream  = NULL;
    mpctx->demuxer = NULL;
    if (mpctx->d_audio) {
        //free_demuxer_stream(mpctx->d_audio);
        mpctx->d_audio = NULL;
    }
    if (mpctx->d_video) {
        //free_demuxer_stream(d_video);
        mpctx->d_video = NULL;
    }
    mpctx->sh_audio = NULL;
    mpctx->sh_video = NULL;

    current_module = "open_stream";
    mpctx->stream  = open_stream(filename, 0, &mpctx->file_format);
    if (!mpctx->stream) { // error...
        mpctx->eof = libmpdemux_was_interrupted(PT_NEXT_ENTRY);
        goto goto_next_file;
    }
    initialized_flags |= INITIALIZED_STREAM;

#ifdef CONFIG_GUI
    if (use_gui)
        gui(GUI_SET_STREAM, mpctx->stream);
#endif

    if (mpctx->file_format == DEMUXER_TYPE_PLAYLIST) {
        play_tree_t *entry;
        // Handle playlist
        current_module = "handle_playlist";
        mp_msg(MSGT_CPLAYER, MSGL_V, "Parsing playlist %s...\n",
               filename_recode(filename));
        if (allow_playlist_parsing) {
            entry      = parse_playtree(mpctx->stream, use_gui);
            mpctx->eof = playtree_add_playlist(entry);
        } else {
            mp_msg(MSGT_CPLAYER, MSGL_ERR, "Playlist parsing disabled for security reasons. Ignoring file.\n");
        }
        goto goto_next_file;
    }
    mpctx->stream->start_pos += seek_to_byte;

    if (stream_dump_type == 5) {
        unsigned char buf[4096];
        int len;
        FILE *f;
        current_module = "dumpstream";
        stream_reset(mpctx->stream);
        stream_seek(mpctx->stream, mpctx->stream->start_pos);
        f = fopen(stream_dump_name, "wb");
        if (!f) {
            mp_msg(MSGT_CPLAYER, MSGL_FATAL, MSGTR_CantOpenDumpfile);
            exit_player(EXIT_ERROR);
        }
        if (dvd_chapter > 1) {
            int chapter = dvd_chapter - 1;
            stream_control(mpctx->stream, STREAM_CTRL_SEEK_TO_CHAPTER, &chapter);
        }
        stream_dump_progress_start();
        while (!mpctx->stream->eof && !async_quit_request) {
            double pts;
            if (stream_control(mpctx->stream, STREAM_CTRL_GET_CURRENT_TIME, &pts) != STREAM_OK)
                pts = MP_NOPTS_VALUE;
            if (is_at_end(mpctx, &end_at, pts))
                break;
            len = stream_read(mpctx->stream, buf, 4096);
            if (len > 0) {
                if (fwrite(buf, len, 1, f) != 1) {
                    mp_msg(MSGT_MENCODER, MSGL_FATAL, MSGTR_ErrorWritingFile, stream_dump_name);
                    exit_player(EXIT_ERROR);
                }
            }
            stream_dump_progress(len, mpctx->stream);
            if (dvd_last_chapter > 0) {
                int chapter = -1;
                if (stream_control(mpctx->stream, STREAM_CTRL_GET_CURRENT_CHAPTER,
                                   &chapter) == STREAM_OK && chapter + 1 > dvd_last_chapter)
                    break;
            }
        }
        if (fclose(f)) {
            mp_msg(MSGT_MENCODER, MSGL_FATAL, MSGTR_ErrorWritingFile, stream_dump_name);
            exit_player(EXIT_ERROR);
        }
        stream_dump_progress_end();
        mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_CoreDumped);
        exit_player_with_rc(EXIT_EOF, 0);
    }

    if (mpctx->stream->type == STREAMTYPE_BD) {
        if (audio_lang && audio_id == -1)
            audio_id = bd_aid_from_lang(mpctx->stream, audio_lang);
        if (dvdsub_lang && dvdsub_id == -1)
            dvdsub_id = bd_sid_from_lang(mpctx->stream, dvdsub_lang);
    }

#ifdef CONFIG_DVDREAD
    if (mpctx->stream->type == STREAMTYPE_DVD) {
        current_module = "dvd lang->id";
        if (audio_lang && audio_id == -1)
            audio_id = dvd_aid_from_lang(mpctx->stream, audio_lang);
        if (dvdsub_lang && dvdsub_id == -1)
            dvdsub_id = dvd_sid_from_lang(mpctx->stream, dvdsub_lang);
        // setup global sub numbering
        mpctx->sub_counts[SUB_SOURCE_DEMUX] = dvd_number_of_subs(mpctx->stream);
        current_module = NULL;
    }
#endif

#ifdef CONFIG_DVDNAV
    if (mpctx->stream->type == STREAMTYPE_DVDNAV) {
        current_module = "dvdnav lang->id";
        if (audio_lang && audio_id == -1)
            audio_id = mp_dvdnav_aid_from_lang(mpctx->stream, audio_lang);
        dvdsub_lang_id = -3;
        if (dvdsub_lang && dvdsub_id == -1)
            dvdsub_lang_id = dvdsub_id = mp_dvdnav_sid_from_lang(mpctx->stream, dvdsub_lang);
        // setup global sub numbering
        mpctx->sub_counts[SUB_SOURCE_DEMUX] = mp_dvdnav_number_of_subs(mpctx->stream);
        current_module = NULL;
    }
#endif

// CACHE2: initial prefill: 20%  later: 5%  (should be set by -cacheopts)
goto_enable_cache:
    if (stream_cache_size > 0) {
        int res;
        current_module = "enable_cache";
        res = stream_enable_cache(mpctx->stream, stream_cache_size * 1024ull,
                                  stream_cache_size * 1024ull * (stream_cache_min_percent / 100.0),
                                  stream_cache_size * 1024ull * (stream_cache_seek_min_percent / 100.0));
        if (res == 0)
            if ((mpctx->eof = libmpdemux_was_interrupted(PT_NEXT_ENTRY)))
                goto goto_next_file;
    }

//============ Open DEMUXERS --- DETECT file type =======================
    current_module = "demux_open";

    mpctx->demuxer = demux_open(mpctx->stream, mpctx->file_format, audio_id, video_id, dvdsub_id, filename);

    // HACK to get MOV Reference Files working
    if (mpctx->demuxer && mpctx->demuxer->type == DEMUXER_TYPE_PLAYLIST) {
        unsigned char *playlist_entry;
        play_tree_t *list = NULL, *entry = NULL;

        current_module = "handle_demux_playlist";
        while (ds_get_packet(mpctx->demuxer->video, &playlist_entry) > 0) {
            char *temp;
            const char *bname;

            mp_msg(MSGT_CPLAYER, MSGL_V, "Adding file %s to element entry.\n",
                   filename_recode(playlist_entry));

            bname = mp_basename(playlist_entry);
            if ((strlen(bname) > 10) && !strncmp(bname, "qt", 2) && !strncmp(bname + 3, "gateQT", 6))
                continue;

            if (!strcmp(playlist_entry, filename)) // ignoring self-reference
                continue;

            entry = play_tree_new();

            if (filename && !strcmp(mp_basename(playlist_entry), playlist_entry)) { // add reference path of current file
                temp = malloc((strlen(filename) - strlen(mp_basename(filename)) + strlen(playlist_entry) + 1));
                if (temp) {
                    strncpy(temp, filename, strlen(filename) - strlen(mp_basename(filename)));
                    temp[strlen(filename) - strlen(mp_basename(filename))] = '\0';
                    strcat(temp, playlist_entry);
                    if (!strcmp(temp, filename)) {
                        free(temp);
                        continue;
                    }
                    play_tree_add_file(entry, temp);
                    mp_msg(MSGT_CPLAYER, MSGL_V, "Resolving reference to %s.\n", temp);
                    free(temp);
                }
            } else
                play_tree_add_file(entry, playlist_entry);

            if (!list)
                list = entry;
            else
                play_tree_append_entry(list, entry);
        }
        free_demuxer(mpctx->demuxer);
        mpctx->demuxer = NULL;

        if (list) {
            entry = play_tree_new();
            play_tree_set_child(entry, list);
            mpctx->eof = playtree_add_playlist(entry);
            goto goto_next_file;
        }
    }

    if (!mpctx->demuxer)
        goto goto_next_file;
    if (dvd_chapter > 1) {
        float pts;
        if (demuxer_seek_chapter(mpctx->demuxer, dvd_chapter - 1, 1, &pts, NULL, NULL) >= 0 && pts > -1.0)
            seek(mpctx, pts, SEEK_ABSOLUTE);
    }

    initialized_flags |= INITIALIZED_DEMUXER;

#ifdef CONFIG_ASS
    if (ass_enabled && ass_library) {
        for (i = 0; i < mpctx->demuxer->num_attachments; ++i) {
            demux_attachment_t *att = mpctx->demuxer->attachments + i;
            if (extract_embedded_fonts &&
                att->name && att->type && att->data && att->data_size &&
                (strcmp(att->type, "application/x-truetype-font") == 0 ||
                 strcmp(att->type, "application/x-font") == 0))
                ass_add_font(ass_library, att->name, att->data, att->data_size);
        }
    }
#endif

    current_module = "demux_open2";

    mpctx->d_audio = mpctx->demuxer->audio;
    mpctx->d_video = mpctx->demuxer->video;
    mpctx->d_sub   = mpctx->demuxer->sub;

    if (ts_prog) {
        int tmp = ts_prog;
        mp_property_do("switch_program", M_PROPERTY_SET, &tmp, mpctx);
    }
    // select audio stream
    select_audio(mpctx->demuxer, audio_id, audio_lang);

    // DUMP STREAMS:
    if ((stream_dump_type) && (stream_dump_type < 4)) {
        FILE *f;
        demux_stream_t *ds = NULL;
        current_module = "dump";
        // select stream to dump
        switch (stream_dump_type) {
        case 1: ds = mpctx->d_audio; break;
        case 2: ds = mpctx->d_video; break;
        case 3: ds = mpctx->d_sub;   break;
        }
        if (!ds) {
            mp_msg(MSGT_CPLAYER, MSGL_FATAL, MSGTR_DumpSelectedStreamMissing);
            exit_player(EXIT_ERROR);
        }
        // disable other streams:
        if (mpctx->d_audio && mpctx->d_audio != ds) {
            ds_free_packs(mpctx->d_audio);
            mpctx->d_audio->id = -2;
        }
        if (mpctx->d_video && mpctx->d_video != ds) {
            ds_free_packs(mpctx->d_video);
            mpctx->d_video->id = -2;
        }
        if (mpctx->d_sub && mpctx->d_sub != ds) {
            ds_free_packs(mpctx->d_sub);
            mpctx->d_sub->id = -2;
        }
        // let's dump it!
        f = fopen(stream_dump_name, "wb");
        if (!f) {
            mp_msg(MSGT_CPLAYER, MSGL_FATAL, MSGTR_CantOpenDumpfile);
            exit_player(EXIT_ERROR);
        }
        stream_dump_progress_start();
        while (!ds->eof) {
            unsigned char *start;
            double pts;
            int in_size = ds_get_packet_pts(ds, &start, &pts);
            if (is_at_end(mpctx, &end_at, pts))
                break;
            if ((mpctx->demuxer->file_format == DEMUXER_TYPE_AVI || mpctx->demuxer->file_format == DEMUXER_TYPE_ASF || mpctx->demuxer->file_format == DEMUXER_TYPE_MOV)
                && stream_dump_type == 2)
                fwrite(&in_size, 1, 4, f);
            if (in_size > 0) {
                fwrite(start, in_size, 1, f);
                stream_dump_progress(in_size, mpctx->stream);
            }
            if (dvd_last_chapter > 0) {
                int cur_chapter = demuxer_get_current_chapter(mpctx->demuxer);
                if (cur_chapter != -1 && cur_chapter + 1 > dvd_last_chapter)
                    break;
            }
        }
        fclose(f);
        stream_dump_progress_end();
        mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_CoreDumped);
        exit_player_with_rc(EXIT_EOF, 0);
    }

    mpctx->sh_audio = mpctx->d_audio->sh;
    mpctx->sh_video = mpctx->d_video->sh;

    if (mpctx->sh_video) {
        current_module = "video_read_properties";
        if (!video_read_properties(mpctx->sh_video)) {
            mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_CannotReadVideoProperties);
            mpctx->sh_video = mpctx->d_video->sh = NULL;
        } else {
            mp_msg(MSGT_CPLAYER, MSGL_V, MSGTR_FilefmtFourccSizeFpsFtime,
                   mpctx->demuxer->file_format, mpctx->sh_video->format, mpctx->sh_video->disp_w, mpctx->sh_video->disp_h,
                   mpctx->sh_video->fps, mpctx->sh_video->frametime
                   );

            /* need to set fps here for output encoders to pick it up in their init */
            if (force_fps) {
                mpctx->sh_video->fps       = force_fps;
                mpctx->sh_video->frametime = 1.0f / mpctx->sh_video->fps;
            }
            vo_fps = mpctx->sh_video->fps;

            if (!mpctx->sh_video->fps && !force_fps && !correct_pts) {
                mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_FPSnotspecified);
                correct_pts = 1;
            }
        }
    }

    if (!mpctx->sh_video && !mpctx->sh_audio) {
        mp_msg(MSGT_CPLAYER, MSGL_FATAL, MSGTR_NoStreamFound);
#ifdef CONFIG_DVBIN
        if (mpctx->stream->type == STREAMTYPE_DVB) {
            int dir;
            int v = mpctx->last_dvb_step;
            if (v > 0)
                dir = DVB_CHANNEL_HIGHER;
            else
                dir = DVB_CHANNEL_LOWER;

            if (dvb_step_channel(mpctx->stream, dir))
                mpctx->eof = mpctx->dvbin_reopen = 1;
        }
#endif
        goto goto_next_file; // exit_player(MSGTR_Exit_error);
    }

    /* display clip info */
    demux_info_print(mpctx->demuxer);

//================== Read SUBTITLES (DVD & TEXT) ==========================
    if (vo_spudec == NULL &&
        (mpctx->stream->type == STREAMTYPE_DVD || mpctx->stream->type == STREAMTYPE_DVDNAV)) {
        init_vo_spudec(mpctx->stream, mpctx->sh_video, mpctx->d_sub ? mpctx->d_sub->sh : NULL);
    }

    if (1 || mpctx->sh_video) {
        // after reading video params we should load subtitles because
        // we know fps so now we can adjust subtitle time to ~6 seconds AST
        // check .sub
        double fps = mpctx->sh_video ? mpctx->sh_video->fps : 25;
        current_module = "read_subtitles_file";
        load_subtitles(filename, fps, add_subtitles);
        if (mpctx->set_of_sub_size > 0)
            mpctx->sub_counts[SUB_SOURCE_SUBS] = mpctx->set_of_sub_size;
        // set even if we have no subs yet, they may be added later
        initialized_flags |= INITIALIZED_SUBS;
    }

    if (select_subtitle(mpctx) && subdata) {
        switch (stream_dump_type) {
        case 3: list_sub_file(subdata);                       break;
        case 4: dump_mpsub(subdata,    mpctx->sh_video->fps); break;
        case 6: dump_srt(subdata,      mpctx->sh_video->fps); break;
        case 7: dump_microdvd(subdata, mpctx->sh_video->fps); break;
        case 8: dump_jacosub(subdata,  mpctx->sh_video->fps); break;
        case 9: dump_sami(subdata,     mpctx->sh_video->fps); break;
        }
    }

    print_file_properties(mpctx, filename);

    // Adjust EDL positions with start_pts
    if (edl_start_pts && start_pts) {
        edl_record_ptr edl = edl_records;
        while (edl) {
            edl->start_sec += start_pts;
            edl->stop_sec  += start_pts;
            edl = edl->next;
        }
    }

    if (mpctx->sh_video)
        reinit_video_chain();

    if (mpctx->sh_video) {
        if (vo_flags & 0x08 && vo_spudec)
            spudec_set_hw_spu(vo_spudec, mpctx->video_out);

#ifdef CONFIG_FREETYPE
        force_load_font = 1;
#endif
    } else if (!mpctx->sh_audio)
        goto goto_next_file;

//================== MAIN: ==========================
    current_module = "main";

    if (playing_msg) {
        char *msg = property_expand_string(mpctx, playing_msg);
        mp_msg(MSGT_CPLAYER, MSGL_INFO, "%s", msg);
        free(msg);
    }

    // Disable the term OSD in verbose mode
    if (verbose)
        term_osd = 0;

    {
        mpctx->num_buffered_frames = 0;

        // Make sure old OSD does not stay around,
        // e.g. with -fixed-vo and same-resolution files
        clear_osd_msgs();
        update_osd_msg();

//================ SETUP AUDIO ==========================

        if (mpctx->sh_audio) {
            reinit_audio_chain();
            if (mpctx->sh_audio && mpctx->sh_audio->codec)
                mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_AUDIO_CODEC=%s\n", mpctx->sh_audio->codec->name);
        }

        current_module = "av_init";

        if (mpctx->sh_video) {
            mpctx->sh_video->timer = 0;
            if (!ignore_start)
                audio_delay += mpctx->sh_video->stream_delay;
        }
        if (mpctx->sh_audio) {
            if (start_volume >= 0)
                mixer_setvolume(&mpctx->mixer, start_volume, start_volume);
            if (!ignore_start)
                audio_delay -= mpctx->sh_audio->stream_delay;
            mpctx->delay = -audio_delay;
        }

        if (!mpctx->sh_audio) {
            mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_NoSound);
            mp_msg(MSGT_CPLAYER, MSGL_V, "Freeing %d unused audio chunks.\n", mpctx->d_audio->packs);
            ds_free_packs(mpctx->d_audio); // free buffered chunks
            //mpctx->d_audio->id=-2;         // do not read audio chunks
            //uninit_player(INITIALIZED_AO); // close device
        }
        if (!mpctx->sh_video) {
            mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_Video_NoVideo);
            mp_msg(MSGT_CPLAYER, MSGL_V, "Freeing %d unused video chunks.\n", mpctx->d_video->packs);
            ds_free_packs(mpctx->d_video);
            mpctx->d_video->id = -2;
            //if(!fixed_vo) uninit_player(INITIALIZED_VO);
        }

        if (!mpctx->sh_video && !mpctx->sh_audio)
            goto goto_next_file;

        //if(demuxer->file_format!=DEMUXER_TYPE_AVI) pts_from_bps=0; // it must be 0 for mpeg/asf!
        if (force_fps && mpctx->sh_video) {
            vo_fps = mpctx->sh_video->fps = force_fps;
            mpctx->sh_video->frametime = 1.0f / mpctx->sh_video->fps;
            mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_FPSforced, mpctx->sh_video->fps, mpctx->sh_video->frametime);
        }

#ifdef CONFIG_GUI
        if (use_gui) {
            if (!gui(GUI_SET_VIDEO, mpctx->sh_video))
                goto goto_next_file;
            gui(GUI_SET_AUDIO, mpctx->sh_audio);
        }
#endif

        mp_input_set_section(NULL);
        //TODO: add desired (stream-based) sections here
        if (mpctx->stream->type == STREAMTYPE_TV)
            mp_input_set_section("tv");
        if (mpctx->stream->type == STREAMTYPE_DVDNAV)
            mp_input_set_section("dvdnav");

//==================== START PLAYING =======================

        if (mpctx->loop_times > 1)
            mpctx->loop_times--;
        else if (mpctx->loop_times == 1)
            mpctx->loop_times = -1;

        mp_msg(MSGT_CPLAYER, MSGL_INFO, MSGTR_StartPlaying);

        total_time_usage_start = GetTimer();
        audio_time_usage       = 0;
        video_time_usage       = 0;
        vout_time_usage = 0;
        total_frame_cnt = 0;
        drop_frame_cnt  = 0;         // fix for multifile fps benchmark
        play_n_frames   = play_n_frames_mf;
        mpctx->startup_decode_retry = DEFAULT_STARTUP_DECODE_RETRY;

        if (play_n_frames == 0) {
            mpctx->eof = PT_NEXT_ENTRY;
            goto goto_next_file;
        }

        if (seek_to_sec != MP_NOPTS_VALUE) {
            seek(mpctx, seek_to_sec, SEEK_ABSOLUTE);
            end_at.pos += seek_to_sec;
        }

#ifdef CONFIG_DVDNAV
        mp_dvdnav_context_free(mpctx);
        if (mpctx->stream->type == STREAMTYPE_DVDNAV) {
            mp_dvdnav_read_wait(mpctx->stream, 0, 1);
            mp_dvdnav_cell_has_changed(mpctx->stream, 1);
        }
#endif

        while (!mpctx->eof) {
            float aq_sleep_time = 0;

            if (dvd_last_chapter > 0) {
                int cur_chapter = demuxer_get_current_chapter(mpctx->demuxer);
                if (cur_chapter != -1 && cur_chapter + 1 > dvd_last_chapter)
                    goto goto_next_file;
            }

            if (!mpctx->sh_audio && mpctx->d_audio->sh) {
                mpctx->sh_audio     = mpctx->d_audio->sh;
                mpctx->sh_audio->ds = mpctx->d_audio;
                reinit_audio_chain();
            }

/*========================== PLAY AUDIO ============================*/

            if (mpctx->sh_audio)
                if (!fill_audio_out_buffers())
                    // at eof, all audio at least written to ao
                    if (!mpctx->sh_video)
                        mpctx->eof = PT_NEXT_ENTRY;

            if (!mpctx->sh_video) {
                // handle audio-only case:
                double a_pos = 0;
                // sh_audio can be NULL due to video stream switching
                // TODO: handle this better
                if (mpctx->sh_audio)
                    a_pos = playing_audio_pts(mpctx->sh_audio, mpctx->d_audio, mpctx->audio_out);

                if (!quiet)
                    print_status(a_pos, 0, 0);

                if (is_at_end(mpctx, &end_at, a_pos))
                    mpctx->eof = PT_NEXT_ENTRY;
                update_subtitles(NULL, a_pos, mpctx->d_sub, 0);
                update_osd_msg();
            } else {
                int frame_time_remaining = 0;
                int blit_frame = 1;
                // skip timing after seek
                int skip_timing = mpctx->startup_decode_retry > 0;

/*========================== PLAY VIDEO ============================*/

                vo_pts = mpctx->sh_video->timer * 90000.0;
                vo_fps = mpctx->sh_video->fps;

                if (!mpctx->num_buffered_frames) {
                    double frame_time = update_video(&blit_frame);
                    while (!blit_frame && mpctx->startup_decode_retry > 0) {
                        double delay = mpctx->delay;
                        // these initial decode failures are probably due to codec delay,
                        // ignore them and also their probably nonsense durations
                        update_video(&blit_frame);
                        mpctx->delay = delay;
                        mpctx->startup_decode_retry--;
                    }
                    mpctx->startup_decode_retry = 0;
                    mp_dbg(MSGT_AVSYNC, MSGL_DBG2, "*** ftime=%5.3f ***\n", frame_time);
                    if (mpctx->sh_video->vf_initialized < 0) {
                        mp_msg(MSGT_CPLAYER, MSGL_FATAL, MSGTR_NotInitializeVOPorVO);
                        mpctx->eof = 1;
                        goto goto_next_file;
                    }
                    if (frame_time < 0) {
                        // if we have no more video, sleep some arbitrary time
                        frame_time = 1.0 / 20.0;
                        // only stop playing when audio is at end as well
                        if (!mpctx->sh_audio || mpctx->d_audio->eof)
                            mpctx->eof = 1;
                    } else {
                        // might return with !eof && !blit_frame if !correct_pts
                        mpctx->num_buffered_frames += blit_frame;
                        mpctx->time_frame += frame_time / playback_speed; // for nosound
                    }
                }

// ==========================================================================

//    current_module="draw_osd";
//    if(vo_config_count) mpctx->video_out->draw_osd();

                current_module = "vo_check_events";
                if (vo_config_count)
                    mpctx->video_out->check_events();

#ifdef CONFIG_X11
                if (stop_xscreensaver) {
                    current_module = "stop_xscreensaver";
                    xscreensaver_heartbeat();
                }
#endif
                if (heartbeat_cmd) {
                    static unsigned last_heartbeat;
                    unsigned now = GetTimerMS();
                    // compare as unsigned so that any mistakes (overflow etc)
                    // trigger a heartbeat, thus resetting the logic.
                    if (now - last_heartbeat > (unsigned)(heartbeat_interval * 1000)) {
                        last_heartbeat = now;
                        system(heartbeat_cmd);
                    }
                }

                if (!skip_timing)
                    frame_time_remaining = sleep_until_update(&mpctx->time_frame, &aq_sleep_time);

//====================== FLIP PAGE (VIDEO BLT): =========================

                if (!edl_needs_reset) {
                    current_module = "flip_page";
                    if (!frame_time_remaining && blit_frame) {
                        unsigned int t2 = GetTimer();

                        if (vo_config_count)
                            mpctx->video_out->flip_page();
                        mpctx->num_buffered_frames--;

                        vout_time_usage += (GetTimer() - t2) * 0.000001;
                    }
                }
//====================== A-V TIMESTAMP CORRECTION: =========================

                adjust_sync_and_print_status(frame_time_remaining, mpctx->time_frame);

//============================ Auto QUALITY ============================

                /*Output quality adjustments:*/
                if (auto_quality > 0) {
                    current_module = "autoq";
                    //float total=0.000001f * (GetTimer()-aq_total_time);
                    //if(output_quality<auto_quality && aq_sleep_time>0.05f*total)
                    if (output_quality < auto_quality && aq_sleep_time > 0)
                        ++output_quality;
                    else
                    //if(output_quality>0 && aq_sleep_time<-0.05f*total)
                    if (output_quality > 1 && aq_sleep_time < 0)
                        --output_quality;
                    else if (output_quality > 0 && aq_sleep_time < -0.050f) // 50ms
                        output_quality = 0;
                    //printf("total: %8.6f  sleep: %8.6f  q: %d\n",(0.000001f*aq_total_time),aq_sleep_time,output_quality);
                    set_video_quality(mpctx->sh_video, output_quality);
                }

                if (play_n_frames >= 0 && !frame_time_remaining && blit_frame) {
                    --play_n_frames;
                    if (play_n_frames <= 0)
                        mpctx->eof = PT_NEXT_ENTRY;
                }

                if (!frame_time_remaining && is_at_end(mpctx, &end_at,
                                                       mpctx->sh_video->pts))
                    mpctx->eof = PT_NEXT_ENTRY;
            } // end if(mpctx->sh_video)

#ifdef CONFIG_DVDNAV
            if (mpctx->stream->type == STREAMTYPE_DVDNAV) {
                // do not clobber subtitles
                if (!mp_dvdnav_number_of_subs(mpctx->stream)) {
                nav_highlight_t hl;
                mp_dvdnav_get_highlight(mpctx->stream, &hl);
                if (!vo_spudec || !spudec_apply_palette_crop(vo_spudec, hl.palette, hl.sx, hl.sy, hl.ex, hl.ey)) {
                    osd_set_nav_box(hl.sx, hl.sy, hl.ex, hl.ey);
                    vo_osd_changed(OSDTYPE_DVDNAV);
                } else {
                    osd_set_nav_box(0, 0, 0, 0);
                    vo_osd_changed(OSDTYPE_DVDNAV);
                    vo_osd_changed(OSDTYPE_SPU);
                }
                }

                if (mp_dvdnav_stream_has_changed(mpctx->stream)) {
                    double ar = -1.0;
                    // clear highlight
                    if (vo_spudec)
                        spudec_apply_palette_crop(vo_spudec, 0, 0, 0, 0, 0);
                    if (mpctx->sh_video &&
                        stream_control(mpctx->demuxer->stream,
                                       STREAM_CTRL_GET_ASPECT_RATIO, &ar)
                        != STREAM_UNSUPPORTED)
                        mpctx->sh_video->stream_aspect = ar;
                }
            }
#endif

//============================ Handle PAUSE ===============================

            current_module = "pause";

            if (mpctx->osd_function == OSD_PAUSE) {
                mpctx->was_paused = 1;
                pause_loop();
            }

            // handle -sstep
            if (step_sec > 0) {
                mpctx->osd_function = OSD_FFW;
                rel_seek_secs      += step_sec;
            }

            edl_update(mpctx);

//================= Keyboard events, SEEKing ====================

            current_module = "key_events";

            {
                mp_cmd_t *cmd;
                int brk_cmd = 0;
                while (!brk_cmd && (cmd = mp_input_get_cmd(0, 0, 0)) != NULL) {
                    brk_cmd = run_command(mpctx, cmd);
                    if (cmd->id == MP_CMD_EDL_LOADFILE) {
                        free(edl_filename);
                        edl_filename = strdup(cmd->args[0].v.s);
                        if (edl_filename)
                            edl_loadfile();
                        else
                            mp_msg(MSGT_CPLAYER, MSGL_ERR, MSGTR_EdlOutOfMemFile,
                                   cmd->args[0].v.s);
                    }
                    mp_cmd_free(cmd);
                    if (brk_cmd == 2)
                        goto goto_enable_cache;
                }
            }
            mpctx->was_paused = 0;

            /* Looping. */
            if (mpctx->eof == 1 && mpctx->loop_times >= 0) {
                mp_msg(MSGT_CPLAYER, MSGL_V, "loop_times = %d, eof = %d\n", mpctx->loop_times, mpctx->eof);

                if (mpctx->loop_times > 1)
                    mpctx->loop_times--;
                else if (mpctx->loop_times == 1)
                    mpctx->loop_times = -1;
                play_n_frames = play_n_frames_mf;
                mpctx->eof    = 0;
                abs_seek_pos  = SEEK_ABSOLUTE;
                rel_seek_secs = seek_to_sec;
                if (seek_to_sec == MP_NOPTS_VALUE) {
                    // the first pts is not necessarily 0
                    abs_seek_pos  = SEEK_ABSOLUTE | SEEK_FACTOR;
                    rel_seek_secs = 0;
                }
                loop_seek     = 1;
            }

            if (rel_seek_secs || abs_seek_pos) {
                if (seek(mpctx, rel_seek_secs, abs_seek_pos) >= 0) {
                    // Set OSD:
                    if (!loop_seek) {
                        if (!edl_decision)
                            set_osd_bar(0, "Position", 0, 100, demuxer_get_percent_pos(mpctx->demuxer));
                    }
                }

                rel_seek_secs = 0;
                abs_seek_pos  = 0;
                loop_seek     = 0;
                edl_decision  = 0;
            }

#ifdef CONFIG_GUI
            if (use_gui) {
                if (mpctx->demuxer->file_format == DEMUXER_TYPE_AVI && mpctx->sh_video && mpctx->sh_video->video.dwLength > 2) {
                    // get pos from frame number / total frames
                    guiInfo.Position = (float)mpctx->d_video->pack_no * 100.0f / mpctx->sh_video->video.dwLength;
                } else {
                    guiInfo.Position = demuxer_get_percent_pos(mpctx->demuxer);
                }
                if (mpctx->sh_video)
                    guiInfo.ElapsedTime = mpctx->sh_video->pts;
                else if (mpctx->sh_audio)
                    guiInfo.ElapsedTime = playing_audio_pts(mpctx->sh_audio, mpctx->d_audio, mpctx->audio_out);
                guiInfo.RunningTime = demuxer_get_time_length(mpctx->demuxer);
                gui(GUI_SET_MIXER, 0);
                gui(GUI_REDRAW, 0);
                if (guiInfo.Playing == GUI_STOP)
                    break;                  // STOP
                if (guiInfo.Playing == GUI_PAUSE)
                    mpctx->osd_function = OSD_PAUSE;
                if (guiInfo.NewPlay)
                    goto goto_next_file;
#ifdef CONFIG_DVDREAD
                if (mpctx->stream->type == STREAMTYPE_DVD) {
                    dvd_priv_t *dvdp = mpctx->stream->priv;
                    guiInfo.Chapter = dvd_chapter_from_cell(dvdp, guiInfo.Track - 1, dvdp->cur_cell) + 1;
                }
#endif
            }
#endif /* CONFIG_GUI */
        } // while(!mpctx->eof)

        mp_msg(MSGT_GLOBAL, MSGL_V, "EOF code: %d  \n", mpctx->eof);

#ifdef CONFIG_DVBIN
        if (mpctx->dvbin_reopen) {
            mpctx->eof = 0;
            uninit_player(INITIALIZED_ALL - (INITIALIZED_GUI | INITIALIZED_STREAM | INITIALIZED_INPUT | INITIALIZED_GETCH2 | (fixed_vo ? INITIALIZED_VO : 0)));
            cache_uninit(mpctx->stream);
            mpctx->dvbin_reopen = 0;
            goto goto_enable_cache;
        }
#endif
    }

goto_next_file:  // don't jump here after ao/vo/getch initialization!

    mp_msg(MSGT_CPLAYER, MSGL_INFO, "\n");

    if (benchmark) {
        double tot = video_time_usage + vout_time_usage + audio_time_usage;
        double total_time_usage;
        total_time_usage_start = GetTimer() - total_time_usage_start;
        total_time_usage       = (float)total_time_usage_start * 0.000001;
        mp_msg(MSGT_CPLAYER, MSGL_INFO, "\nBENCHMARKs: VC:%8.3fs VO:%8.3fs A:%8.3fs Sys:%8.3fs = %8.3fs\n",
               video_time_usage, vout_time_usage, audio_time_usage,
               total_time_usage - tot, total_time_usage);
        if (total_time_usage > 0.0)
            mp_msg(MSGT_CPLAYER, MSGL_INFO, "BENCHMARK%%: VC:%8.4f%% VO:%8.4f%% A:%8.4f%% Sys:%8.4f%% = %8.4f%%\n",
                   100.0 * video_time_usage         / total_time_usage,
                   100.0 * vout_time_usage          / total_time_usage,
                   100.0 * audio_time_usage         / total_time_usage,
                   100.0 * (total_time_usage - tot) / total_time_usage,
                   100.0);
        if (total_frame_cnt && frame_dropping)
            mp_msg(MSGT_CPLAYER, MSGL_INFO, "BENCHMARKn: disp: %d (%3.2f fps)  drop: %d (%d%%)  total: %d (%3.2f fps)\n",
                   total_frame_cnt - drop_frame_cnt,
                   (total_time_usage > 0.5) ? ((total_frame_cnt - drop_frame_cnt) / total_time_usage) : 0,
                   drop_frame_cnt,
                   100 * drop_frame_cnt / total_frame_cnt,
                   total_frame_cnt,
                   (total_time_usage > 0.5) ? (total_frame_cnt / total_time_usage) : 0);
    }

    // time to uninit all, except global stuff:
    uninit_player(INITIALIZED_ALL - (INITIALIZED_GUI + INITIALIZED_INPUT + (fixed_vo ? INITIALIZED_VO : 0)));

    if (mpctx->eof == PT_NEXT_ENTRY || mpctx->eof == PT_PREV_ENTRY) {
        mpctx->eof = mpctx->eof == PT_NEXT_ENTRY ? 1 : -1;
        if (play_tree_iter_step(mpctx->playtree_iter, mpctx->play_tree_step, 0) == PLAY_TREE_ITER_ENTRY) {
            mpctx->eof = 1;
        } else {
            play_tree_iter_free(mpctx->playtree_iter);
            mpctx->playtree_iter = NULL;
        }
        mpctx->play_tree_step = 1;
    } else if (mpctx->eof == PT_UP_NEXT || mpctx->eof == PT_UP_PREV) {
        mpctx->eof = mpctx->eof == PT_UP_NEXT ? 1 : -1;
        if (mpctx->playtree_iter) {
            if (play_tree_iter_up_step(mpctx->playtree_iter, mpctx->eof, 0) == PLAY_TREE_ITER_ENTRY) {
                mpctx->eof = 1;
            } else {
                play_tree_iter_free(mpctx->playtree_iter);
                mpctx->playtree_iter = NULL;
            }
        }
    } else if (mpctx->eof == PT_STOP) {
        play_tree_iter_free(mpctx->playtree_iter);
        mpctx->playtree_iter = NULL;
    } else { // NEXT PREV SRC
        mpctx->eof = mpctx->eof == PT_PREV_SRC ? -1 : 1;
    }

    if (mpctx->eof == 0)
        mpctx->eof = 1;

    while (mpctx->playtree_iter != NULL) {
        filename = play_tree_iter_get_file(mpctx->playtree_iter, mpctx->eof);
        if (filename == NULL) {
            if (play_tree_iter_step(mpctx->playtree_iter, mpctx->eof, 0) != PLAY_TREE_ITER_ENTRY) {
                play_tree_iter_free(mpctx->playtree_iter);
                mpctx->playtree_iter = NULL;
            }
        } else
            break;
    }

#ifdef CONFIG_GUI
    if (use_gui)
        if (guiInfo.NewPlay != GUI_FILE_SAME)
            gui(GUI_END_FILE, 0);
#endif

    if (
#ifdef CONFIG_GUI
        (use_gui && guiInfo.Playing) ||
#endif
                                        mpctx->playtree_iter != NULL || player_idle_mode) {
        if (!mpctx->playtree_iter && !use_gui)
            filename = NULL;
        mpctx->eof = 0;
        goto play_next_file;
    }

    exit_player_with_rc(EXIT_EOF, 0);
    return 1;
}

#endif /* DISABLE_MAIN */
