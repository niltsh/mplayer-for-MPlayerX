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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"
#include "interface.h"
#include "util/list.h"
#include "util/string.h"

#include "config.h"
#include "help_mp.h"
#include "libmpcodecs/vd.h"
#include "libmpdemux/demuxer.h"
#include "libvo/video_out.h"
#include "libvo/x11_common.h"
#include "mixer.h"
#include "mp_core.h"
#include "mp_msg.h"
#include "mpcommon.h"
#include "mplayer.h"
#include "parser-cfg.h"
#include "path.h"
#include "stream/stream.h"
#include "sub/ass_mp.h"
#include "sub/font_load.h"
#include "sub/sub.h"

#define GUI_CONFIGURATION "gui.conf"
#define GUI_HISTORY       "gui.history"
#define GUI_PLAYLIST      "gui.pl"
#define GUI_URLLIST       "gui.url"

m_config_t *gui_conf;

int gtkCacheOn;
int gtkCacheSize = 2048;

int gtkVfPP;

int gtkVfLAVC;
char *gtkDXR3Device;

int gtkAutoSyncOn;
int gtkAutoSync;

int gtkAONorm;
int gtkAOSurround;
int gtkAOExtraStereo;
float gtkAOExtraStereoMul = 1.0f;

char *gtkAOALSAMixer;
char *gtkAOALSAMixerChannel;
char *gtkAOALSADevice;

char *gtkAOOSSMixer;
char *gtkAOOSSMixerChannel;
char *gtkAOOSSDevice;

char *gtkAOESDDevice;

char *gtkAOSDLDriver;

int gtkEnableAudioEqualizer;
float gtkEquChannels[6][10];

int gtkSubDumpMPSub;
int gtkSubDumpSrt;

gtkASS_t gtkASS;

int gtkEnablePlayBar = 1;
int gtkLoadFullscreen;
int gtkShowVideoWindow = 1;

int gui_save_pos   = 1;
int gui_main_pos_x = -3;
int gui_main_pos_y = -3;
int gui_sub_pos_x  = -3;
int gui_sub_pos_y  = -3;

int guiWinID = -1;

char *skinName;

char *fsHistory[5];

static const m_option_t gui_opts[] = {
    { "cache",                       &gtkCacheOn,              CONF_TYPE_FLAG,        0,           0,     1,       NULL },
    { "cache_size",                  &gtkCacheSize,            CONF_TYPE_INT,         CONF_RANGE,  32,    1048576, NULL },

    { "vf_pp",                       &gtkVfPP,                 CONF_TYPE_FLAG,        0,           0,     1,       NULL },

#ifdef CONFIG_DXR3
    { "vf_lavc",                     &gtkVfLAVC,               CONF_TYPE_FLAG,        0,           0,     1,       NULL },
    { "vo_dxr3_device",              &gtkDXR3Device,           CONF_TYPE_STRING,      0,           0,     0,       NULL },
#endif

    { "autosync",                    &gtkAutoSyncOn,           CONF_TYPE_FLAG,        0,           0,     1,       NULL },
    { "autosync_size",               &gtkAutoSync,             CONF_TYPE_INT,         CONF_RANGE,  0,     10000,   NULL },

    { "ao_volnorm",                  &gtkAONorm,               CONF_TYPE_FLAG,        0,           0,     1,       NULL },
    { "ao_surround",                 &gtkAOSurround,           CONF_TYPE_FLAG,        0,           0,     1,       NULL },
    { "ao_extra_stereo",             &gtkAOExtraStereo,        CONF_TYPE_FLAG,        0,           0,     1,       NULL },
    { "ao_extra_stereo_coefficient", &gtkAOExtraStereoMul,     CONF_TYPE_FLOAT,       CONF_RANGE,  -10.0, 10.0,    NULL },

#ifdef CONFIG_ALSA
    { "ao_alsa_mixer",               &gtkAOALSAMixer,          CONF_TYPE_STRING,      0,           0,     0,       NULL },
    { "ao_alsa_mixer_channel",       &gtkAOALSAMixerChannel,   CONF_TYPE_STRING,      0,           0,     0,       NULL },
    { "ao_alsa_device",              &gtkAOALSADevice,         CONF_TYPE_STRING,      0,           0,     0,       NULL },
#endif

#ifdef CONFIG_OSS_AUDIO
    { "ao_oss_mixer",                &gtkAOOSSMixer,           CONF_TYPE_STRING,      0,           0,     0,       NULL },
    { "ao_oss_mixer_channel",        &gtkAOOSSMixerChannel,    CONF_TYPE_STRING,      0,           0,     0,       NULL },
    { "ao_oss_device",               &gtkAOOSSDevice,          CONF_TYPE_STRING,      0,           0,     0,       NULL },
#endif

#ifdef CONFIG_ESD
    { "ao_esd_device",               &gtkAOESDDevice,          CONF_TYPE_STRING,      0,           0,     0,       NULL },
#endif

#ifdef CONFIG_SDL
    { "ao_sdl_subdriver",            &gtkAOSDLDriver,          CONF_TYPE_STRING,      0,           0,     0,       NULL },
#endif

    { "enable_audio_equ",            &gtkEnableAudioEqualizer, CONF_TYPE_FLAG,        0,           0,     1,       NULL },

    { "equ_channel_1",               &gtkEquChannel1,          CONF_TYPE_STRING,      0,           0,     0,       NULL },
    { "equ_channel_2",               &gtkEquChannel2,          CONF_TYPE_STRING,      0,           0,     0,       NULL },
    { "equ_channel_3",               &gtkEquChannel3,          CONF_TYPE_STRING,      0,           0,     0,       NULL },
    { "equ_channel_4",               &gtkEquChannel4,          CONF_TYPE_STRING,      0,           0,     0,       NULL },
    { "equ_channel_5",               &gtkEquChannel5,          CONF_TYPE_STRING,      0,           0,     0,       NULL },
    { "equ_channel_6",               &gtkEquChannel6,          CONF_TYPE_STRING,      0,           0,     0,       NULL },

#define audio_equ_row(i, j) \
    { "equ_band_" # i # j,           &gtkEquChannels[i][j],    CONF_TYPE_FLOAT,       CONF_RANGE,  -15.0, 15.0,    NULL }
    audio_equ_row(0, 0), audio_equ_row(0, 1), audio_equ_row(0, 2), audio_equ_row(0, 3), audio_equ_row(0, 4), audio_equ_row(0, 5), audio_equ_row(0, 6), audio_equ_row(0, 7), audio_equ_row(0, 8), audio_equ_row(0, 9),
    audio_equ_row(1, 0), audio_equ_row(1, 1), audio_equ_row(1, 2), audio_equ_row(1, 3), audio_equ_row(1, 4), audio_equ_row(1, 5), audio_equ_row(1, 6), audio_equ_row(1, 7), audio_equ_row(1, 8), audio_equ_row(1, 9),
    audio_equ_row(2, 0), audio_equ_row(2, 1), audio_equ_row(2, 2), audio_equ_row(2, 3), audio_equ_row(2, 4), audio_equ_row(2, 5), audio_equ_row(2, 6), audio_equ_row(2, 7), audio_equ_row(2, 8), audio_equ_row(2, 9),
    audio_equ_row(3, 0), audio_equ_row(3, 1), audio_equ_row(3, 2), audio_equ_row(3, 3), audio_equ_row(3, 4), audio_equ_row(3, 5), audio_equ_row(3, 6), audio_equ_row(3, 7), audio_equ_row(3, 8), audio_equ_row(3, 9),
    audio_equ_row(4, 0), audio_equ_row(4, 1), audio_equ_row(4, 2), audio_equ_row(4, 3), audio_equ_row(4, 4), audio_equ_row(4, 5), audio_equ_row(4, 6), audio_equ_row(4, 7), audio_equ_row(4, 8), audio_equ_row(4, 9),
    audio_equ_row(5, 0), audio_equ_row(5, 1), audio_equ_row(5, 2), audio_equ_row(5, 3), audio_equ_row(5, 4), audio_equ_row(5, 5), audio_equ_row(5, 6), audio_equ_row(5, 7), audio_equ_row(5, 8), audio_equ_row(5, 9),
#undef audio_equ_row

    { "playbar",                     &gtkEnablePlayBar,        CONF_TYPE_FLAG,        0,           0,     1,       NULL },
    { "load_fullscreen",             &gtkLoadFullscreen,       CONF_TYPE_FLAG,        0,           0,     1,       NULL },
    { "show_videowin",               &gtkShowVideoWindow,      CONF_TYPE_FLAG,        0,           0,     1,       NULL },

    { "gui_save_pos",                &gui_save_pos,            CONF_TYPE_FLAG,        0,           0,     1,       NULL },
    { "gui_main_pos_x",              &gui_main_pos_x,          CONF_TYPE_INT,         0,           0,     0,       NULL },
    { "gui_main_pos_y",              &gui_main_pos_y,          CONF_TYPE_INT,         0,           0,     0,       NULL },
    { "gui_video_out_pos_x",         &gui_sub_pos_x,           CONF_TYPE_INT,         0,           0,     0,       NULL },
    { "gui_video_out_pos_y",         &gui_sub_pos_y,           CONF_TYPE_INT,         0,           0,     0,       NULL },

    { "idle",                        &player_idle_mode,        CONF_TYPE_FLAG,        CONF_GLOBAL, 0,     1,       NULL },

    // NOTE TO MYSELF: Do we really need all/any non-gtkOptions, i.e. override mplayer options?

    { "gui_skin",                    &skinName,                CONF_TYPE_STRING,      0,           0,     0,       NULL },

    { "stopxscreensaver",            &stop_xscreensaver,       CONF_TYPE_FLAG,        0,           0,     1,       NULL },

    { "cdrom_device",                &cdrom_device,            CONF_TYPE_STRING,      0,           0,     0,       NULL },
    { "dvd_device",                  &dvd_device,              CONF_TYPE_STRING,      0,           0,     0,       NULL },

    { "osd_level",                   &osd_level,               CONF_TYPE_INT,         CONF_RANGE,  0,     3,       NULL },

    { "vo_driver",                   &video_driver_list,       CONF_TYPE_STRING_LIST, 0,           0,     0,       NULL },
    { "v_vfm",                       &video_fm_list,           CONF_TYPE_STRING_LIST, 0,           0,     0,       NULL },

    { "vf_autoq",                    &auto_quality,            CONF_TYPE_INT,         CONF_RANGE,  0,     100,     NULL },
    { "vo_direct_render",            &vo_directrendering,      CONF_TYPE_FLAG,        0,           0,     1,       NULL },
    { "vo_doublebuffering",          &vo_doublebuffering,      CONF_TYPE_FLAG,        0,           0,     1,       NULL },
    { "vo_panscan",                  &vo_panscan,              CONF_TYPE_FLOAT,       CONF_RANGE,  0.0,   1.0,     NULL },

    { "v_flip",                      &flip,                    CONF_TYPE_INT,         CONF_RANGE,  -1,    1,       NULL },
    { "v_framedrop",                 &frame_dropping,          CONF_TYPE_INT,         CONF_RANGE,  0,     2,       NULL },
    { "v_idx",                       &index_mode,              CONF_TYPE_INT,         CONF_RANGE,  -1,    2,       NULL },
    { "v_ni",                        &force_ni,                CONF_TYPE_FLAG,        0,           0,     1,       NULL },

    { "ao_driver",                   &audio_driver_list,       CONF_TYPE_STRING_LIST, 0,           0,     0,       NULL },
    { "a_afm",                       &audio_fm_list,           CONF_TYPE_STRING_LIST, 0,           0,     0,       NULL },

    { "softvol",                     &soft_vol,                CONF_TYPE_FLAG,        0,           0,     1,       NULL },

    { "sub_auto_load",               &sub_auto,                CONF_TYPE_FLAG,        0,           0,     1,       NULL },
#ifdef CONFIG_ICONV
    { "sub_cp",                      &sub_cp,                  CONF_TYPE_STRING,      0,           0,     0,       NULL },
#endif
    { "sub_overlap",                 &suboverlap_enabled,      CONF_TYPE_FLAG,        0,           0,     0,       NULL },
    { "sub_pos",                     &sub_pos,                 CONF_TYPE_INT,         CONF_RANGE,  0,     200,     NULL },
    { "sub_unicode",                 &sub_unicode,             CONF_TYPE_FLAG,        0,           0,     1,       NULL },

    { "font_factor",                 &font_factor,             CONF_TYPE_FLOAT,       CONF_RANGE,  0.0,   10.0,    NULL },
    { "font_name",                   &font_name,               CONF_TYPE_STRING,      0,           0,     0,       NULL },

#ifdef CONFIG_FREETYPE
    { "font_encoding",               &subtitle_font_encoding,  CONF_TYPE_STRING,      0,           0,     0,       NULL },
    { "font_text_scale",             &text_font_scale_factor,  CONF_TYPE_FLOAT,       CONF_RANGE,  0.0,   100.0,   NULL },
    { "font_osd_scale",              &osd_font_scale_factor,   CONF_TYPE_FLOAT,       CONF_RANGE,  0.0,   100.0,   NULL },
    { "font_blur",                   &subtitle_font_radius,    CONF_TYPE_FLOAT,       CONF_RANGE,  0.0,   8.0,     NULL },
    { "font_outline",                &subtitle_font_thickness, CONF_TYPE_FLOAT,       CONF_RANGE,  0.0,   8.0,     NULL },
    { "font_autoscale",              &subtitle_autoscale,      CONF_TYPE_INT,         CONF_RANGE,  0,     3,       NULL },
#endif

#ifdef CONFIG_ASS
    { "ass_enabled",                 &ass_enabled,             CONF_TYPE_FLAG,        0,           0,     1,       NULL },
    { "ass_use_margins",             &ass_use_margins,         CONF_TYPE_FLAG,        0,           0,     1,       NULL },
    { "ass_top_margin",              &ass_top_margin,          CONF_TYPE_INT,         CONF_RANGE,  0,     512,     NULL },
    { "ass_bottom_margin",           &ass_bottom_margin,       CONF_TYPE_INT,         CONF_RANGE,  0,     512,     NULL },
#endif

    { NULL,                          NULL,                     0,                     0,           0,     0,       NULL }
};

int cfg_gui_include(m_option_t *conf, const char *filename)
{
    (void)conf;

    return m_config_parse_config_file(gui_conf, filename, 0);
}

void cfg_read(void)
{
    char *fname, line[512];
    FILE *file;

    player_idle_mode = 1;   // GUI is in idle mode by default

    // configuration

    fname = get_path(GUI_CONFIGURATION);

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[cfg] file: %s\n", fname);

    gui_conf = m_config_new();

    if (!gui_conf) {
        gmp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_MemAllocFailed);
        mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
    }

    m_config_register_options(gui_conf, gui_opts);

    if (!disable_gui_conf && (m_config_parse_config_file(gui_conf, fname, 1) < 0)) {
        gmp_msg(MSGT_GPLAYER, MSGL_ERR, MSGTR_ConfigFileError "\n");
        mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
    }

    free(fname);

    // playlist

    fname = get_path(GUI_PLAYLIST);
    file  = fopen(fname, "rt");

    if (file) {
        while (fgetstr(line, sizeof(line), file)) {
            plItem *item;

            item = calloc(1, sizeof(plItem));

            if (!item) {
                gmp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_MemAllocFailed);
                mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
            }

            item->path = strdup(line);
            fgetstr(line, sizeof(line), file);
            item->name = strdup(line);
            listSet(gtkAddPlItem, item);
        }

        fclose(file);
    }

    free(fname);

    // URL list

    fname = get_path(GUI_URLLIST);
    file  = fopen(fname, "rt");

    if (file) {
        while (fgetstr(line, sizeof(line), file)) {
            urlItem *item;

            item = calloc(1, sizeof(urlItem));

            if (!item) {
                gmp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_MemAllocFailed);
                mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
            }

            item->url = strdup(line);
            listSet(gtkAddURLItem, item);
        }

        fclose(file);
    }

    free(fname);

    // directory history

    fname = get_path(GUI_HISTORY);
    file  = fopen(fname, "rt");

    if (file) {
        int i = 0;

        while (fgetstr(line, sizeof(line), file))
            fsHistory[i++] = gstrdup(line);

        fclose(file);
    }

    free(fname);
}

void cfg_write(void)
{
    char *fname;
    FILE *file;
    int i;

    // configuration

    fname = get_path(GUI_CONFIGURATION);
    file  = fopen(fname, "wt+");

    if (file) {
        for (i = 0; gui_opts[i].name; i++) {
            char *val = m_option_print(&gui_opts[i], gui_opts[i].p);

            if (val == (char *)-1) {
                mp_msg(MSGT_GPLAYER, MSGL_WARN, MSGTR_UnableToSaveOption, gui_opts[i].name);
                val = NULL;
            }

            if (val) {
                fprintf(file, "%s = \"%s\"\n", gui_opts[i].name, val);
                free(val);
            }
        }

        fclose(file);
    }

    free(fname);

    // playlist

    fname = get_path(GUI_PLAYLIST);
    file  = fopen(fname, "wt+");

    if (file) {
        plCurrent = plList;

        while (plCurrent) {
            if (plCurrent->path && plCurrent->name) {
                fprintf(file, "%s\n", plCurrent->path);
                fprintf(file, "%s\n", plCurrent->name);
            }

            plCurrent = plCurrent->next;
        }

        fclose(file);
    }

    free(fname);

    // URL list

    fname = get_path(GUI_URLLIST);
    file  = fopen(fname, "wt+");

    if (file) {
        while (urlList) {
            if (urlList->url)
                fprintf(file, "%s\n", urlList->url);

            urlList = urlList->next;
        }

        fclose(file);
    }

    free(fname);

    // directory history

    fname = get_path(GUI_HISTORY);
    file  = fopen(fname, "wt+");

    if (file) {
        int i = 0;

// while (fsHistory[i] != NULL)
        for (i = 0; i < 5; i++)
            if (fsHistory[i])
                fprintf(file, "%s\n", fsHistory[i]);

        fclose(file);
    }

    free(fname);
}
