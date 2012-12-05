/*
 * MPlayer GUI for Win32
 * Copyright (C) 2003 Sascha Sommer <saschasommer@freenet.de>
 * Copyright (C) 2006 Erik Augustson <erik_27can@yahoo.com>
 * Copyright (C) 2006 Gianluigi Tiesi <sherpya@netfarm.it>
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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MPLAYER_GUI_SKINLOAD_H
#define MPLAYER_GUI_SKINLOAD_H

typedef struct
{
    char *name;                  /* image name */
    int size;                    /* image data size in bytes */
    int width;                   /* image width */
    int height;                  /* image height */
    char *data;                  /* pointer to image data */
} image;

typedef struct
{
    char c;
    int x;
    int y;
    int width;
    int height;
} char_t;

typedef struct
{
    char *name;
    char *id;
    image *image;
    unsigned int charcount;
    char_t **chars;
} font_t;

typedef struct
{
    int id;                         /* widget id */
    int type;                       /* widget type */
    int window;                     /* the window it belongs to */
    // ---
    int x, y;                       /* x and y position the button */
    int wx, wy;                     /* x and y postion of the widget */
    int width, height;              /* width and height of the button */
    int wwidth, wheight;            /* width and height of the widget */
    // ---
    // ---
    int msg, msg2;
    int pressed, tmp;
    int key, key2;
    int phases;
    float value;
    image *bitmap[2];               /* Associated image(s) in imagepool */
    // ---
    font_t *font;
    int length;
    int align;
    char *label;
    // ---
    int event;
} widget;

typedef struct
{
    char *name;
    int decoration;
    int type;
    widget *base;
    int backgroundcolor[3];
    void *priv;
} window;

typedef struct skin_t skin_t;
struct skin_t
{
    char *skindir;
    unsigned int widgetcount;       /* number of widgets */
    unsigned int lastusedid;        /* which widget id was used last */
    widget **widgets;               /* widget handle */
    unsigned int imagecount;        /* number of images */
    image **images;                 /* image handle */
    int desktopbpp;                 /* image format of those images */
    unsigned int fontcount;
    font_t **fonts;

    unsigned int windowcount;       /* number of windows */
    window **windows;
    void (*freeskin)(skin_t* skin);
    void (*addwidget)(skin_t *skin, window *win, const char *desc);
    void (*removewidget)(skin_t *skin, int id);
    char *(*geteventname)(int event);
    image *(*pngRead)(skin_t *skin, const char *fname);
};

skin_t *loadskin(char *skindir, int desktopbpp);

/* --- Widget types --- */

#define tyBase          1
#define tyButton        2
#define tyHpotmeter     3
#define tyVpotmeter     4
#define tyPotmeter      5
#define tyMenu          6
#define tySlabel        7
#define tyDlabel        8

/* --- Window types --- */

#define wiMain          1
#define wiVideo         2
#define wiMenu          3
#define wiPlaybar       4

/* --- User events --- */

enum
{
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

/* --- Internal events --- */

enum
{
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

#endif /* MPLAYER_GUI_SKINLOAD_H */
