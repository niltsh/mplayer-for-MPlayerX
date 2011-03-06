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

#include "app.h"

#include "gui/skin/font.h"
#include "interface.h"

#include "libavutil/common.h"

guiItems appMPlayer;

static const evName evNames[] = {
    { evNone,              "evNone"              },
    { evPlay,              "evPlay"              },
    { evStop,              "evStop"              },
    { evPause,             "evPause"             },
    { evPrev,              "evPrev"              },
    { evNext,              "evNext"              },
    { evLoad,              "evLoad"              },
    { evEqualizer,         "evEqualizer"         },
    { evEqualizer,         "evEqualeaser"        }, // NOTE TO MYSELF: any skin using this?
    { evPlayList,          "evPlaylist"          },
    { evExit,              "evExit"              },
    { evIconify,           "evIconify"           },
    { evIncBalance,        "evIncBalance"        }, // NOTE TO MYSELF: not all of these events
    { evDecBalance,        "evDecBalance"        }, // are actually implemented, and update doc
    { evFullScreen,        "evFullScreen"        },
    { evFName,             "evFName"             },
    { evMovieTime,         "evMovieTime"         },
    { evAbout,             "evAbout"             },
    { evLoadPlay,          "evLoadPlay"          },
    { evPreferences,       "evPreferences"       },
    { evSkinBrowser,       "evSkinBrowser"       },
    { evBackward10sec,     "evBackward10sec"     },
    { evForward10sec,      "evForward10sec"      },
    { evBackward1min,      "evBackward1min"      },
    { evForward1min,       "evForward1min"       },
    { evBackward10min,     "evBackward10min"     },
    { evForward10min,      "evForward10min"      },
    { evIncVolume,         "evIncVolume"         },
    { evDecVolume,         "evDecVolume"         },
    { evMute,              "evMute"              },
    { evIncAudioBufDelay,  "evIncAudioBufDelay"  },
    { evDecAudioBufDelay,  "evDecAudioBufDelay"  },
    { evPlaySwitchToPause, "evPlaySwitchToPause" },
    { evPauseSwitchToPlay, "evPauseSwitchToPlay" },
    { evNormalSize,        "evHalfSize"          },
    { evNormalSize,        "evNormalSize"        },
    { evDoubleSize,        "evDoubleSize"        },
    { evSetMoviePosition,  "evSetMoviePosition"  },
    { evSetVolume,         "evSetVolume"         },
    { evSetBalance,        "evSetBalance"        },
    { evHelp,              "evHelp"              },
    { evLoadSubtitle,      "evLoadSubtitle"      },
    { evPlayDVD,           "evPlayDVD"           },
    { evPlayVCD,           "evPlayVCD"           },
    { evSetURL,            "evSetURL"            },
    { evLoadAudioFile,     "evLoadAudioFile"     },
    { evDropSubtitle,      "evDropSubtitle"      },
    { evSetAspect,         "evSetAspect"         }
};

static void appClearItem(wItem *item)
{
    item->type   = itNone;
    item->x      = 0;
    item->y      = 0;
    item->width  = 0;
    item->height = 0;
    bpFree(&item->Bitmap);
    bpFree(&item->Mask);
    item->fontid = 0;
    item->align  = fntAlignLeft;
    gfree((void **)&item->label);
    item->pwidth    = 0;
    item->pheight   = 0;
    item->numphases = 0;
    item->value     = 0;
    item->message   = evNone;
    item->R = 0;
    item->G = 0;
    item->B = 0;
    gfree((void **)&item->text);
    item->textwidth = 0;
    item->starttime = 0;
    item->last_x    = 0;
    item->pressed   = btnDisabled;
    item->tmp       = 0;
}

void appResetStruct(void)
{
    appMPlayer.IndexOfMainItems = -1;
    appMPlayer.IndexOfBarItems  = -1;
    appMPlayer.IndexOfMenuItems = -1;

    appMPlayer.sub.x = -1;   // NOTE TO MYSELF: is this really necessary?
    appMPlayer.sub.y = -1;   // NOTE TO MYSELF: is this really necessary?
}

void appFreeStruct(void)
{
    int i;

    appClearItem(&appMPlayer.main);
    appMPlayer.mainDecoration = 0;

    appClearItem(&appMPlayer.sub);

    appClearItem(&appMPlayer.bar);
    appMPlayer.barIsPresent = 0;

    appClearItem(&appMPlayer.menuBase);
    appClearItem(&appMPlayer.menuSelected);
    appMPlayer.menuIsPresent = 0;

    for (i = 0; i <= appMPlayer.IndexOfMainItems; i++)
        appClearItem(&appMPlayer.mainItems[i]);
    for (i = 0; i <= appMPlayer.IndexOfBarItems; i++)
        appClearItem(&appMPlayer.barItems[i]);
    for (i = 0; i <= appMPlayer.IndexOfMenuItems; i++)
        appClearItem(&appMPlayer.menuItems[i]);

    appResetStruct();
    fntFreeFont();
}

int appFindMessage(unsigned char *str)
{
    unsigned int i;

    for (i = 0; i < FF_ARRAY_ELEMS(evNames); i++)
        if (!strcmp(evNames[i].name, str))
            return evNames[i].message;

    return -1;
}

void btnModify(int event, float state)
{
    int i;

    for (i = 0; i <= appMPlayer.IndexOfMainItems; i++) {
        if (appMPlayer.mainItems[i].message == event) {
            switch (appMPlayer.mainItems[i].type) {
            case itButton:
                appMPlayer.mainItems[i].pressed = (int)state;
                appMPlayer.mainItems[i].tmp     = (int)state;
                break;

            case itPotmeter:
            case itVPotmeter:
            case itHPotmeter:
                if (state < 0.0)
                    state = 0.0;
                if (state > 100.0)
                    state = 100.0;
                appMPlayer.mainItems[i].value = state;
                break;
            }
        }
    }

    for (i = 0; i <= appMPlayer.IndexOfBarItems; i++) {
        if (appMPlayer.barItems[i].message == event) {
            switch (appMPlayer.barItems[i].type) {
            case itButton:
                appMPlayer.barItems[i].pressed = (int)state;
                appMPlayer.barItems[i].tmp     = (int)state;
                break;

            case itPotmeter:
            case itVPotmeter:
            case itHPotmeter:
                if (state < 0.0)
                    state = 0.0;
                if (state > 100.0)
                    state = 100.0;
                appMPlayer.barItems[i].value = state;
                break;
            }
        }
    }
}

void btnSet(int event, int set)
{
    int i;

    for (i = 0; i <= appMPlayer.IndexOfMainItems; i++) {
        if (appMPlayer.mainItems[i].message == event) {
            appMPlayer.mainItems[i].pressed = set;
            appMPlayer.barItems[i].tmp      = 0;
        }
    }

    for (i = 0; i <= appMPlayer.IndexOfBarItems; i++) {
        if (appMPlayer.barItems[i].message == event) {
            appMPlayer.barItems[i].pressed = set;
            appMPlayer.barItems[i].tmp     = 0;
        }
    }
}
