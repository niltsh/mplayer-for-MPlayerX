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
 * @brief GUI application helpers
 */

#include "app.h"
#include "skin/font.h"

#include "libavutil/common.h"

/**
 * @brief Initialize item counters.
 */
guiItems guiApp = {
    .IndexOfMainItems    = -1,
    .IndexOfPlaybarItems = -1,
    .IndexOfMenuItems    = -1
};

/**
 * @brief Event messages belonging to event names.
 */
static const evName evNames[] = {
    { evNone,              "evNone"              },
    { evPlay,              "evPlay"              },
    { evStop,              "evStop"              },
    { evPause,             "evPause"             },
    { evPrev,              "evPrev"              },
    { evNext,              "evNext"              },
    { evLoad,              "evLoad"              },
    { evLoadPlay,          "evLoadPlay"          },
    { evLoadAudioFile,     "evLoadAudioFile"     },
    { evLoadSubtitle,      "evLoadSubtitle"      },
    { evDropSubtitle,      "evDropSubtitle"      },
    { evPlaylist,          "evPlaylist"          },
    { evPlayCD,            "evPlayCD"            },
    { evPlayVCD,           "evPlayVCD"           },
    { evPlayDVD,           "evPlayDVD"           },
    { evLoadURL,           "evSetURL"            }, // legacy
    { evLoadURL,           "evLoadURL"           },
    { evPlaySwitchToPause, "evPlaySwitchToPause" },
    { evPauseSwitchToPlay, "evPauseSwitchToPlay" },
    { evBackward10sec,     "evBackward10sec"     },
    { evForward10sec,      "evForward10sec"      },
    { evBackward1min,      "evBackward1min"      },
    { evForward1min,       "evForward1min"       },
    { evBackward10min,     "evBackward10min"     },
    { evForward10min,      "evForward10min"      },
    { evSetMoviePosition,  "evSetMoviePosition"  },
    { evHalfSize,          "evHalfSize"          },
    { evDoubleSize,        "evDoubleSize"        },
    { evFullScreen,        "evFullScreen"        },
    { evNormalSize,        "evNormalSize"        },
    { evSetAspect,         "evSetAspect"         },
    { evIncVolume,         "evIncVolume"         },
    { evDecVolume,         "evDecVolume"         },
    { evSetVolume,         "evSetVolume"         },
    { evMute,              "evMute"              },
    { evSetBalance,        "evSetBalance"        },
    { evEqualizer,         "evEqualizer"         },
    { evAbout,             "evAbout"             },
    { evPreferences,       "evPreferences"       },
    { evSkinBrowser,       "evSkinBrowser"       },
    { evMenu,              "evMenu"              },
    { evIconify,           "evIconify"           },
    { evExit,              "evExit"              }
};

/**
 * @brief Free all memory allocated to an item and set all its pointers to NULL.
 *
 * @param item item to be freed
 */
static void appClearItem(wItem *item)
{
    bpFree(&item->Bitmap);
    bpFree(&item->Mask);
    free(item->label);
    free(item->text);
    memset(item, 0, sizeof(*item));
}

/**
 * @brief Free all memory allocated to all GUI items and reset all item counters.
 */
void appFreeStruct(void)
{
    int i;

    appClearItem(&guiApp.main);
    guiApp.mainDecoration = 0;

    appClearItem(&guiApp.video);

    appClearItem(&guiApp.playbar);
    guiApp.playbarIsPresent = 0;

    appClearItem(&guiApp.menu);
    appClearItem(&guiApp.menuSelected);
    guiApp.menuIsPresent = 0;

    for (i = 0; i <= guiApp.IndexOfMainItems; i++)
        appClearItem(&guiApp.mainItems[i]);
    for (i = 0; i <= guiApp.IndexOfPlaybarItems; i++)
        appClearItem(&guiApp.playbarItems[i]);
    for (i = 0; i <= guiApp.IndexOfMenuItems; i++)
        appClearItem(&guiApp.menuItems[i]);

    guiApp.IndexOfMainItems    = -1;
    guiApp.IndexOfPlaybarItems = -1;
    guiApp.IndexOfMenuItems    = -1;

    fntFreeFont();
}

/**
 * @brief Find the event belonging to an event name.
 *
 * @param name event name
 *
 * @return event >= 0 (ok) or -1 (not found)
 */
int appFindMessage(const char *name)
{
    unsigned int i;

    for (i = 0; i < FF_ARRAY_ELEMS(evNames); i++)
        if (!strcmp(evNames[i].name, name))
            return evNames[i].message;

    return -1;
}

/**
 * @brief Find the item belonging to an event.
 *
 * @param event event
 *
 * @return pointer to the item (ok) or NULL (not found)
 */
wItem *appFindItem(int event)
{
    wItem *item;
    int i, n;

    if (guiApp.videoWindow.isFullScreen && guiApp.playbarIsPresent) {
        item = guiApp.playbarItems;
        n    = guiApp.IndexOfPlaybarItems;
    } else {
        item = guiApp.mainItems;
        n    = guiApp.IndexOfMainItems;
    }

    for (i = 0; i <= n; i++)
        if (item[i].message == event)
            return &item[i];

    return NULL;
}

/**
 * @brief Modify the state (i.e. set a new value) to the item belonging to an event.
 *
 * @param event event
 * @param state new value
 */
void btnModify(int event, float state)
{
    int i;

    for (i = 0; i <= guiApp.IndexOfMainItems; i++) {
        if (guiApp.mainItems[i].message == event) {
            switch (guiApp.mainItems[i].type) {
            case itButton:
                guiApp.mainItems[i].pressed = (int)state;
                break;

            case itPotmeter:
            case itVPotmeter:
            case itHPotmeter:
                if (state < 0.0f)
                    state = 0.0f;
                if (state > 100.0f)
                    state = 100.0f;
                guiApp.mainItems[i].value = state;
                break;
            }
        }
    }

    for (i = 0; i <= guiApp.IndexOfPlaybarItems; i++) {
        if (guiApp.playbarItems[i].message == event) {
            switch (guiApp.playbarItems[i].type) {
            case itButton:
                guiApp.playbarItems[i].pressed = (int)state;
                break;

            case itPotmeter:
            case itVPotmeter:
            case itHPotmeter:
                if (state < 0.0f)
                    state = 0.0f;
                if (state > 100.0f)
                    state = 100.0f;
                guiApp.playbarItems[i].value = state;
                break;
            }
        }
    }
}

/**
 * @brief Set the @a pressed state (i.e. a new value) to the item belonging to an event.
 *
 * @param event event
 * @param set new value
 */
void btnSet(int event, int set)
{
    int i;

    for (i = 0; i <= guiApp.IndexOfMainItems; i++)
        if (guiApp.mainItems[i].message == event)
            guiApp.mainItems[i].pressed = set;

    for (i = 0; i <= guiApp.IndexOfPlaybarItems; i++)
        if (guiApp.playbarItems[i].message == event)
            guiApp.playbarItems[i].pressed = set;
}
