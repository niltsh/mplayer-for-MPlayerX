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

/**
 * @file
 * @brief User interface actions
 */

#include <stdlib.h>
#include <string.h>

#include "actions.h"
#include "gmplayer.h"
#include "gui/app.h"
#include "gui/gui.h"
#include "gui/interface.h"
#include "gui/skin/skin.h"
#include "gui/util/list.h"
#include "gui/util/mem.h"
#include "gui/util/string.h"
#include "gui/wm/ws.h"
#include "gui/wm/wsxdnd.h"

#include "help_mp.h"
#include "input/input.h"
#include "libmpdemux/demuxer.h"
#include "libvo/video_out.h"
#include "mp_core.h"
#include "mp_msg.h"
#include "mplayer.h"

/**
 * @brief Switch video window fullscreen mode.
 *
 *        Switch normal video to fullscreen and fullscreen video to normal.
 */
void uiFullScreen(void)
{
    if (!guiInfo.VideoWindow)
        return;

    wsFullScreen(&guiApp.videoWindow);

    vo_fs = guiApp.videoWindow.isFullScreen;

    wsSetLayer(wsDisplay, guiApp.mainWindow.WindowID, guiApp.videoWindow.isFullScreen);

    if (guiApp.menuIsPresent)
        wsSetLayer(wsDisplay, guiApp.menuWindow.WindowID, guiApp.videoWindow.isFullScreen);

    if (guiInfo.Playing)
        wsSetBackgroundRGB(&guiApp.videoWindow, 0, 0, 0);
    else
        wsSetBackgroundRGB(&guiApp.videoWindow, guiApp.video.R, guiApp.video.G, guiApp.video.B);
}

/**
 * @brief Switch to play mode.
 */
void uiPlay(void)
{
    if (guiInfo.Playing == GUI_PLAY)
        return;

    if (guiInfo.StreamType != STREAMTYPE_CDDA &&
        guiInfo.StreamType != STREAMTYPE_VCD &&
        guiInfo.StreamType != STREAMTYPE_DVD &&
        (!guiInfo.Filename || (guiInfo.Filename[0] == 0)))
        return;

    if (guiInfo.Playing == GUI_PAUSE) {
        uiPause();
        return;
    }

    gui(GUI_SET_STATE, (void *)GUI_PLAY);
    uiVideoRender = 0;
    wsSetBackgroundRGB(&guiApp.videoWindow, 0, 0, 0);
    wsClearWindow(&guiApp.videoWindow);
}

/**
 * @brief Switch to pause mode.
 */
void uiPause(void)
{
    if (!guiInfo.Playing)
        return;

    if (guiInfo.Playing == GUI_PLAY) {
        mp_cmd_t *cmd = calloc(1, sizeof(*cmd));

        if (cmd) {
            cmd->id   = MP_CMD_PAUSE;
            cmd->name = strdup("pause");
            mp_input_queue_cmd(cmd);
        }
    } else
        guiInfo.Playing = GUI_PLAY;
}

/**
 * @brief Adjust GUI items to reflect current state (i.e. current playing mode).
 */
void uiState(void)
{
    if (guiInfo.Playing == GUI_STOP || guiInfo.Playing == GUI_PAUSE) {
        btnSet(evPlaySwitchToPause, btnReleased);
        btnSet(evPauseSwitchToPlay, btnDisabled);
    } else {
        btnSet(evPauseSwitchToPlay, btnReleased);
        btnSet(evPlaySwitchToPause, btnDisabled);
    }
}

/**
 * @brief Seek new playback position.
 *
 *        The new position is a relative one.
 *
 * @param sec seconds to seek (either forward (> 0) or backward (< 0))
 */
void uiRelSeek(float sec)
{
    rel_seek_secs = sec;
    abs_seek_pos  = 0;
}

/**
 * @brief Seek new playback position.
 *
 *        The new position is an absolute one.
 *
 * @param percent percentage of playback time to position to
 */
void uiAbsSeek(float percent)
{
    rel_seek_secs = percent / 100.0;
    abs_seek_pos  = SEEK_ABSOLUTE | SEEK_FACTOR;
}

/**
 * @brief Change to a different skin.
 *
 * @param name name of the skin to change to
 */
void uiChangeSkin(char *name)
{
    int prev, bprev;

    prev  = guiApp.menuIsPresent;
    bprev = guiApp.playbarIsPresent;

    mainVisible = 0;

    if (skinRead(name) != 0) {
        if (skinRead(skinName) != 0) {
            mainVisible = 1;
            return;
        }
    }

    /* reload menu window */

    if (prev && guiApp.menuIsPresent) {
        free(menuDrawBuffer);
        menuDrawBuffer = calloc(1, guiApp.menu.Bitmap.ImageSize);

        if (!menuDrawBuffer) {
            gmp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_NEMDB);
            mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
        }

        wsResizeWindow(&guiApp.menuWindow, guiApp.menu.width, guiApp.menu.height);
        wsResizeImage(&guiApp.menuWindow, guiApp.menu.width, guiApp.menu.height);
        wsSetShape(&guiApp.menuWindow, guiApp.menu.Mask.Image);
        wsVisibleWindow(&guiApp.menuWindow, wsHideWindow);
    } else
        uiMenuInit();

    /* reload video window */

    if (guiApp.video.Bitmap.Image)
        wsResizeImage(&guiApp.videoWindow, guiApp.video.Bitmap.Width, guiApp.video.Bitmap.Height);

    if (!guiApp.videoWindow.isFullScreen && !guiInfo.Playing) {
        wsResizeWindow(&guiApp.videoWindow, guiApp.video.width, guiApp.video.height);
        wsMoveWindow(&guiApp.videoWindow, False, guiApp.video.x, guiApp.video.y);
    }

    if (guiApp.video.Bitmap.Image)
        wsConvert(&guiApp.videoWindow, guiApp.video.Bitmap.Image);

    if (!guiInfo.Playing) {
        uiVideoRender = 1;
        wsSetBackgroundRGB(&guiApp.videoWindow, guiApp.video.R, guiApp.video.G, guiApp.video.B);
        wsClearWindow(&guiApp.videoWindow);
        wsPostRedisplay(&guiApp.videoWindow);
    }

    /* reload playbar */

    if (bprev)
        wsDestroyWindow(&guiApp.playbarWindow);

    uiPlaybarInit();

    /* reload main window */

    free(mainDrawBuffer);
    mainDrawBuffer = calloc(1, guiApp.main.Bitmap.ImageSize);

    if (!mainDrawBuffer) {
        gmp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_NEMDB);
        mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
    }

    wsDestroyWindow(&guiApp.mainWindow);

    wsCreateWindow(&guiApp.mainWindow, guiApp.main.x, guiApp.main.y, guiApp.main.width, guiApp.main.height, wsNoBorder, wsShowMouseCursor | wsHandleMouseButton | wsHandleMouseMove, wsShowFrame | wsMaxSize | wsHideWindow, "MPlayer");
    wsCreateImage(&guiApp.mainWindow, guiApp.main.Bitmap.Width, guiApp.main.Bitmap.Height);
    wsSetShape(&guiApp.mainWindow, guiApp.main.Mask.Image);
    wsSetIcon(wsDisplay, guiApp.mainWindow.WindowID, &guiIcon);

    guiApp.mainWindow.ReDraw       = (void *)uiMainDraw;
    guiApp.mainWindow.MouseHandler = uiMainMouseHandle;
    guiApp.mainWindow.KeyHandler   = uiMainKeyHandle;
    guiApp.mainWindow.DandDHandler = uiDandDHandler;

    wsXDNDMakeAwareness(&guiApp.mainWindow);

    if (!guiApp.mainDecoration)
        wsWindowDecoration(&guiApp.mainWindow, 0);

    wsVisibleWindow(&guiApp.mainWindow, wsShowWindow);
    mainVisible = 1;

    btnModify(evSetVolume, guiInfo.Volume);
    btnModify(evSetBalance, guiInfo.Balance);
    btnModify(evSetMoviePosition, guiInfo.Position);
    btnSet(evFullScreen, (guiApp.videoWindow.isFullScreen ? btnPressed : btnReleased));

    wsSetLayer(wsDisplay, guiApp.mainWindow.WindowID, guiApp.videoWindow.isFullScreen);
    wsSetLayer(wsDisplay, guiApp.menuWindow.WindowID, guiApp.videoWindow.isFullScreen);
}

/**
 * @brief Set the file to be played.
 *
 * @param dir directory (optional, else NULL)
 * @param name filename
 * @param type stream type of the file
 *
 * @note All #guiInfo members associated with the file will be cleared.
 */
void uiSetFile(char *dir, char *name, int type)
{
    if (!dir)
        setdup(&guiInfo.Filename, name);
    else
        setddup(&guiInfo.Filename, dir, name);

    filename = guiInfo.Filename;

    if (type != SAME_STREAMTYPE) {
        guiInfo.StreamType = type;

        guiInfo.VideoWidth    = 0;
        guiInfo.VideoHeight   = 0;
        guiInfo.AudioChannels = 0;
        guiInfo.RunningTime   = 0;
        guiInfo.Track   = 0;
        guiInfo.Chapter = 0;
        guiInfo.Angle   = 0;

        nfree(guiInfo.CodecName);
        nfree(guiInfo.AudioFilename);
        nfree(guiInfo.SubtitleFilename);
    }
}

/**
 * @brief Unset the file being played.
 */
void uiUnsetFile(void)
{
    uiSetFile(NULL, NULL, STREAMTYPE_DUMMY);
}

/**
 * @brief Set file to be played to current playlist entry.
 */
void uiCurr(void)
{
    plItem *curr;

    if (guiInfo.Playing == GUI_PAUSE)
        return;

    switch (guiInfo.StreamType) {
    case STREAMTYPE_CDDA:
    case STREAMTYPE_VCD:
    case STREAMTYPE_DVD:

        break;

    default:

        curr = listMgr(PLAYLIST_ITEM_GET_CURR, 0);

        if (curr) {
            uiSetFile(curr->path, curr->name, STREAMTYPE_FILE);
            guiInfo.PlaylistNext = 0;
            guiInfo.Track = (int)listMgr(PLAYLIST_ITEM_GET_POS, curr);
            break;
        }

        return;
    }

    if (guiInfo.Playing == GUI_PLAY)
        uiEventHandling(evPlay, 0);
}

/**
 * @brief Switch to previous playback track.
 */
void uiPrev(void)
{
    int stop = 0;
    plItem *prev;

    if (guiInfo.Playing == GUI_PAUSE)
        return;

    switch (guiInfo.StreamType) {
    case STREAMTYPE_CDDA:

        if (--guiInfo.Track == 0) {
            guiInfo.Track = 1;
            stop = 1;
        }

        break;

    case STREAMTYPE_VCD:

        if (--guiInfo.Track == 1) {
            guiInfo.Track = 2;
            stop = 1;
        }

        break;

    case STREAMTYPE_DVD:

        if (--guiInfo.Chapter == 0) {
            guiInfo.Chapter = 1;

            if (--guiInfo.Track == 0) {
                guiInfo.Track = 1;
                stop = 1;
            }
        }

        break;

    default:

        prev = listMgr(PLAYLIST_ITEM_GET_PREV, 0);

        if (prev) {
            uiSetFile(prev->path, prev->name, STREAMTYPE_FILE);
            guiInfo.PlaylistNext = (guiInfo.Playing ? 0 : 1);
            guiInfo.Track = (int)listMgr(PLAYLIST_ITEM_GET_POS, prev);
            break;
        }

        return;
    }

    if (stop)
        uiEventHandling(evStop, 0);

    if (guiInfo.Playing == GUI_PLAY)
        uiEventHandling(evPlay, 0);
}

/**
 * @brief Switch to next playback track.
 */
void uiNext(void)
{
    int stop = 0;
    plItem *next;

    if (guiInfo.Playing == GUI_PAUSE)
        return;

    switch (guiInfo.StreamType) {
    case STREAMTYPE_CDDA:
    case STREAMTYPE_VCD:

        if (++guiInfo.Track > guiInfo.Tracks) {
            guiInfo.Track = guiInfo.Tracks;
            stop = 1;
        }

        break;

    case STREAMTYPE_DVD:

        if (guiInfo.Chapter++ == guiInfo.Chapters) {
            guiInfo.Chapter = 1;

            if (++guiInfo.Track > guiInfo.Tracks) {
                guiInfo.Track = guiInfo.Tracks;
                stop = 1;
            }
        }

        break;

    default:

        next = listMgr(PLAYLIST_ITEM_GET_NEXT, 0);

        if (next) {
            uiSetFile(next->path, next->name, STREAMTYPE_FILE);
            guiInfo.PlaylistNext = (guiInfo.Playing ? 0 : 1);
            guiInfo.Track = (int)listMgr(PLAYLIST_ITEM_GET_POS, next);
            break;
        }

        return;
    }

    if (stop)
        uiEventHandling(evStop, 0);

    if (guiInfo.Playing == GUI_PLAY)
        uiEventHandling(evPlay, 0);
}
