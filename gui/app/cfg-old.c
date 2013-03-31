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
 * @brief Translation of old settings or old configure options
 */

/**
 * @brief Convert a filename into the original byte sequence
 *        according to the encoding specified in G_FILENAME_ENCODING,
 *        if it's currently in UTF-8.
 *
 * @param fname filename
 *
 * @return converted filename
 */
static const gchar *cfg_old_filename_from_utf8(const gchar *fname)
{
#if defined(CFG_OLD_PLAYLIST) || defined(CFG_OLD_FILESELECT)
    static gchar *name;

    if (g_utf8_validate(fname, -1, NULL)) {
        free(name);
        name = g_filename_from_utf8(fname, -1, NULL, NULL, NULL);

        return name;
    }
#endif
    return fname;
}
