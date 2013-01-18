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

#include "gui/util/bitmap.h"
#include "gui/wm/ws.h"

/// GUI messages (user events)
enum {
    evNone,
    evPlay,
    evStop,
    evPause,
    evPrev,
    evNext,
    evLoad,
    evLoadPlay,
    evLoadAudioFile,
    evLoadSubtitle,
    evDropSubtitle,
    evPlaylist,
    evPlayCD,
    evPlayVCD,
    evPlayDVD,
    evLoadURL,
    evPlaySwitchToPause,
    evPauseSwitchToPlay,
    evBackward10sec,
    evForward10sec,
    evBackward1min,
    evForward1min,
    evBackward10min,
    evForward10min,
    evSetMoviePosition,
    evHalfSize,
    evDoubleSize,
    evFullScreen,
    evNormalSize,
    evSetAspect,
    evIncVolume,
    evDecVolume,
    evSetVolume,
    evMute,
    evSetBalance,
    evEqualizer,
    evAbout,
    evPreferences,
    evSkinBrowser,
    evMenu,
    evIconify,
    evExit = 100
};

/// Internal messages (events)
enum {
    ivSetVideo = 1000,
    ivSetAudio,
    ivSetSubtitle,
    ivSetCDTrack,
    ivSetVCDTrack,
    ivSetDVDTitle,
    ivSetDVDChapter,
    ivSetDVDAudio,
    ivSetDVDSubtitle,
    ivPlayDVD,
    ivShowPopUpMenu,
    ivHidePopUpMenu,
    ivRedraw
};

typedef struct {
    int message;
    const char *name;
} evName;

/// Skin items
enum {
    itNone,
    itBase,
    itButton,
    itSLabel,
    itDLabel,
    itHPotmeter,
    itVPotmeter,
    itPotmeter,
    itMenu,
    itPLMButton = 100,
    itPRMButton
};

//@{
/// Button state
#define btnDisabled 0
#define btnReleased 1
#define btnPressed  2
//@}

/// Item definition
typedef struct {
    int type;

    int x, y;
    int width, height;

    guiImage Bitmap;
    guiImage Mask;

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
} guiItem;

/// Maximum GUI items
#define MAX_ITEMS 64

typedef struct {
    guiItem main;
    wsWindow mainWindow;
    int mainDecoration;

    guiItem video;
    wsWindow videoWindow;

    guiItem playbar;
    wsWindow playbarWindow;
    int playbarIsPresent;

    guiItem menu;
    guiItem menuSelected;
    wsWindow menuWindow;
    int menuIsPresent;

    int IndexOfMainItems;
    guiItem mainItems[MAX_ITEMS];

    int IndexOfPlaybarItems;
    guiItem playbarItems[MAX_ITEMS];

    int IndexOfMenuItems;
    guiItem menuItems[MAX_ITEMS];
} guiItems;

extern guiItems guiApp;

guiItem *appFindItem(int event);
int appFindMessage(const char *name);
void appFreeStruct(void);
void btnModify(int event, float state);
void btnSet(int event, int set);

#endif /* MPLAYER_GUI_APP_H */
