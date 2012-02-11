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

#ifndef MPLAYER_GUI_LIST_H
#define MPLAYER_GUI_LIST_H

#define gtkAddPlItem     5
#define gtkGetNextPlItem 6
#define gtkGetPrevPlItem 7
#define gtkGetCurrPlItem 8
#define gtkDelPl         9
#define gtkDelURL        10
#define gtkDelCurrPlItem 23
#define gtkInsertPlItem  24
#define gtkSetCurrPlItem 25
#define gtkAddURLItem    15

typedef struct plItem {
    struct plItem *prev, *next;
    char *path;
    char *name;
} plItem;

typedef struct urlItem {
    struct urlItem *next;
    char *url;
} urlItem;

extern plItem *plList;
extern plItem *plCurrent;
extern plItem *plLastPlayed;

extern urlItem *urlList;

void gaddlist(char ***list, const char *entry);
void greplace(char ***list, const char *search, const char *replace);
void *listSet(int cmd, void *vparam);

#endif /* MPLAYER_GUI_LIST_H */
