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

#define gtkSetContrast      0
#define gtkSetBrightness    1
#define gtkSetHue           2
#define gtkSetSaturation    3
#define gtkSetEqualizer     4
#define gtkSetExtraStereo   10
#define gtkSetPanscan       11
#define gtkSetFontFactor    12
#define gtkSetAutoq         13
#define gtkClearStruct      14
#define gtkSetFontOutLine   16
#define gtkSetFontBlur      17
#define gtkSetFontTextScale 18
#define gtkSetFontOSDScale  19
#define gtkSetFontEncoding  20
#define gtkSetFontAutoScale 21
#define gtkSetSubEncoding   22

#define fsPersistant_MaxPos 5

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

extern char *fsHistory[fsPersistant_MaxPos];

/* MPlayer -> GUI */

void guiDone(void);
int gui(int what, void *arg);
void guiInit(void);
int guiInitializePlaylist(play_tree_t *my_playtree, m_config_t *config, int enqueue);
int guiAddPlaylist(play_tree_t *my_playtree, m_config_t *config);

/* GUI -> MPlayer */

void gmp_msg(int mod, int lev, const char *format, ...);
void mplayer(int cmd, float fparam, void *vparam);
void guiExit(enum exit_reason how);
void guiLoadFont(void);
void guiLoadSubtitle(char *name);

#endif /* MPLAYER_GUI_INTERFACE_H */
