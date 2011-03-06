/*
 * Strings utilities
 *
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
#include <stdarg.h>
#include <stdio.h>

#include "mp_strings.h"

char *mp_asprintf(const char *fmt, ...)
{
    char *p = NULL;
    va_list va, va_bak;
    int len;

    va_start(va, fmt);
    va_copy(va_bak, va);

    len = vsnprintf(NULL, 0, fmt, va);
    if (len < 0)
        goto end;

    p = malloc(len + 1);
    if (!p)
        goto end;

    len = vsnprintf(p, len + 1, fmt, va_bak);
    if (len < 0)
        free(p), p = NULL;

end:
    va_end(va);
    return p;
}
