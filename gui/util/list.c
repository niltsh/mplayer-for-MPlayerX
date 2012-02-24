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
 * @brief List management
 */

#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "string.h"

static plItem *plList;
static plItem *plCurrent;

static urlItem *urlList;

/**
 * @brief Manage playlists and URL lists.
 *
 * @param cmd task to be performed
 * @param data list item for the task
 *
 * @return pointer to top of list (GET command),
 *         pointer to current list item (ITEM command) or
 *         NULL (DELETE or unknown command)
 *
 */
void *listMgr(int cmd, void *data)
{
    plItem *pdat  = (plItem *)data;
    urlItem *udat = (urlItem *)data;

    switch (cmd) {
    /* playlist */

    case PLAYLIST_GET:

        return plList;

    case PLAYLIST_ITEM_APPEND:

        if (plList) {
            plItem *item = plList;

            while (item->next)
                item = item->next;

            item->next = pdat;
            pdat->prev = item;
            pdat->next = NULL;
        } else {
            pdat->next = pdat->prev = NULL;
            plCurrent  = plList = pdat;
        }

        return plCurrent;

    case PLAYLIST_ITEM_INSERT:

        if (plCurrent) {
            pdat->next = plCurrent->next;

            if (pdat->next)
                pdat->next->prev = pdat;

            pdat->prev      = plCurrent;
            plCurrent->next = pdat;

            plCurrent = pdat;

            return plCurrent;
        } else
            return listMgr(PLAYLIST_ITEM_APPEND, pdat);

    case PLAYLIST_ITEM_SET_CURR:

        plCurrent = pdat;
        return plCurrent;

    case PLAYLIST_ITEM_GET_CURR:

        return plCurrent;

    case PLAYLIST_ITEM_GET_PREV:

        if (plCurrent && plCurrent->prev) {
            plCurrent = plCurrent->prev;
            return plCurrent;
        }

        return NULL;

    case PLAYLIST_ITEM_GET_NEXT:

        if (plCurrent && plCurrent->next) {
            plCurrent = plCurrent->next;
            return plCurrent;
        }

        return NULL;

    case PLAYLIST_ITEM_DEL_CURR:

        if (plCurrent) {
            plItem *curr = plCurrent;

            if (curr->prev)
                curr->prev->next = curr->next;
            if (curr->next)
                curr->next->prev = curr->prev;

            plCurrent = curr->next;

            if (curr == plList)
                plList = plCurrent;

            free(curr->path);
            free(curr->name);
            free(curr);
        }

        return plCurrent;

    case PLAYLIST_DELETE:

        while (plList) {
            plItem *item = plList->next;

            free(plList->path);
            free(plList->name);
            free(plList);

            plList = item;
        }

        plCurrent = NULL;
        return NULL;

    /* URL list */

    case URLLIST_GET:

        return urlList;

    case URLLIST_ITEM_ADD:

        if (urlList) {
            urlItem *item = urlList;

            while (item) {
                if (strcmp(udat->url, item->url) == 0) {
                    free(udat->url);
                    free(udat);
                    return NULL;
                }

                if (item->next)
                    item = item->next;
                else {
                    item->next = udat;
                    udat->next = NULL;
                    break;
                }
            }
        } else {
            udat->next = NULL;
            urlList    = udat;
        }

        return udat;

    case URLLIST_DELETE:

        while (urlList) {
            urlItem *item = urlList->next;

            free(urlList->url);
            free(urlList);

            urlList = item;
        }

        return NULL;

    default:

        return NULL;
    }
}

/**
 * @brief Set list to @a entry.
 *
 * @param list pointer to the char pointer list
 * @param entry the new (and only) element of the list
 *
 * @note Actually, a new list will be created and the old list will be freed.
 */
void listSet(char ***list, const char *entry)
{
    if (*list) {
        char **l = *list;

        while (*l) {
            free(*l);
            l++;
        }

        free(*list);
    }

    *list = malloc(2 * sizeof(char *));

    if (*list) {
        (*list)[0] = gstrdup(entry);
        (*list)[1] = NULL;
    }
}

/**
 * @brief Replace the first element in list that starts with @a search.
 *
 * @note If no such element is found, @a replace will be appended.
 *
 * @param list pointer to the char pointer list
 * @param search element to search
 * @param replace replacement element
 */
void listRepl(char ***list, const char *search, const char *replace)
{
    int i      = 0;
    char **org = *list;

    if (!replace)
        return;

    if (*list) {
        size_t len = (search ? strlen(search) : 0);

        for (i = 0; (*list)[i]; i++) {
            if (gstrncmp((*list)[i], search, len) == 0) {
                free((*list)[i]);
                (*list)[i] = strdup(replace);
                return;
            }
        }

        *list = realloc(*list, (i + 2) * sizeof(char *));
    } else
        *list = malloc(2 * sizeof(char *));

    if (!*list) {
        *list = org;
        return;
    }

    (*list)[i]     = strdup(replace);
    (*list)[i + 1] = NULL;
}
