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

#ifndef MPLAYER_GUI_APP_H
#define MPLAYER_GUI_APP_H

#include "util/bitmap.h"
#include "wm/ws.h"

// User events

#define evNone              0
#define evPlay              1
#define evStop              2
#define evPause             3
#define evPrev              6
#define evNext              7
#define evLoad              8
#define evEqualizer         9
#define evPlayList          10
#define evIconify           11
#define evAbout             12
#define evLoadPlay          13
#define evPreferences       14
#define evSkinBrowser       15
#define evPlaySwitchToPause 16
#define evPauseSwitchToPlay 17

#define evBackward10sec     18
#define evForward10sec      19
#define evBackward1min      20
#define evForward1min       21
#define evBackward10min     22
#define evForward10min      23

#define evHalfSize          301
#define evNormalSize        24
#define evDoubleSize        25
#define evFullScreen        26

#define evSetMoviePosition  27
#define evSetVolume         28
#define evSetBalance        29
#define evMute              30

#define evIncVolume         31
#define evDecVolume         32
#define evIncAudioBufDelay  33   // NOTE TO MYSELF: not all of these events
#define evDecAudioBufDelay  34   // are actually implemented, and update doc
#define evIncBalance        35
#define evDecBalance        36

#define evHelp              37

#define evLoadSubtitle      38
#define evDropSubtitle      43
#define evPlayDVD           39
#define evPlayVCD           40
#define evPlayNetwork       41
#define evLoadAudioFile     42
#define evSetAspect         44
#define evSetAudio          45
#define evSetVideo          46
#define evSetSubtitle       47

#define evExit              1000

// General events

#define evFileLoaded      5000
#define evHideMouseCursor 5001
#define evMessageBox      5002
#define evGeneralTimer    5003
#define evGtkIsOk         5004
#define evShowPopUpMenu   5005
#define evHidePopUpMenu   5006
#define evSetDVDAudio     5007
#define evSetDVDSubtitle  5008
#define evSetDVDTitle     5009
#define evSetDVDChapter   5010
#define evSubtitleLoaded  5011
#define evSetVCDTrack     5012
#define evSetURL          5013

#define evFName           7000
#define evMovieTime       7001
#define evRedraw          7002
#define evHideWindow      7003
#define evShowWindow      7004
#define evFirstLoad       7005

typedef struct {
    int message;
    const char *name;
} evName;

// Skin items

#define itNone      0
#define itButton    101
#define itHPotmeter 102
#define itVPotmeter 103
#define itSLabel    104
#define itDLabel    105
#define itBase      106
#define itPotmeter  107
#define itMenu      108

#define itPLMButton (itNone - 1)
#define itPRMButton (itNone - 2)

// Button states

#define btnDisabled 0
#define btnReleased 1
#define btnPressed  2

// Item definition

#define MAX_ITEMS 64

typedef struct {
    int type;

    int x, y;
    int width, height;

    txSample Bitmap;
    txSample Mask;

    int fontid;
    int align;
    char *label;

    int pwidth, pheight;
    int numphases;
    float value;

    int message;

    int R, G, B;

    char *text;
    int textwidth;
    unsigned int starttime;
    int last_x;

    int pressed;
} wItem;

typedef struct {
    wItem main;
    wsTWindow mainWindow;
    int mainDecoration;

    wItem sub;
    wsTWindow subWindow;

    wItem bar;
    wsTWindow barWindow;
    int barIsPresent;

    wItem menu;
    wItem menuSelected;
    wsTWindow menuWindow;
    int menuIsPresent;

    int IndexOfMainItems;
    wItem mainItems[MAX_ITEMS];

    int IndexOfBarItems;
    wItem barItems[MAX_ITEMS];

    int IndexOfMenuItems;
    wItem menuItems[MAX_ITEMS];
} guiItems;

extern guiItems appMPlayer;

int appFindMessage(unsigned char *str);
void appFreeStruct(void);
void btnModify(int event, float state);
void btnSet(int event, int set);

#endif /* MPLAYER_GUI_APP_H */
