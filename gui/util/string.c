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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

char *decomment(char *in)
{
    char *p;
    int nap = 0;

    p = in;

    if (*p == '#')
        *p = 0;

    while (*p) {
        if (*p == '"')
            nap = !nap;

        if ((*p == ';') && !nap) {
            *p = 0;
            break;
        }

        p++;
    }

    return in;
}

char *gstrchr(const char *str, int c)
{
    if (!str)
        return NULL;

    return strchr(str, c);
}

int gstrcmp(const char *a, const char *b)
{
    if (!a && !b)
        return 0;
    if (!a || !b)
        return -1;

    return strcmp(a, b);
}

int gstrncmp(const char *a, const char *b, int n)
{
    if (!a && !b)
        return 0;
    if (!a || !b)
        return -1;

    return strncmp(a, b, n);
}

char *gstrdup(const char *str)
{
    if (!str)
        return NULL;

    return strdup(str);
}

void setdup(char **old, const char *str)
{
    free(*old);
    *old = gstrdup(str);
}

void setddup(char **old, const char *dir, const char *name)
{
    free(*old);
    *old = malloc(strlen(dir) + strlen(name) + 2);
    if (*old)
        sprintf(*old, "%s/%s", dir, name);
}
