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

#include "list.h"
#include "string.h"

plItem *plList;
plItem *plCurrent;
plItem *plLastPlayed;

urlItem *urlList;

void *listSet(int cmd, void *vparam)
{
    plItem *item      = (plItem *)vparam;
    urlItem *url_item = (urlItem *)vparam;
    int is_added      = 1;

    switch (cmd) {
    // handle playlist

    // add item to playlist
    case gtkAddPlItem:
        if (plList) {
            plItem *next = plList;

            while (next->next)
// {
// printf( "%s\n",next->name );
                next = next->next;
// }

            next->next = item;
            item->prev = next;
            item->next = NULL;
        } else {
            item->prev = item->next = NULL;
            plCurrent  = plList = item;
        }
        return NULL;

    // add item into playlist after current
    case gtkInsertPlItem:
        if (plCurrent) {
            plItem *curr = plCurrent;
            item->next = curr->next;

            if (item->next)
                item->next->prev = item;

            item->prev = curr;
            curr->next = item;
            plCurrent  = plCurrent->next;

            return plCurrent;
        } else
            return listSet(gtkAddPlItem, item);

    // get next item from playlist
    case gtkGetNextPlItem:
        if (plCurrent && plCurrent->next) {
            plCurrent = plCurrent->next;
// if (!plCurrent && plList)
// {
// plItem *next = plList;
//
// while (next->next)
// {
// if (!next->next) break;
// next = next->next;
// }
//
// plCurrent = next;
// }
            return plCurrent;
        }
        return NULL;

    // get previous item from playlist
    case gtkGetPrevPlItem:
        if (plCurrent && plCurrent->prev) {
            plCurrent = plCurrent->prev;
// if ( !plCurrent && plList ) plCurrent=plList;
            return plCurrent;
        }
        return NULL;

    // set current item
    case gtkSetCurrPlItem:
        plCurrent = item;
        return plCurrent;

    // get current item
    case gtkGetCurrPlItem:
        return plCurrent;

    // delete current item
    case gtkDelCurrPlItem:
    {
        plItem *curr = plCurrent;

        if (!curr)
            return NULL;

        if (curr->prev)
            curr->prev->next = curr->next;
        if (curr->next)
            curr->next->prev = curr->prev;
        if (curr == plList)
            plList = curr->next;

        plCurrent = curr->next;

        // free it
        free(curr->path);
        free(curr->name);
        free(curr);
    }
        //uiCurr();     // instead of using uiNext && uiPrev
        return plCurrent;

    // delete list
    case gtkDelPl:
    {
        plItem *curr = plList;
        plItem *next;

        if (!plList)
            return NULL;

        if (!curr->next) {
            free(curr->path);
            free(curr->name);
            free(curr);
        } else {
            while (curr->next) {
                next = curr->next;
                free(curr->path);
                free(curr->name);
                free(curr);
                curr = next;
            }
        }

        plList    = NULL;
        plCurrent = NULL;
    }
        return NULL;

    // handle url
    case gtkAddURLItem:
        if (urlList) {
            urlItem *next_url = urlList;
            is_added = 0;

            while (next_url->next) {
                if (!gstrcmp(next_url->url, url_item->url)) {
                    is_added = 1;
                    break;
                }

                next_url = next_url->next;
            }

            if (!is_added && gstrcmp(next_url->url, url_item->url))
                next_url->next = url_item;
        } else {
            url_item->next = NULL;
            urlList = url_item;
        }
        return NULL;

    case gtkDelURL:
        while (urlList) {
            urlItem *next = urlList->next;

            free(urlList->url);
            free(urlList);

            urlList = next;
        }
        return NULL;
    }

    return NULL;
}

/**
 * \brief This actually creates a new list containing only one element...
 */
void gaddlist(char ***list, const char *entry)
{
    int i;

    if (*list) {
        for (i = 0; (*list)[i]; i++)
            free((*list)[i]);

        free(*list);
    }

    *list      = malloc(2 * sizeof(char **));
    (*list)[0] = gstrdup(entry);
    (*list)[1] = NULL;
}

/**
 * \brief This replaces a string starting with search by replace.
 * If not found, replace is appended.
 */
void greplace(char ***list, const char *search, const char *replace)
{
    int i   = 0;
    int len = (search ? strlen(search) : 0);

    if (*list) {
        for (i = 0; (*list)[i]; i++) {
            if (search && (strncmp((*list)[i], search, len) == 0)) {
                free((*list)[i]);
                (*list)[i] = gstrdup(replace);
                return;
            }
        }

        *list = realloc(*list, (i + 2) * sizeof(char *));
    } else
        *list = malloc(2 * sizeof(char *));

    (*list)[i]     = gstrdup(replace);
    (*list)[i + 1] = NULL;
}
