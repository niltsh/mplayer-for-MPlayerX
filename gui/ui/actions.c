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
#include "ui.h"
#include "gui/app/app.h"
#include "gui/app/cfg.h"
#include "gui/app/gui.h"
#include "gui/dialog/dialog.h"
#include "gui/interface.h"
#include "gui/skin/skin.h"
#include "gui/util/list.h"
#include "gui/util/mem.h"
#include "gui/util/string.h"
#include "gui/wm/ws.h"
#include "gui/wm/wsxdnd.h"

#include "access_mpcontext.h"
#include "config.h"
#include "help_mp.h"
#include "input/input.h"
#include "libmpcodecs/vd.h"
#include "libmpdemux/demuxer.h"
#include "libvo/video_out.h"
#include "libvo/wskeys.h"
#include "libvo/x11_common.h"
#include "m_property.h"
#include "mixer.h"
#include "mp_core.h"
#include "mp_msg.h"
#include "mpcommon.h"
#include "mplayer.h"
#include "osdep/timer.h"
#include "stream/stream.h"
#include "sub/sub.h"

#define GUI_REDRAW_WAIT 375

static unsigned int last_redraw_time;

static void MediumPrepare(int type)
{
    switch (type) {
    case STREAMTYPE_DVD:
        listMgr(PLAYLIST_DELETE, 0);
        break;

    case STREAMTYPE_CDDA:
    case STREAMTYPE_VCD:
        listMgr(PLAYLIST_DELETE, 0);
    case STREAMTYPE_FILE:
    case STREAMTYPE_STREAM:
    case STREAMTYPE_PLAYLIST:
        guiInfo.AudioStreams = 0;
        guiInfo.Subtitles    = 0;
        guiInfo.Chapters     = 0;
        guiInfo.Angles       = 0;
        break;
    }
}

void uiEvent(int ev, float param)
{
    int iparam     = (int)param, osd;
    mixer_t *mixer = mpctx_get_mixer(guiInfo.mpcontext);

    switch (ev) {
/* user events */
    case evExit:
        mplayer(MPLAYER_EXIT_GUI, EXIT_QUIT, 0);
        break;

    case evLoadURL:
        gtkShow(evLoadURL, NULL);
        break;

    case ivSetAudio:

        if (!mpctx_get_demuxer(guiInfo.mpcontext) || audio_id == iparam)
            break;

        mp_property_do("switch_audio", M_PROPERTY_SET, &iparam, guiInfo.mpcontext);
        break;

    case ivSetVideo:

        if (!mpctx_get_demuxer(guiInfo.mpcontext) || video_id == iparam)
            break;

        mp_property_do("switch_video", M_PROPERTY_SET, &iparam, guiInfo.mpcontext);
        break;

    case ivSetSubtitle:
        mp_property_do("sub", M_PROPERTY_SET, &iparam, guiInfo.mpcontext);
        break;

#ifdef CONFIG_CDDA
    case ivSetCDTrack:
        guiInfo.Track = iparam;

    case evPlayCD:
        guiInfo.StreamType = STREAMTYPE_CDDA;
        goto play;

#endif
#ifdef CONFIG_VCD
    case ivSetVCDTrack:
        guiInfo.Track = iparam;

    case evPlayVCD:
        guiInfo.StreamType = STREAMTYPE_VCD;
        goto play;

#endif
#ifdef CONFIG_DVDREAD
    case ivSetDVDSubtitle:
        dvdsub_id = iparam;
        uiEvent(ivPlayDVD, 0);
        break;

    case ivSetDVDAudio:
        audio_id = iparam;
        uiEvent(ivPlayDVD, 0);
        break;

    case ivSetDVDChapter:
        guiInfo.Chapter = iparam;
        uiEvent(ivPlayDVD, 0);
        break;

    case ivSetDVDTitle:
        guiInfo.Track   = iparam;
        guiInfo.Chapter = 1;
        guiInfo.Angle   = 1;
        uiEvent(ivPlayDVD, 0);
        break;

    case evPlayDVD:
        guiInfo.Chapter = 1;
        guiInfo.Angle   = 1;

    case ivPlayDVD:
        guiInfo.StreamType = STREAMTYPE_DVD;
        goto play;

#endif
    case evPlay:
    case evPlaySwitchToPause:
play:

        if ((ev == evPlaySwitchToPause) && (guiInfo.Playing == GUI_PAUSE))
            goto NoPause;

        MediumPrepare(guiInfo.StreamType);

        switch (guiInfo.StreamType) {
        case STREAMTYPE_FILE:
        case STREAMTYPE_STREAM:
        case STREAMTYPE_PLAYLIST:

            if (!guiInfo.Track)
                guiInfo.Track = 1;

            guiInfo.NewPlay = GUI_FILE_NEW;
            break;

        case STREAMTYPE_CDDA:

            if (guiInfo.Playing != GUI_PAUSE) {
                if (!guiInfo.Track)
                    guiInfo.Track = 1;

                guiInfo.NewPlay = GUI_FILE_SAME;
            }

            break;

        case STREAMTYPE_VCD:

            if (guiInfo.Playing != GUI_PAUSE) {
                if (!guiInfo.Track)
                    guiInfo.Track = 2;

                guiInfo.NewPlay = GUI_FILE_SAME;
            }

            break;

        case STREAMTYPE_DVD:

            if (guiInfo.Playing != GUI_PAUSE) {
                if (!guiInfo.Track)
                    guiInfo.Track = 1;

                guiInfo.NewPlay = GUI_FILE_SAME;
            }

            break;
        }

        uiPlay();
        break;

    case evPause:
    case evPauseSwitchToPlay:
NoPause:
        uiPause();
        break;

    case evStop:
        guiInfo.Playing = GUI_STOP;
        uiState();
        break;

    case evLoadPlay:
        uiLoadPlay = True;

//      guiInfo.StreamType=STREAMTYPE_FILE;
    case evLoad:
        gtkShow(evLoad, NULL);
        break;

    case evLoadSubtitle:
        gtkShow(evLoadSubtitle, NULL);
        break;

    case evDropSubtitle:
        nfree(guiInfo.SubtitleFilename);
        mplayerLoadSubtitle(NULL);
        break;

    case evLoadAudioFile:
        gtkShow(evLoadAudioFile, NULL);
        break;

    case evPrev:
        uiPrev();
        break;

    case evNext:
        uiNext();
        break;

    case evPlaylist:
        gtkShow(evPlaylist, NULL);
        break;

    case evSkinBrowser:
        gtkShow(evSkinBrowser, skinName);
        break;

    case evAbout:
        gtkShow(evAbout, NULL);
        break;

    case evPreferences:
        gtkShow(evPreferences, NULL);
        break;

    case evEqualizer:
        gtkShow(evEqualizer, NULL);
        break;

    case evForward10min:
        uiRelSeek(600);
        break;

    case evBackward10min:
        uiRelSeek(-600);
        break;

    case evForward1min:
        uiRelSeek(60);
        break;

    case evBackward1min:
        uiRelSeek(-60);
        break;

    case evForward10sec:
        uiRelSeek(10);
        break;

    case evBackward10sec:
        uiRelSeek(-10);
        break;

    case evSetMoviePosition:
        uiAbsSeek(param);
        break;

    case evIncVolume:
        vo_x11_putkey(wsGrayMul);
        break;

    case evDecVolume:
        vo_x11_putkey(wsGrayDiv);
        break;

    case evMute:
        mixer_mute(mixer);
        break;

    case evSetVolume:
        guiInfo.Volume = param;
        {
            float l = guiInfo.Volume * ((100.0 - guiInfo.Balance) / 50.0);
            float r = guiInfo.Volume * ((guiInfo.Balance) / 50.0);
            mixer_setvolume(mixer, FFMIN(l, guiInfo.Volume), FFMIN(r, guiInfo.Volume));
        }

        if (osd_level) {
            osd_visible = (GetTimerMS() + 1000) | 1;
            vo_osd_progbar_type  = OSD_VOLUME;
            vo_osd_progbar_value = ((guiInfo.Volume) * 256.0) / 100.0;
            vo_osd_changed(OSDTYPE_PROGBAR);
        }

        break;

    case evSetBalance:
        guiInfo.Balance = param;
        mixer_setbalance(mixer, (guiInfo.Balance - 50.0) / 50.0);     // transform 0..100 to -1..1
        osd       = osd_level;
        osd_level = 0;
        uiEvent(evSetVolume, guiInfo.Volume);
        osd_level = osd;

        if (osd_level) {
            osd_visible = (GetTimerMS() + 1000) | 1;
            vo_osd_progbar_type  = OSD_BALANCE;
            vo_osd_progbar_value = ((guiInfo.Balance) * 256.0) / 100.0;
            vo_osd_changed(OSDTYPE_PROGBAR);
        }

        break;

    case evMenu:
        /*if (guiApp.menuIsPresent)   NOTE TO MYSELF: Uncomment only after mouse
         * {                                            pointer and cursor keys work
         * gtkShow( ivHidePopUpMenu,NULL );            with this menu from skin as
         * uiMenuShow( 0,0 );                          they do with normal menus.
         * }
         * else*/gtkShow(ivShowPopUpMenu, NULL);
        break;

    case evIconify:

        switch (iparam) {
        case 0:
            wsWindowIconify(&guiApp.mainWindow);
            break;

        case 1:
            wsWindowIconify(&guiApp.videoWindow);
            break;
        }

        break;

    case evHalfSize:

        if (guiInfo.VideoWindow && guiInfo.Playing) {
            if (guiApp.videoWindow.isFullScreen) {
                uiFullScreen();
            }

            wsWindowResize(&guiApp.videoWindow, guiInfo.VideoWidth / 2, guiInfo.VideoHeight / 2);
            btnSet(evFullScreen, btnReleased);
        }

        break;

    case evDoubleSize:

        if (guiInfo.VideoWindow && guiInfo.Playing) {
            if (guiApp.videoWindow.isFullScreen) {
                uiFullScreen();
            }

            wsWindowResize(&guiApp.videoWindow, guiInfo.VideoWidth * 2, guiInfo.VideoHeight * 2);
            wsWindowMoveWithin(&guiApp.videoWindow, False, guiApp.video.x, guiApp.video.y);
            btnSet(evFullScreen, btnReleased);
        }

        break;

    case evNormalSize:

        if (guiInfo.VideoWindow && guiInfo.Playing) {
            if (guiApp.videoWindow.isFullScreen) {
                uiFullScreen();
            }

            wsWindowResize(&guiApp.videoWindow, guiInfo.VideoWidth, guiInfo.VideoHeight);
            btnSet(evFullScreen, btnReleased);
            break;
        } else if (!guiApp.videoWindow.isFullScreen)
            break;

    case evFullScreen:

        if (guiInfo.VideoWindow && (guiInfo.Playing || !iparam)) {
            uiFullScreen();

            if (!guiApp.videoWindow.isFullScreen)
                wsWindowResize(&guiApp.videoWindow, iparam ? guiInfo.VideoWidth : guiApp.video.width, iparam ? guiInfo.VideoHeight : guiApp.video.height);
        }

        if (guiApp.videoWindow.isFullScreen)
            btnSet(evFullScreen, btnPressed);
        else
            btnSet(evFullScreen, btnReleased);

        break;

    case evSetAspect:

        switch (iparam) {
        case 2:
            movie_aspect = 16.0f / 9.0f;
            break;

        case 3:
            movie_aspect = 4.0f / 3.0f;
            break;

        case 4:
            movie_aspect = 2.35;
            break;

        case 1:
        default:
            movie_aspect = -1;
        }

        if (guiInfo.StreamType == STREAMTYPE_VCD)
            uiEvent(evPlayVCD, 0);
        else if (guiInfo.StreamType == STREAMTYPE_DVD)
            uiEvent(ivPlayDVD, 0);
        else
            guiInfo.NewPlay = GUI_FILE_NEW;

        break;

/* timer events */
    case ivRedraw:
    {
        unsigned int now = GetTimerMS();

        if ((now > last_redraw_time) &&
            (now < last_redraw_time + GUI_REDRAW_WAIT) &&
            !uiPlaybarFade && (iparam == 0))
            break;

        last_redraw_time = now;
    }
        uiMainRender = True;
        wsWindowRedraw(&guiApp.mainWindow);
        wsWindowRedraw(&guiApp.playbarWindow);
        break;

/* system events */
    case evNone:
        mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[actions] uiEvent: evNone\n");
        break;

    default:
        mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[actions] uiEvent: unknown event %d, param %.2f\n", ev, param);
        break;
    }
}

/**
 * @brief Switch video window fullscreen mode.
 *
 *        Switch normal video to fullscreen and fullscreen video to normal.
 */
void uiFullScreen(void)
{
    if (!guiInfo.VideoWindow)
        return;

    wsWindowFullscreen(&guiApp.videoWindow);

    vo_fs = guiApp.videoWindow.isFullScreen;

    wsWindowLayer(wsDisplay, guiApp.mainWindow.WindowID, guiApp.videoWindow.isFullScreen);

    if (guiApp.menuIsPresent)
        wsWindowLayer(wsDisplay, guiApp.menuWindow.WindowID, guiApp.videoWindow.isFullScreen);
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

    mainVisible = False;

    if (skinRead(name) != 0) {
        if (skinRead(skinName) != 0) {
            mainVisible = True;
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

        wsWindowResize(&guiApp.menuWindow, guiApp.menu.width, guiApp.menu.height);
        wsImageResize(&guiApp.menuWindow, guiApp.menu.width, guiApp.menu.height);
        wsWindowShape(&guiApp.menuWindow, guiApp.menu.Mask.Image);
        wsWindowVisibility(&guiApp.menuWindow, wsHideWindow);
    } else
        uiMenuInit();

    /* reload video window */

    if (guiApp.video.Bitmap.Image)
        wsImageResize(&guiApp.videoWindow, guiApp.video.Bitmap.Width, guiApp.video.Bitmap.Height);

    if (!guiApp.videoWindow.isFullScreen && !guiInfo.Playing) {
        wsWindowResize(&guiApp.videoWindow, guiApp.video.width, guiApp.video.height);
        wsWindowMove(&guiApp.videoWindow, False, guiApp.video.x, guiApp.video.y);
    }

    if (guiApp.video.Bitmap.Image)
        wsImageRender(&guiApp.videoWindow, guiApp.video.Bitmap.Image);

    if (!guiInfo.Playing)
        wsWindowRedraw(&guiApp.videoWindow);

    /* reload playbar */

    if (bprev)
        wsWindowDestroy(&guiApp.playbarWindow);

    uiPlaybarInit();

    /* reload main window */

    free(mainDrawBuffer);
    mainDrawBuffer = calloc(1, guiApp.main.Bitmap.ImageSize);

    if (!mainDrawBuffer) {
        gmp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_NEMDB);
        mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
    }

    wsWindowDestroy(&guiApp.mainWindow);

    wsWindowCreate(&guiApp.mainWindow, guiApp.main.x, guiApp.main.y, guiApp.main.width, guiApp.main.height, (guiApp.mainDecoration ? wsShowFrame : 0) | wsMinSize | wsMaxSize | wsHideWindow, wsShowMouseCursor | wsHandleMouseButton | wsHandleMouseMove, "MPlayer");
    wsImageCreate(&guiApp.mainWindow, guiApp.main.Bitmap.Width, guiApp.main.Bitmap.Height);
    wsWindowShape(&guiApp.mainWindow, guiApp.main.Mask.Image);
    wsWindowIcon(wsDisplay, guiApp.mainWindow.WindowID, &guiIcon);

    guiApp.mainWindow.DrawHandler  = uiMainDraw;
    guiApp.mainWindow.MouseHandler = uiMainMouse;
    guiApp.mainWindow.KeyHandler   = uiMainKey;
    guiApp.mainWindow.DNDHandler   = uiMainDND;

    wsXDNDMakeAwareness(&guiApp.mainWindow);

    wsWindowVisibility(&guiApp.mainWindow, wsShowWindow);
    mainVisible = True;

    btnModify(evSetVolume, guiInfo.Volume);
    btnModify(evSetBalance, guiInfo.Balance);
    btnModify(evSetMoviePosition, guiInfo.Position);
    btnSet(evFullScreen, (guiApp.videoWindow.isFullScreen ? btnPressed : btnReleased));

    wsWindowLayer(wsDisplay, guiApp.mainWindow.WindowID, guiApp.videoWindow.isFullScreen);
    wsWindowLayer(wsDisplay, guiApp.menuWindow.WindowID, guiApp.videoWindow.isFullScreen);
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
        uiUnsetMedia(False);
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
 * @brief Unset media information.
 *
 * @param totals whether to unset number of chapters and angles (#True) or
 *               just track, chapter and angle (#False) as well
 */
void uiUnsetMedia(int totals)
{
    guiInfo.VideoWidth    = 0;
    guiInfo.VideoHeight   = 0;
    guiInfo.AudioChannels = 0;
    guiInfo.RunningTime   = 0;

    if (totals) {
        guiInfo.Chapters = 0;
        guiInfo.Angles   = 0;
    } else {
        guiInfo.Track   = 0;
        guiInfo.Chapter = 0;
        guiInfo.Angle   = 0;
    }

    nfree(guiInfo.CodecName);
    nfree(guiInfo.AudioFilename);
    nfree(guiInfo.SubtitleFilename);
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
            guiInfo.PlaylistNext = False;
            guiInfo.Track = (int)listMgr(PLAYLIST_ITEM_GET_POS, curr);
            break;
        }

        return;
    }

    if (guiInfo.Playing == GUI_PLAY)
        uiEvent(evPlay, 0);
}

/**
 * @brief Switch to previous playback track.
 */
void uiPrev(void)
{
    int stop = False, unset = True;
    plItem *prev = NULL;

    if (guiInfo.Playing == GUI_PAUSE)
        return;

    switch (guiInfo.StreamType) {
    case STREAMTYPE_CDDA:

        if (--guiInfo.Track == 0) {
            guiInfo.Track = 1;
            stop = True;
        }

        break;

    case STREAMTYPE_VCD:

        if (--guiInfo.Track == 1) {
            guiInfo.Track = 2;
            stop = True;
        }

        break;

    case STREAMTYPE_DVD:

        if (--guiInfo.Chapter == 0) {
            guiInfo.Chapter = 1;

            if (--guiInfo.Track == 0) {
                guiInfo.Track = 1;
                stop = True;
            }
        } else
            unset = False;

        break;

    default:

        prev = listMgr(PLAYLIST_ITEM_GET_PREV, 0);

        if (prev) {
            uiSetFile(prev->path, prev->name, STREAMTYPE_FILE);
            guiInfo.PlaylistNext = !guiInfo.Playing;
            guiInfo.Track = (int)listMgr(PLAYLIST_ITEM_GET_POS, prev);
            break;
        }

        return;
    }

    if (stop)
        uiEvent(evStop, 0);

    if (guiInfo.Playing == GUI_PLAY)
        uiEvent(evPlay, 0);
    else if (!stop && !prev && unset)
        uiUnsetMedia(True);
}

/**
 * @brief Switch to next playback track.
 */
void uiNext(void)
{
    int stop = False, unset = True;
    plItem *next = NULL;

    if (guiInfo.Playing == GUI_PAUSE)
        return;

    switch (guiInfo.StreamType) {
    case STREAMTYPE_CDDA:
    case STREAMTYPE_VCD:

        if (++guiInfo.Track > guiInfo.Tracks) {
            guiInfo.Track = guiInfo.Tracks;
            stop = True;
        }

        break;

    case STREAMTYPE_DVD:

        if (guiInfo.Chapter++ >= guiInfo.Chapters) {
            guiInfo.Chapter = 1;

            if (++guiInfo.Track > guiInfo.Tracks) {
                guiInfo.Track   = guiInfo.Tracks;
                guiInfo.Chapter = guiInfo.Chapters;
                stop = True;
            }
        } else
            unset = False;

        break;

    default:

        next = listMgr(PLAYLIST_ITEM_GET_NEXT, 0);

        if (next) {
            uiSetFile(next->path, next->name, STREAMTYPE_FILE);
            guiInfo.PlaylistNext = !guiInfo.Playing;
            guiInfo.Track = (int)listMgr(PLAYLIST_ITEM_GET_POS, next);
            break;
        }

        return;
    }

    if (stop)
        uiEvent(evStop, 0);

    if (guiInfo.Playing == GUI_PLAY)
        uiEvent(evPlay, 0);
    else if (!stop && !next && unset)
        uiUnsetMedia(True);
}
