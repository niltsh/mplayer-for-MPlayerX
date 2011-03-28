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

#include <stddef.h>
#include <string.h>

#include "string.h"

char *strswap(char *in, char from, char to)
{
    char *p = in;

    while (*p) {
        if (*p == from)
            *p = to;

        p++;
    }

    return in;
}

char *trim(char *in)
{
    int c = 0, id = 0, i;

    if (!*in)
        return NULL;

    while (c != (int)strlen(in)) {
        if (in[c] == '"')
            id = !id;

        if ((in[c] == ' ') && (!id)) {
            for (i = 0; i < (int)strlen(in) - c; i++)
                in[c + i] = in[c + i + 1];
            continue;
        }

        c++;
    }

    return in;
}
