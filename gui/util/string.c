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

#include "string.h"
#include "gui/interface.h"

#include "config.h"
#include "help_mp.h"
#include "libavutil/avstring.h"
#include "stream/stream.h"

/**
 * @brief Convert a string to lower case.
 *
 * @param string to be converted
 *
 * @return converted string
 *
 * @note Only characters from A to Z will be converted and this is an in-place conversion.
 */
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

/**
 * @brief Swap characters in a string.
 *
 * @param in string to be processed
 * @param from character to be swapped
 * @param to character to swap in
 *
 * @return processed string
 *
 * @note All occurrences will be swapped and this is an in-place processing.
 */
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

/**
 * @brief Remove all space characters from a string,
 *        but leave text enclosed in quotation marks untouched.
 *
 * @param in string to be processed
 *
 * @return processed string
 *
 * @note This is an in-place processing.
 */
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

/**
 * @brief Remove a comment from a string,
 *        but leave text enclosed in quotation marks untouched.
 *
 *        A comment starts either with a semicolon anywhere in the string
 *        or with a number sign character at the very beginning.
 *
 * @param in string to be processed
 *
 * @return string without comment
 *
 * @note This is an in-place processing, i.e. @a in will be shortened.
 */
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

/**
 * @brief A strncmp() that can handle NULL pointers.
 *
 * @param a string to be compared
 * @param b string which is compared
 * @param n number of characters compared at the most
 *
 * @return return value of strncmp() or -1, if a or b are NULL
 */
int gstrncmp(const char *a, const char *b, size_t n)
{
    if (!a && !b)
        return 0;
    if (!a || !b)
        return -1;

    return strncmp(a, b, n);
}

/**
 * @brief Duplicate a string.
 *
 *        If @a str is NULL, it returns NULL.
 *        The string is duplicated by calling strdup().
 *
 * @param str string to be duplicated
 *
 * @return duplicated string (newly allocated)
 */
char *gstrdup(const char *str)
{
    if (!str)
        return NULL;

    return strdup(str);
}

/**
 * @brief Assign a duplicated string.
 *
 *        The string is duplicated by calling #gstrdup().
 *
 * @note @a *old is freed prior to the assignment.
 *
 * @param old pointer to a variable suitable to store the new pointer
 * @param str string to be duplicated
 */
void setdup(char **old, const char *str)
{
    free(*old);
    *old = gstrdup(str);
}

/**
 * @brief Assign a newly allocated string
 *        containing the path created from a directory and a filename.
 *
 * @note @a *old is freed prior to the assignment.
 *
 * @param old pointer to a variable suitable to store the new pointer
 * @param dir directory
 * @param name filename
 */
void setddup(char **old, const char *dir, const char *name)
{
    free(*old);
    *old = malloc(strlen(dir) + strlen(name) + 2);
    if (*old)
        sprintf(*old, "%s/%s", dir, name);
}

/**
 * @brief Convert #guiInfo member Filename.
 *
 * @param how 0 (cut file path and extension),
 *            1 (additionally, convert lower case) or
 *            2 (additionally, convert upper case)
 * @param fname pointer to a buffer to receive the converted Filename
 * @param maxlen size of @a fname buffer
 *
 * @return pointer to @a fname buffer
 */
char *TranslateFilename(int how, char *fname, size_t maxlen)
{
    char *p;
    size_t len;

    switch (guiInfo.StreamType) {
    case STREAMTYPE_FILE:
        if (guiInfo.Filename && *guiInfo.Filename) {
            p = strrchr(guiInfo.Filename,
#if HAVE_DOS_PATHS
                        '\\');
#else
                        '/');
#endif

            if (p)
                av_strlcpy(fname, p + 1, maxlen);
            else
                av_strlcpy(fname, guiInfo.Filename, maxlen);

            len = strlen(fname);

            if (len > 3 && fname[len - 3] == '.')
                fname[len - 3] = 0;
            else if (len > 4 && fname[len - 4] == '.')
                fname[len - 4] = 0;
            else if (len > 5 && fname[len - 5] == '.')
                fname[len - 5] = 0;
        } else
            av_strlcpy(fname, MSGTR_NoFileLoaded, maxlen);
        break;

    case STREAMTYPE_STREAM:
        av_strlcpy(fname, guiInfo.Filename, maxlen);
        break;

    case STREAMTYPE_CDDA:
        snprintf(fname, maxlen, MSGTR_Title, guiInfo.Track);
        break;

    case STREAMTYPE_VCD:
        snprintf(fname, maxlen, MSGTR_Title, guiInfo.Track - 1);
        break;

    case STREAMTYPE_DVD:
        if (guiInfo.Chapter)
            snprintf(fname, maxlen, MSGTR_Chapter, guiInfo.Chapter);
        else
            av_strlcat(fname, MSGTR_NoChapter, maxlen);
        break;

    default:
        av_strlcpy(fname, MSGTR_NoMediaOpened, maxlen);
        break;
    }

    if (how) {
        p = fname;

        while (*p) {
            char t = 0;

            if (how == 1 && *p >= 'A' && *p <= 'Z')
                t = 32;
            if (how == 2 && *p >= 'a' && *p <= 'z')
                t = -32;

            *p = *p + t;
            p++;
        }
    }

    return fname;
}

/**
 * @brief Read characters from @a file.
 *
 * @note Reading stops with an end-of-line character or at end of file.
 *
 * @param str pointer to a buffer to receive the read characters
 * @param size number of characters read at the most (including a terminating null-character)
 * @param file file to read from
 *
 * @return str (success) or NULL (error)
 */
char *fgetstr(char *str, int size, FILE *file)
{
    char *s;

    s = fgets(str, size, file);

    if (s)
        s[strcspn(s, "\n\r")] = 0;

    return s;
}
