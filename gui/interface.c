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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interface.h"
#include "app/app.h"
#include "app/gui.h"
#include "dialog/dialog.h"
#include "skin/skin.h"
#include "ui/actions.h"
#include "ui/ui.h"
#include "util/list.h"
#include "util/mem.h"
#include "util/string.h"
#include "wm/ws.h"
#include "wm/wsxdnd.h"

#include "access_mpcontext.h"
#include "codec-cfg.h"
#include "config.h"
#include "help_mp.h"
#include "mixer.h"
#include "mp_msg.h"
#include "mpcommon.h"
#include "mplayer.h"
#include "path.h"
#include "input/input.h"
#include "libaf/equalizer.h"
#include "libavutil/common.h"
#include "libmpcodecs/dec_audio.h"
#include "libmpcodecs/dec_video.h"
#include "libmpcodecs/vd.h"
#include "libmpcodecs/vf.h"
#include "libvo/video_out.h"
#include "libvo/x11_common.h"
#ifdef CONFIG_DVDREAD
#include "stream/stream_dvd.h"
#endif
#include "sub/font_load.h"
#include "sub/sub.h"
#include "sub/subreader.h"

/**
 * @brief Initialize interface data.
 */
guiInterface_t guiInfo = {
    .StreamType   = STREAMTYPE_DUMMY,
    .Balance      = 50.0f,
    .PlaylistNext = True
};

static int initialized;

/* MPlayer -> GUI */

/**
 * @brief Initialize and start the GUI.
 */
void guiInit(void)
{
    int ret;
    plItem *playlist;

    mp_msg(MSGT_GPLAYER, MSGL_V, "GUI init.\n");

    /* check options */

    if (!cdrom_device)
        cdrom_device = strdup(DEFAULT_CDROM_DEVICE);
    if (!dvd_device)
        dvd_device = strdup(DEFAULT_DVD_DEVICE);
#ifdef CONFIG_DXR3
    if (!gtkDXR3Device)
        gtkDXR3Device = strdup("/dev/em8300-0");
#endif

    if (stream_cache_size > 0) {
        gtkCacheOn   = True;
        gtkCacheSize = stream_cache_size;
    } else if (stream_cache_size == 0)
        gtkCacheOn = False;

    if (autosync && (autosync != gtkAutoSync)) {
        gtkAutoSyncOn = True;
        gtkAutoSync   = autosync;
    }

    gtkASS.enabled       = ass_enabled;
    gtkASS.use_margins   = ass_use_margins;
    gtkASS.top_margin    = ass_top_margin;
    gtkASS.bottom_margin = ass_bottom_margin;

    /* initialize graphical user interfaces */

    wsInit(mDisplay);
    gtkInit(mDisplayName);

    /* load skin */

    skinDirInHome = get_path("skins");
    skinDirInData = MPLAYER_DATADIR "/skins";

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[interface] skin directory #1: %s\n", skinDirInHome);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[interface] skin directory #2: %s\n", skinDirInData);

    if (!skinName)
        skinName = strdup("default");

    ret = skinRead(skinName);

    if (ret == -1 && strcmp(skinName, "default") != 0) {
        mp_msg(MSGT_GPLAYER, MSGL_WARN, MSGTR_SKIN_SKINCFG_SelectedSkinNotFound, skinName);

        skinName = strdup("default");
        ret      = skinRead(skinName);
    }

    switch (ret) {
    case -1:
        gmp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_SKIN_SKINCFG_SkinNotFound, skinName);
        mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);

    case -2:
        gmp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_SKIN_SKINCFG_SkinCfgError, skinName);
        mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
    }

    /* initialize windows */

    if (gui_save_pos) {
        if (gui_main_pos_x != -3)
            guiApp.main.x = gui_main_pos_x;
        if (gui_main_pos_y != -3)
            guiApp.main.y = gui_main_pos_y;
        if (gui_video_pos_x != -3)
            guiApp.video.x = gui_video_pos_x;
        if (gui_video_pos_y != -3)
            guiApp.video.y = gui_video_pos_y;
    }

    if (WinID > 0) {
        guiApp.videoWindow.Parent = WinID;
        guiApp.video.x = 0;
        guiApp.video.y = 0;
    }

    if (guiWinID >= 0)
        guiApp.mainWindow.Parent = guiWinID;

    uiMainInit();      // main window must be first!
    uiVideoInit();     // video window must be second!
    uiPlaybarInit();
    uiMenuInit();

    WinID = guiApp.videoWindow.WindowID;

    btnModify(evSetVolume, guiInfo.Volume);
    btnModify(evSetBalance, guiInfo.Balance);
    btnModify(evSetMoviePosition, guiInfo.Position);

    wsWindowVisibility(&guiApp.mainWindow, wsShowWindow);

    if (gtkShowVideoWindow) {
        wsWindowVisibility(&guiApp.videoWindow, wsShowWindow);

        guiInfo.VideoWindow = True;

        if (gtkLoadFullscreen)
            uiFullScreen();
    } else
        wsWindowBackground(&guiApp.videoWindow, 0, 0, 0);

    if (gtkLoadFullscreen)
        btnSet(evFullScreen, btnPressed);

    guiInfo.Playing = GUI_STOP;

    playlist = listMgr(PLAYLIST_ITEM_GET_CURR, 0);

    if (playlist && !filename) {
        uiSetFile(playlist->path, playlist->name, STREAMTYPE_FILE);
        guiInfo.Tracks = (int)listMgr(PLAYLIST_ITEM_GET_POS, 0);
        guiInfo.Track  = 1;
        filename       = NULL; // don't start playing
    }

    if (subdata)
        setdup(&guiInfo.SubtitleFilename, subdata->filename);

    mplayerLoadFont();

    initialized = True;
}

/**
 * @brief Stop and finalize the GUI.
 */
void guiDone(void)
{
    if (initialized) {
        if (gui_save_pos) {
            gui_main_pos_x  = guiApp.mainWindow.X;
            gui_main_pos_y  = guiApp.mainWindow.Y;
            gui_video_pos_x = guiApp.videoWindow.X;
            gui_video_pos_y = guiApp.videoWindow.Y;
        }

        ass_enabled       = gtkASS.enabled;
        ass_use_margins   = gtkASS.use_margins;
        ass_top_margin    = gtkASS.top_margin;
        ass_bottom_margin = gtkASS.bottom_margin;

        cfg_write();

        if (guiApp.menuIsPresent)
            uiMenuDone();
        if (guiApp.playbarIsPresent)
            uiPlaybarDone();

        uiVideoDone();
        uiMainDone();

        wsDone();
    }

    uiUnsetFile();
    listMgr(PLAYLIST_DELETE, 0);
    listMgr(URLLIST_DELETE, 0);
    appFreeStruct();
    free(guiIcon.collection);

    if (gui_conf) {
        m_config_free(gui_conf);
        gui_conf = NULL;
    }

    mp_msg(MSGT_GPLAYER, MSGL_V, "GUI done.\n");
}

static void add_vf(char *str)
{
    void *p;

    if (vf_settings) {
        int i = 0;

        while (vf_settings[i].name) {
            if (!gstrcmp(vf_settings[i++].name, str)) {
                i = -1;
                break;
            }
        }

        if (i != -1) {
            p = realloc(vf_settings, (i + 2) * sizeof(m_obj_settings_t));

            if (!p)
                return;

            vf_settings = p;
            vf_settings[i].name     = strdup(str);
            vf_settings[i].attribs  = NULL;
            vf_settings[i + 1].name = NULL;
        }
    } else {
        vf_settings = malloc(2 * sizeof(m_obj_settings_t));
        vf_settings[0].name    = strdup(str);
        vf_settings[0].attribs = NULL;
        vf_settings[1].name    = NULL;
    }

    mp_msg(MSGT_GPLAYER, MSGL_INFO, MSGTR_AddingVideoFilter, str);
}

/**
 * @brief Issue a command to the GUI.
 *
 * @note The GUI is controlled by giving it commands.
 *
 * @param what command to be performed
 * @param data pointer to data needed for the command
 *
 * @return #True (ok) or #False (error)
 */
int gui(int what, void *data)
{
    stream_t *stream;
#ifdef CONFIG_DVDREAD
    dvd_priv_t *dvd;
#endif
    int msg, state;
    mixer_t *mixer = NULL;
    plItem *next   = NULL;

    switch (what) {
    case GUI_SET_CONTEXT:

        guiInfo.mpcontext = data;
        break;

    case GUI_SET_STATE:

        switch ((int)data) {
        case GUI_STOP:
        case GUI_PLAY:
// if ( !gtkShowVideoWindow ) wsWindowVisibility( &guiApp.videoWindow,wsHideWindow );
        case GUI_PAUSE:
            guiInfo.Playing = (int)data;
            break;
        }

        uiState();
        break;

    case GUI_HANDLE_EVENTS:

        if (!guiInfo.Playing || !guiInfo.VideoWindow)
            wsEvents();

        wsMouseAutohide();
        gtkEventHandling();

        break;

    case GUI_RUN_COMMAND:

        mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[interface] GUI_RUN_COMMAND: %d\n", (int)data);

        switch ((int)data) {
        case MP_CMD_VO_FULLSCREEN:
            uiEvent(evFullScreen, True);
            break;

        case MP_CMD_PLAY_TREE_STEP:
            uiEvent(evNext, 0);
            break;

        case -MP_CMD_PLAY_TREE_STEP:
            uiEvent(evPrev, 0);
            break;

        case MP_CMD_STOP:
            uiEvent(evStop, 0);
            break;

        case MP_CMD_QUIT:
            uiEvent(evExit, 0);
            break;
        }

        break;

    case GUI_RUN_MESSAGE:

        mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[interface] GUI_RUN_MESSAGE: %s\n", (const char *)data);
        msg = appFindMessage((const char *)data);

        if ((msg == evMenu) || appFindItem(msg))
            uiEvent(msg, 0);

        break;

    case GUI_PREPARE:

        wsMouseVisibility(&guiApp.videoWindow, wsHideMouseCursor);

        if (guiInfo.NewPlay == GUI_FILE_NEW) {
            audio_id  = -1;
            video_id  = -1;
            dvdsub_id = -1;
            vobsub_id = -1;

            stream_cache_size = -1;
            autosync  = 0;
            force_fps = 0;
        }

        switch (guiInfo.StreamType) {
        case STREAMTYPE_FILE:
        case STREAMTYPE_STREAM:
            filename = guiInfo.Filename;
            break;

        case STREAMTYPE_CDDA:
        {
            char tmp[512];

            sprintf(tmp, "cdda://%d", guiInfo.Track);
            uiSetFile(NULL, tmp, SAME_STREAMTYPE);
        }
        break;

        case STREAMTYPE_VCD:
        {
            char tmp[512];

            sprintf(tmp, "vcd://%d", guiInfo.Track);
            uiSetFile(NULL, tmp, SAME_STREAMTYPE);
        }
        break;

        case STREAMTYPE_DVD:
        {
            char tmp[512];

            sprintf(tmp, "dvd://%d", guiInfo.Track);
            uiSetFile(NULL, tmp, SAME_STREAMTYPE);
        }
#ifdef CONFIG_DVDREAD
            dvd_chapter = guiInfo.Chapter;
            dvd_angle   = guiInfo.Angle;
#endif
            break;
        }

        /* video opts */

        if (!video_driver_list) {
            int i = 0;

            while (video_out_drivers[i++]) {
                if (video_out_drivers[i - 1]->control(VOCTRL_GUISUPPORT, NULL) == VO_TRUE) {
                    listSet(&video_driver_list, (char *)video_out_drivers[i - 1]->info->short_name);
                    break;
                }
            }
        }

        if (!video_driver_list && !video_driver_list[0]) {
            gmp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_IDFGCVD);
            mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
        }

        {
            int i = 0;

            guiInfo.VideoWindow = True;

            while (video_out_drivers[i++]) {
                if (video_out_drivers[i - 1]->control(VOCTRL_GUISUPPORT, NULL) == VO_TRUE) {
                    if ((video_driver_list && !gstrcmp(video_driver_list[0], (char *)video_out_drivers[i - 1]->info->short_name)) && (video_out_drivers[i - 1]->control(VOCTRL_GUI_NOWINDOW, NULL) == VO_TRUE)) {
                        guiInfo.VideoWindow = False;
                        break;
                    }
                }
            }
        }

        if (video_driver_list && !gstrcmp(video_driver_list[0], "dxr3"))
            if (guiInfo.StreamType != STREAMTYPE_DVD && guiInfo.StreamType != STREAMTYPE_VCD)
                if (gtkVfLAVC)
                    add_vf("lavc");

        if (gtkVfPP)
            add_vf("pp");

        /* audio opts */

// if ( ao_plugin_cfg.plugin_list ) { free( ao_plugin_cfg.plugin_list ); ao_plugin_cfg.plugin_list=NULL; }
        if (gtkAONorm)
            listRepl(&af_cfg.list, "volnorm", "volnorm");

        if (gtkEnableAudioEqualizer)
            listRepl(&af_cfg.list, "equalizer", "equalizer");

        if (gtkAOExtraStereo) {
            char *name;

            name = malloc(12 + 20 + 1);
            snprintf(name, 12 + 20, "extrastereo=%f", gtkAOExtraStereoMul);
            name[12 + 20] = 0;
            listRepl(&af_cfg.list, "extrastereo", name);
            free(name);
        }

        if (audio_driver_list && !gstrncmp(audio_driver_list[0], "oss", 3)) {
            mixer_device  = gtkAOOSSMixer;
            mixer_channel = gtkAOOSSMixerChannel;

            if (gtkAOOSSDevice) {
                char *tmp;

                tmp = calloc(1, strlen(gtkAOOSSDevice) + 7);
                sprintf(tmp, "oss:%s", gtkAOOSSDevice);
                listSet(&audio_driver_list, tmp);
                free(tmp);
            }
        }

        if (audio_driver_list && !gstrncmp(audio_driver_list[0], "alsa", 4)) {
            mixer_device  = gtkAOALSAMixer;
            mixer_channel = gtkAOALSAMixerChannel;

            if (gtkAOALSADevice) {
                char *tmp;

                tmp = calloc(1, strlen(gtkAOALSADevice) + 14);
                sprintf(tmp, "alsa:device=%s", gtkAOALSADevice);
                listSet(&audio_driver_list, tmp);
                free(tmp);
            }
        }

        if (audio_driver_list && !gstrncmp(audio_driver_list[0], "sdl", 3)) {
            if (gtkAOSDLDriver) {
                char *tmp;

                tmp = calloc(1, strlen(gtkAOSDLDriver) + 10);
                sprintf(tmp, "sdl:%s", gtkAOSDLDriver);
                listSet(&audio_driver_list, tmp);
                free(tmp);
            }
        }

        if (audio_driver_list && !gstrncmp(audio_driver_list[0], "esd", 3)) {
            if (gtkAOESDDevice) {
                char *tmp;

                tmp = calloc(1, strlen(gtkAOESDDevice) + 10);
                sprintf(tmp, "esd:%s", gtkAOESDDevice);
                listSet(&audio_driver_list, tmp);
                free(tmp);
            }
        }

        /* subtitle */

// subdata->filename=gstrdup( guiInfo.SubtitleFilename );
        stream_dump_type = 0;

        if (gtkSubDumpMPSub)
            stream_dump_type = 4;

        if (gtkSubDumpSrt)
            stream_dump_type = 6;

        gtkSubDumpMPSub = gtkSubDumpSrt = False;
        mplayerLoadFont();

        /* misc */

        if (gtkCacheOn)
            stream_cache_size = gtkCacheSize;

        if (gtkAutoSyncOn)
            autosync = gtkAutoSync;

        if (guiInfo.AudioFilename)
            audio_stream = gstrdup(guiInfo.AudioFilename);
        else if (guiInfo.NewPlay == GUI_FILE_NEW)
            nfree(audio_stream);

// audio_stream = NULL;

        guiInfo.NewPlay = 0;

        ass_enabled       = gtkASS.enabled;
        ass_use_margins   = gtkASS.use_margins;
        ass_top_margin    = gtkASS.top_margin;
        ass_bottom_margin = gtkASS.bottom_margin;

        break;

    case GUI_SET_STREAM:

        if (guiInfo.StreamType == STREAMTYPE_PLAYLIST)
            guiInfo.mpcontext->file_format = DEMUXER_TYPE_PLAYLIST;

        stream = data;
        guiInfo.StreamType = stream->type;

        switch (guiInfo.StreamType) {
        case STREAMTYPE_FILE:
        case STREAMTYPE_STREAM:
            guiInfo.Tracks = (int)listMgr(PLAYLIST_ITEM_GET_POS, 0);
            break;

        case STREAMTYPE_CDDA:
            guiInfo.Tracks = 0;
            stream_control(stream, STREAM_CTRL_GET_NUM_TITLES, &guiInfo.Tracks);
            break;

        case STREAMTYPE_VCD:
            guiInfo.Tracks = 0;
            stream_control(stream, STREAM_CTRL_GET_NUM_TITLES, &guiInfo.Tracks);
            break;

        case STREAMTYPE_DVD:
            guiInfo.Tracks = 0;
            stream_control(stream, STREAM_CTRL_GET_NUM_TITLES, &guiInfo.Tracks);
            guiInfo.Chapters = 0;
            stream_control(stream, STREAM_CTRL_GET_NUM_CHAPTERS, &guiInfo.Chapters);
            guiInfo.Angles = 0;
            stream_control(stream, STREAM_CTRL_GET_NUM_ANGLES, &guiInfo.Angles);
            guiInfo.Track = 0;
            stream_control(stream, STREAM_CTRL_GET_CURRENT_TITLE, &guiInfo.Track);
            guiInfo.Track++;
            // guiInfo.Chapter will be set by mplayer
            guiInfo.Angle = 1;
            stream_control(stream, STREAM_CTRL_GET_ANGLE, &guiInfo.Angle);
#ifdef CONFIG_DVDREAD
            dvd = stream->priv;
            guiInfo.AudioStreams = dvd->nr_of_channels;
            memcpy(guiInfo.AudioStream, dvd->audio_streams, sizeof(dvd->audio_streams));
            guiInfo.Subtitles = dvd->nr_of_subtitles;
            memcpy(guiInfo.Subtitle, dvd->subtitles, sizeof(dvd->subtitles));
#endif
            break;
        }

        break;

    case GUI_SET_AFILTER:

        guiInfo.afilter = data;
        break;

    case GUI_SET_VIDEO:

        /* video */

        guiInfo.sh_video = data;

        nfree(guiInfo.CodecName);

        if (guiInfo.sh_video)
            guiInfo.CodecName = strdup(guiInfo.sh_video->codec->name);

        state = (guiInfo.StreamType == STREAMTYPE_STREAM ? btnDisabled : btnReleased);
        btnSet(evForward10sec, state);
        btnSet(evBackward10sec, state);
        btnSet(evForward1min, state);
        btnSet(evBackward1min, state);
        btnSet(evForward10min, state);
        btnSet(evBackward10min, state);
        btnSet(evSetMoviePosition, state);

        if (video_driver_list && !gstrcmp(video_driver_list[0], "dxr3") && (((demuxer_t *)mpctx_get_demuxer(guiInfo.mpcontext))->file_format != DEMUXER_TYPE_MPEG_PS) && !gtkVfLAVC) {
            gtkMessageBox(MSGBOX_FATAL, MSGTR_NEEDLAVC);
            return False;
        }

        break;

    case GUI_SET_AUDIO:

        guiInfo.AudioChannels = data ? ((sh_audio_t *)data)->channels : 0;

        if (data && !guiInfo.sh_video) {
            guiInfo.VideoWindow = False;
            guiInfo.VideoWidth  = 0;
            guiInfo.VideoHeight = 0;
        }

        gui(GUI_SET_MIXER, 0);

        if (gtkEnableAudioEqualizer) {
            equalizer_t eq;
            unsigned int i, j;

            for (i = 0; i < FF_ARRAY_ELEMS(gtkEquChannels); i++) {
                for (j = 0; j < FF_ARRAY_ELEMS(*gtkEquChannels); j++) {
                    eq.channel = i;
                    eq.band    = j;
                    eq.gain    = gtkEquChannels[i][j];
                    mplayer(MPLAYER_SET_EQUALIZER, 0, &eq);
                }
            }
        }

        // These must be done here (in the last call from MPlayer before
        // playback starts) and not in GUI_SETUP_VIDEO_WINDOW, because...

        // ...without video there will be no call to GUI_SETUP_VIDEO_WINDOW
        if (!guiInfo.VideoWindow) {
            wsWindowVisibility(&guiApp.videoWindow, wsHideWindow);
            btnSet(evFullScreen, (gtkLoadFullscreen ? btnPressed : btnReleased));
        }

        // ...option variable fullscreen determines whether MPlayer will handle
        //    the window given by WinID as fullscreen window (and will do aspect
        //    scaling then) or not - quite rubbish
        fullscreen = gtkLoadFullscreen;

        break;

    case GUI_SET_MIXER:

        if (guiInfo.mpcontext)
            mixer = mpctx_get_mixer(guiInfo.mpcontext);

        if (mixer) {
            float l, r, b;
            static float last_balance = 50.0f;

            mixer_getvolume(mixer, &l, &r);
            guiInfo.Volume = FFMAX(l, r);
            btnModify(evSetVolume, guiInfo.Volume);

            mixer_getbalance(mixer, &b);
            guiInfo.Balance = (b + 1.0) * 50.0; // transform -1..1 to 0..100

            if (guiInfo.Balance != last_balance) {
                l = guiInfo.Volume * (100.0 - guiInfo.Balance) / 50.0;
                r = guiInfo.Volume * guiInfo.Balance / 50.0;
                mixer_setvolume(mixer, FFMIN(l, guiInfo.Volume), FFMIN(r, guiInfo.Volume));
                btnModify(evSetBalance, guiInfo.Balance);
                last_balance = guiInfo.Balance;
            }
        }

        break;

    case GUI_REDRAW:

        uiEvent(ivRedraw, 0);
        break;

    case GUI_SETUP_VIDEO_WINDOW:

        guiInfo.VideoWidth  = vo_dwidth;
        guiInfo.VideoHeight = vo_dheight;

        if (!guiApp.videoWindow.isFullScreen || !guiApp.videoWindow.Mapped) {
            if (!guiApp.videoWindow.isFullScreen)
                wsWindowResize(&guiApp.videoWindow, guiInfo.VideoWidth, guiInfo.VideoHeight);
            if (!guiApp.videoWindow.Mapped)
                wsWindowVisibility(&guiApp.videoWindow, wsShowWindow);
        }

        if (gtkLoadFullscreen ^ guiApp.videoWindow.isFullScreen)
            uiEvent(evFullScreen, True);

        if (guiWinID >= 0)
            wsWindowMove(&guiApp.mainWindow, True, 0, guiInfo.VideoHeight);

        wsWindowBackground(&guiApp.videoWindow, -1, -1, -1);

        break;

    case GUI_HANDLE_X_EVENT:

        wsEvent(data);
        gtkEventHandling();
        break;

    case GUI_END_FILE:

        guiInfo.sh_video = NULL;

        uiEvent(ivRedraw, True);

        if (guiInfo.Playing) {
            if (!guiInfo.PlaylistNext) {
                guiInfo.PlaylistNext = True;
                break;
            }

            if (guiInfo.StreamType == STREAMTYPE_CDDA && guiInfo.Track < guiInfo.Tracks) {
                uiNext();
                break;
            }

            next = listMgr(PLAYLIST_ITEM_GET_NEXT, 0);
        }

        if (next) {
            uiSetFile(next->path, next->name, STREAMTYPE_FILE);
            guiInfo.NewPlay = GUI_FILE_NEW;
            guiInfo.Track   = (int)listMgr(PLAYLIST_ITEM_GET_POS, next);
        } else {
            if (guiInfo.NewPlay == GUI_FILE_NEW)
                break;

            filename = NULL;

            if (isPlaylistStreamtype) {
                plItem *curr = listMgr(PLAYLIST_ITEM_GET_CURR, 0);

                if (!curr)
                    uiUnsetFile();
                else if ((curr != listMgr(PLAYLIST_GET, 0)) && guiInfo.Playing) {
                    curr = listMgr(PLAYLIST_ITEM_SET_CURR, listMgr(PLAYLIST_GET, 0));
                    uiSetFile(curr->path, curr->name, STREAMTYPE_FILE);
                    guiInfo.Track = 1;
                }
            } else if (guiInfo.Playing) {
                int first = (guiInfo.StreamType == STREAMTYPE_VCD ? 2 : 1);

                if (guiInfo.Track != first) {
                    uiUnsetMedia(True);
                    guiInfo.Track = first;
                }

                if (guiInfo.StreamType == STREAMTYPE_DVD) {
                    guiInfo.Chapter = 1;
                    guiInfo.Angle   = 1;
                }
            }

            guiInfo.ElapsedTime = 0;
            guiInfo.Position    = 0;

            if (gtkShowVideoWindow) {
                guiInfo.VideoWindow = True;

                if (!guiApp.videoWindow.isFullScreen)
                    wsWindowResize(&guiApp.videoWindow, guiApp.video.width, guiApp.video.height);

                if (!guiApp.videoWindow.Mapped)
                    wsWindowVisibility(&guiApp.videoWindow, wsShowWindow);

                if (gtkLoadFullscreen ^ guiApp.videoWindow.isFullScreen)
                    uiEvent(evFullScreen, False);
            } else {
                wsWindowVisibility(&guiApp.videoWindow, wsHideWindow);
                guiInfo.VideoWindow = False;
                btnSet(evFullScreen, (gtkLoadFullscreen ? btnPressed : btnReleased));
            }

            gui(GUI_SET_STATE, (void *)GUI_STOP);

            wsEvents();
            wsWindowRedraw(&guiApp.videoWindow);
            wsMouseVisibility(&guiApp.videoWindow, wsShowMouseCursor);
        }

        break;
    }

    return True;
}

/**
 * @brief Initialize the GUI playlist (i.e. import files that had been given
 *        on the command line) or add files "on the fly" (i.e. replace the
 *        current one (a playlist file) by other ones (its content)).
 *
 * @param what command (#GUI_PLAYLIST_INIT or #GUI_PLAYLIST_ADD) to be performed
 * @param playtree MPlayer playtree to read from
 * @param config MPlayer config context
 * @param enqueue whether to overwrite GUI playlist (#False) or to append to it (#True)
 *
 * @return #True (ok) or #False (error)
 */
int guiPlaylist(int what, play_tree_t *playtree, m_config_t *config, int enqueue)
{
    play_tree_iter_t *pt_iter;
    const char *file;
    int added = False;
    plItem *curr;

    pt_iter = pt_iter_create(&playtree, config);

    if (!pt_iter)
        return False;

    switch (what) {
    case GUI_PLAYLIST_INIT:

        if (!enqueue)
            listMgr(PLAYLIST_DELETE, 0);

        while ((file = pt_iter_get_next_file(pt_iter)))
            if (add_to_gui_playlist(file, PLAYLIST_ITEM_APPEND))
                added = True;

        uiCurr();   // update filename
        guiInfo.PlaylistNext = True;

        if (added)
            guiInfo.Track = 1;

        if (enqueue)
            filename = NULL;            // don't start playing

        break;

    case GUI_PLAYLIST_ADD:

        curr = listMgr(PLAYLIST_ITEM_GET_CURR, 0);

        while ((file = pt_iter_get_next_file(pt_iter)))
            if (add_to_gui_playlist(file, PLAYLIST_ITEM_INSERT))
                added = True;

        if (curr)
            listMgr(PLAYLIST_ITEM_SET_CURR, curr);
        else
            listMgr(PLAYLIST_ITEM_SET_CURR, listMgr(PLAYLIST_GET, 0));

        if (curr && added)
            listMgr(PLAYLIST_ITEM_DEL_CURR, 0);

        uiCurr();   // update filename

        break;
    }

    pt_iter_destroy(&pt_iter);

    return added;
}

/* GUI -> MPlayer */

void mplayer(int what, float value, void *data)
{
    equalizer_t *eq = (equalizer_t *)data;

    switch (what) {
    /* subtitle */

    case MPLAYER_SET_FONT_FACTOR:
        font_factor = value;
        mplayerLoadFont();
        break;

    case MPLAYER_SET_FONT_OUTLINE:
        subtitle_font_thickness = (8.0f / 100.0f) * value;
        mplayerLoadFont();
        break;

    case MPLAYER_SET_FONT_BLUR:
        subtitle_font_radius = (8.0f / 100.0f) * value;
        mplayerLoadFont();
        break;

    case MPLAYER_SET_FONT_TEXTSCALE:
        text_font_scale_factor = value;
        mplayerLoadFont();
        break;

    case MPLAYER_SET_FONT_OSDSCALE:
        osd_font_scale_factor = value;
        mplayerLoadFont();
        break;

    case MPLAYER_SET_FONT_ENCODING:
        nfree(subtitle_font_encoding);
        subtitle_font_encoding = gstrdup((char *)data);
        mplayerLoadFont();
        break;

    case MPLAYER_SET_FONT_AUTOSCALE:
        subtitle_autoscale = (int)value;
        mplayerLoadFont();
        break;

    case MPLAYER_SET_SUB_ENCODING:
        nfree(sub_cp);
        sub_cp = gstrdup((char *)data);
        break;

    case MPLAYER_SET_EXTRA_STEREO:
        gtkAOExtraStereoMul = value;
        if (guiInfo.afilter)
            af_control_any_rev(guiInfo.afilter, AF_CONTROL_ES_MUL | AF_CONTROL_SET, &gtkAOExtraStereoMul);
        break;

    case MPLAYER_SET_PANSCAN:
    {
        mp_cmd_t *mp_cmd;

        mp_cmd       = calloc(1, sizeof(*mp_cmd));
        mp_cmd->id   = MP_CMD_PANSCAN;
        mp_cmd->name = strdup("panscan");
        mp_cmd->args[0].v.f = value;
        mp_cmd->args[1].v.i = 1;
        mp_input_queue_cmd(mp_cmd);
    }
    break;

    case MPLAYER_SET_AUTO_QUALITY:
        auto_quality = (int)value;
        break;

    /* set equalizers */

    case MPLAYER_SET_CONTRAST:
        if (guiInfo.sh_video)
            set_video_colors(guiInfo.sh_video, "contrast", (int)value);
        break;

    case MPLAYER_SET_BRIGHTNESS:
        if (guiInfo.sh_video)
            set_video_colors(guiInfo.sh_video, "brightness", (int)value);
        break;

    case MPLAYER_SET_HUE:
        if (guiInfo.sh_video)
            set_video_colors(guiInfo.sh_video, "hue", (int)value);
        break;

    case MPLAYER_SET_SATURATION:
        if (guiInfo.sh_video)
            set_video_colors(guiInfo.sh_video, "saturation", (int)value);
        break;

    case MPLAYER_SET_EQUALIZER:
    {
        af_control_ext_t tmp;

        if (eq) {
            gtkEquChannels[eq->channel][eq->band] = eq->gain;
            tmp.ch  = eq->channel;
            tmp.arg = gtkEquChannels[eq->channel];

            if (guiInfo.afilter)
                af_control_any_rev(guiInfo.afilter, AF_CONTROL_EQUALIZER_GAIN | AF_CONTROL_SET, &tmp);
        } else {
            unsigned int i;

            memset(gtkEquChannels, 0, sizeof(gtkEquChannels));

            if (guiInfo.afilter) {
                for (i = 0; i < FF_ARRAY_ELEMS(gtkEquChannels); i++) {
                    tmp.ch  = i;
                    tmp.arg = gtkEquChannels[i];
                    af_control_any_rev(guiInfo.afilter, AF_CONTROL_EQUALIZER_GAIN | AF_CONTROL_SET, &tmp);
                }
            }
        }

        break;
    }

    case MPLAYER_EXIT_GUI:
        exit_player_with_rc((enum exit_reason)value, (enum exit_reason)value >= EXIT_ERROR);
        break;
    }
}

void mplayerLoadFont(void)
{
#ifdef CONFIG_FREETYPE
    load_font_ft(vo_image_width, vo_image_height, &vo_font, font_name, osd_font_scale_factor);
#else
    if (vo_font) {
        int i;

        free(vo_font->name);
        free(vo_font->fpath);

        for (i = 0; i < 16; i++) {
            if (vo_font->pic_a[i]) {
                free(vo_font->pic_a[i]->bmp);
                free(vo_font->pic_a[i]->pal);
            }
        }

        for (i = 0; i < 16; i++) {
            if (vo_font->pic_b[i]) {
                free(vo_font->pic_b[i]->bmp);
                free(vo_font->pic_b[i]->pal);
            }
        }

        free(vo_font);
        vo_font = NULL;
    }

    if (font_name) {
        vo_font = read_font_desc(font_name, font_factor, 0);

        if (!vo_font)
            gmp_msg(MSGT_GPLAYER, MSGL_ERR, MSGTR_CantLoadFont, font_name);
    } else {
        font_name = gstrdup(get_path("font/font.desc"));
        vo_font   = read_font_desc(font_name, font_factor, 0);

        if (!vo_font) {
            nfree(font_name);
            font_name = gstrdup(MPLAYER_DATADIR "/font/font.desc");
            vo_font   = read_font_desc(font_name, font_factor, 0);
        }
    }
#endif
}

void mplayerLoadSubtitle(const char *name)
{
    if (guiInfo.Playing == GUI_STOP)
        return;

    if (subdata) {
        mp_msg(MSGT_GPLAYER, MSGL_INFO, MSGTR_DeletingSubtitles);

        sub_free(subdata);
        subdata = NULL;
        vo_sub  = NULL;

        if (vo_osd_list) {
            int len;
            mp_osd_obj_t *osd;

            osd = vo_osd_list;

            while (osd) {
                if (osd->type == OSDTYPE_SUBTITLE)
                    break;

                osd = osd->next;
            }

            if (osd && (osd->flags & OSDFLAG_VISIBLE)) {
                len = osd->stride * (osd->bbox.y2 - osd->bbox.y1);
                memset(osd->bitmap_buffer, 0, len);
                memset(osd->alpha_buffer, 0, len);
            }
        }
    }

    if (name) {
        mp_msg(MSGT_GPLAYER, MSGL_INFO, MSGTR_LoadingSubtitles, name);

        subdata = sub_read_file(name, (guiInfo.sh_video ? guiInfo.sh_video->fps : 0));

        if (!subdata)
            gmp_msg(MSGT_GPLAYER, MSGL_ERR, MSGTR_CantLoadSub, name);

        sub_name    = (malloc(2 * sizeof(char *))); // when mplayer will be restarted
        sub_name[0] = strdup(name);                 // sub_name[0] will be read
        sub_name[1] = NULL;
    }

    update_set_of_subtitles();
}

// NOTE TO MYSELF: This function is nonsense.
//                 MPlayer should pass messages to the GUI
//                 which must decide then which message has
//                 to be shown (MSGL_FATAL, for example).
//                 But with this function it is at least
//                 possible to show GUI's very critical or
//                 abort messages.
void gmp_msg(int mod, int lev, const char *format, ...)
{
    char msg[512];
    va_list va;

    va_start(va, format);
    vsnprintf(msg, sizeof(msg), format, va);
    va_end(va);

    mp_msg(mod, lev, msg);

    if (mp_msg_test(mod, lev))
        gtkMessageBox(MSGBOX_FATAL, msg);
}
