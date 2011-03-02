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

#include <stdlib.h>
#include <string.h>

#include "app.h"

listItems appMPlayer;

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

static const int evBoxs = sizeof(evNames) / sizeof(evName);

static void appClearItem(wItem *item)
{
    item->type             = 0;
    item->x                = 0;
    item->y                = 0;
    item->width            = 0;
    item->height           = 0;
    item->pwidth           = 0;
    item->pheight          = 0;
    item->message          = 0;
    item->pressed          = btnReleased;
    item->tmp              = 0;
    item->Bitmap.Width     = 0;
    item->Bitmap.Height    = 0;
    item->Bitmap.BPP       = 0;
    item->Bitmap.ImageSize = 0;
    free(item->Bitmap.Image);
    item->Bitmap.Image = NULL;
    item->fontid       = 0;
    free(item->label);
    item->label = NULL;
    free(item->text);
    item->text      = NULL;
    item->textwidth = 0;
    item->starttime = 0;
    item->last_x    = 0;
}

void appInitStruct(listItems *item)
{
    int i;

    for (i = 0; i < item->NumberOfMainItems; i++)
        appClearItem(&item->mainItems[i]);
    for (i = 0; i < item->NumberOfBarItems; i++)
        appClearItem(&item->barItems[i]);
    for (i = 0; i < item->NumberOfMenuItems; i++)
        appClearItem(&item->menuItems[i]);

    item->NumberOfMainItems = -1;
    memset(item->mainItems, 0, 256 * sizeof(wItem));

    item->NumberOfMenuItems = -1;
    memset(item->menuItems, 0, 64 * sizeof(wItem));

    item->NumberOfBarItems = -1;
    memset(item->barItems, 0, 256 * sizeof(wItem));

    appClearItem(&item->main);
    item->mainDecoration = 0;

    appClearItem(&item->sub);
    item->sub.width  = 0;
    item->sub.height = 0;
    item->sub.x      = -1;
    item->sub.y      = -1;

    appClearItem(&item->menuBase);
    appClearItem(&item->menuSelected);

    item->sub.R         = item->sub.G = item->sub.B = 0;
    item->bar.R         = item->bar.G = item->bar.B = 0;
    item->main.R        = item->main.G = item->main.B = 0;
    item->barIsPresent  = 0;
    item->menuIsPresent = 0;
}

int appFindMessage(unsigned char *str)
{
    int i;

    for (i = 0; i < evBoxs; i++)
        if (!strcmp(evNames[i].name, str))
            return evNames[i].message;

    return -1;
}

void btnModify(int event, float state)
{
    int i;

    for (i = 0; i < appMPlayer.NumberOfMainItems + 1; i++) {
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

    for (i = 0; i < appMPlayer.NumberOfBarItems + 1; i++) {
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

    for (i = 0; i < appMPlayer.NumberOfMainItems + 1; i++) {
        if (appMPlayer.mainItems[i].message == event) {
            appMPlayer.mainItems[i].pressed = set;
            appMPlayer.barItems[i].tmp      = 0;
        }
    }

    for (i = 0; i < appMPlayer.NumberOfBarItems + 1; i++) {
        if (appMPlayer.barItems[i].message == event) {
            appMPlayer.barItems[i].pressed = set;
            appMPlayer.barItems[i].tmp     = 0;
        }
    }
}
