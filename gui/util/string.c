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

#include "string.h"

char *strlower(char *in)
{
    char *p = in;

    while (*p) {
        if (*p >= 'A' && *p <= 'Z')
            *p += 'a' - 'A';

        p++;
    }

    return in;
}

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
    char *src, *dest;
    int freeze = 0;

    src = dest = in;

    while (*src) {
        if (*src == '"')
            freeze = !freeze;

        if (freeze || (*src != ' '))
            *dest++ = *src;

        src++;
    }

    *dest = 0;

    return in;
}
