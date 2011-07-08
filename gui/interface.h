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

#ifndef MPLAYER_GUI_INTERFACE_H
#define MPLAYER_GUI_INTERFACE_H

#include "config.h"
#include "m_config.h"
#include "mp_core.h"
#include "playtree.h"
#include "stream/stream.h"

// These are in support of the non-GUI files that interact with
// the GUI and that only need to include interface.h for this.
// ------------------------------------------------------------
#include "cfg.h"

extern int use_gui;             // this is defined in mplayer.c
// ------------------------------------------------------------

#define GMPlayer "gmplayer"

#define GUI_HANDLE_X_EVENT     0
#define GUI_SET_STATE          1
#define GUI_RUN_COMMAND        2
#define GUI_SET_AUDIO          6
#define GUI_SETUP_VIDEO_WINDOW 8
#define GUI_SET_STREAM         9
#define GUI_REDRAW             10
#define GUI_SET_MIXER          11
#define GUI_SET_FILE           12
#define GUI_SET_VIDEO          13
#define GUI_PREPARE            16
#define GUI_SET_AFILTER        17
#define GUI_SET_CONTEXT        18
#define GUI_END_FILE           19
#define GUI_HANDLE_EVENTS      20

#define GUI_STOP  0
#define GUI_PLAY  1
#define GUI_PAUSE 2

// NOTE TO MYSELF: This should rather be in app.h.
#define guiDVD       1
#define guiVCD       2
#define guiFilenames 4
#define guiALL       0xffffffff
#define gtkClearStruct      14

#define MPLAYER_SET_CONTRAST       0
#define MPLAYER_SET_BRIGHTNESS     1
#define MPLAYER_SET_HUE            2
#define MPLAYER_SET_SATURATION     3
#define MPLAYER_SET_EQUALIZER      4
#define MPLAYER_SET_EXTRA_STEREO   10
#define MPLAYER_SET_PANSCAN        11
#define MPLAYER_SET_FONT_FACTOR    12
#define MPLAYER_SET_AUTO_QUALITY   13
#define MPLAYER_SET_FONT_OUTLINE   16
#define MPLAYER_SET_FONT_BLUR      17
#define MPLAYER_SET_FONT_TEXTSCALE 18
#define MPLAYER_SET_FONT_OSDSCALE  19
#define MPLAYER_SET_FONT_ENCODING  20
#define MPLAYER_SET_FONT_AUTOSCALE 21
#define MPLAYER_SET_SUB_ENCODING   22

typedef struct {
    int x;
    int y;
    int width;
    int height;
} guiResizeStruct;

typedef struct {
    int signal;
    char module[512];
} guiUnknownErrorStruct;

typedef struct {
    int seek;
    int format;
    int width;
    int height;
    char codecdll[128];
} guiVideoStruct;

#ifdef CONFIG_DVDREAD
typedef struct {
    int titles;
    int chapters;
    int angles;
    int current_chapter;
    int current_title;
    int current_angle;
    int nr_of_audio_channels;
    stream_language_t audio_streams[32];
    int nr_of_subtitles;
    stream_language_t subtitles[32];
} guiDVDStruct;
#endif

typedef struct {
    int message;
    guiResizeStruct resize;
    guiVideoStruct videodata;
    guiUnknownErrorStruct error;

    struct MPContext *mpcontext;
    void *sh_video;
    void *afilter;
    void *event_struct;

    int DiskChanged;
    int NewPlay;

#ifdef CONFIG_DVDREAD
    guiDVDStruct DVD;
    int Title;
    int Angle;
    int Chapter;
#endif

#ifdef CONFIG_VCD
    int VCDTracks;
#endif

    int Playing;
    float Position;

    int MovieWidth;
    int MovieHeight;
    int MovieWindow;

    float Volume;
    float Balance;

    int Track;
    int AudioChannels;
    int StreamType;
    int TimeSec;
    int LengthInSec;
    int FrameDrop;
    float FPS;

    char *Filename;
    int FilenameChanged;

    char *Subtitlename;
    int SubtitleChanged;

    char *Othername;
    int OtherChanged;

    char *AudioFile;
    int AudioFileChanged;

    int SkinChange;
} guiInterface_t;

extern guiInterface_t guiInfo;

/* MPlayer -> GUI */

int gui(int what, void *arg);
void guiDone(void);
void guiInit(void);
int guiPlaylistAdd(play_tree_t *my_playtree, m_config_t *config);
int guiPlaylistInitialize(play_tree_t *my_playtree, m_config_t *config, int enqueue);

/* GUI -> MPlayer */

void mplayer(int what, float fparam, void *vparam);
void mplayerLoadFont(void);
void mplayerLoadSubtitle(char *name);
void gmp_msg(int mod, int lev, const char *format, ...);
void guiExit(enum exit_reason how);

#endif /* MPLAYER_GUI_INTERFACE_H */
