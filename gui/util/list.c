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
