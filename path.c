/*
 * Get path to config dir/file.
 *
 * Return Values:
 *   Returns the pointer to the ALLOCATED buffer containing the
 *   zero terminated path string. This buffer has to be FREED
 *   by the caller.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "mp_msg.h"
#include "path.h"

#ifdef CONFIG_MACOSX_BUNDLE
#include <CoreFoundation/CoreFoundation.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#elif defined(__MINGW32__)
#include <windows.h>
#elif defined(__CYGWIN__)
#include <windows.h>
#include <sys/cygwin.h>
#endif

#include "osdep/osdep.h"

char *get_path(const char *filename){
	// temporary buffer that will be freed
	char *tmp = 0;
	char *homedir;
	char *buff;
#ifdef __MINGW32__
	const char *config_dir = "/mplayer";
#else
	const char *config_dir = "/.mplayer";
#endif
	int len;
#ifdef CONFIG_MACOSX_BUNDLE
	struct stat dummy;
	CFIndex maxlen=256;
	CFURLRef res_url_ref=NULL;
	CFURLRef bdl_url_ref=NULL;
	char *res_url_path = NULL;
	char *bdl_url_path = NULL;
#endif

	if ((homedir = getenv("MPLAYER_HOME")) != NULL)
		config_dir = "";
	else if ((homedir = getenv("HOME")) == NULL)
	{
#if !defined(__MINGW32__) && !defined(__CYGWIN__) && !defined(__OS2__)
		return NULL;
#else
		int i;
		char path[260];
#ifdef __OS2__
		PPIB ppib;
		// Get process info blocks
		DosGetInfoBlocks(NULL, &ppib);
		// Get full path of the executable
		DosQueryModuleName(ppib->pib_hmte, sizeof( path ), path);
#else
		GetModuleFileNameA(NULL, path, 260);
#endif
		// Extract directory part
		tmp = homedir = mp_dirname(path);
		// Convert backslashes to slashes
		for (i = 0; homedir[i]; i++)
			if (homedir[i] == '\\') homedir[i] = '/';
		// If there is a trailing slash remove it.
		// If there isn't, remove the one from config dir (if homedir
		// ends up e.g. c:)
		if (i && homedir[i-1] == '/') homedir[i-1] = 0;
		else config_dir++;
#endif
	}
	len = strlen(homedir) + strlen(config_dir) + 1;
	if (filename == NULL) {
		if ((buff = malloc(len)) == NULL)
			return NULL;
		sprintf(buff, "%s%s", homedir, config_dir);
	} else {
		len += strlen(filename) + 1;
		if ((buff = malloc(len)) == NULL)
			return NULL;
		sprintf(buff, "%s%s/%s", homedir, config_dir, filename);
	}

#ifdef CONFIG_MACOSX_BUNDLE
	if (stat(buff, &dummy)) {

		res_url_ref=CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
		bdl_url_ref=CFBundleCopyBundleURL(CFBundleGetMainBundle());

		if (res_url_ref&&bdl_url_ref) {

			res_url_path=malloc(maxlen);
			bdl_url_path=malloc(maxlen);

			while (!CFURLGetFileSystemRepresentation(res_url_ref, true, res_url_path, maxlen)) {
				maxlen*=2;
				res_url_path=realloc(res_url_path, maxlen);
			}
			CFRelease(res_url_ref);

			while (!CFURLGetFileSystemRepresentation(bdl_url_ref, true, bdl_url_path, maxlen)) {
				maxlen*=2;
				bdl_url_path=realloc(bdl_url_path, maxlen);
			}
			CFRelease(bdl_url_ref);

			if (strcmp(res_url_path, bdl_url_path) == 0)
				res_url_path = NULL;
		}

		if (res_url_path&&filename) {
			if ((strlen(filename)+strlen(res_url_path)+2)>maxlen) {
				maxlen=strlen(filename)+strlen(res_url_path)+2;
			}
			free(buff);
			buff = malloc(maxlen);
			strcpy(buff, res_url_path);

			strcat(buff,"/");
			strcat(buff, filename);
		}
	}
#endif
	free(tmp);
	mp_msg(MSGT_GLOBAL,MSGL_V,"get_path('%s') -> '%s'\n",filename,buff);
	return buff;
}

#if (defined(__MINGW32__) || defined(__CYGWIN__)) && defined(CONFIG_WIN32DLL)
void set_path_env(void)
{
	/*make our codec dirs available for LoadLibraryA()*/
	char win32path[MAX_PATH];
#ifdef __CYGWIN__
	cygwin_conv_to_full_win32_path(BINARY_CODECS_PATH, win32path);
#else /*__CYGWIN__*/
	/* Expand to absolute path unless it's already absolute */
	if (!strstr(BINARY_CODECS_PATH,":") && BINARY_CODECS_PATH[0] != '\\') {
		GetModuleFileNameA(NULL, win32path, MAX_PATH);
		strcpy(strrchr(win32path, '\\') + 1, BINARY_CODECS_PATH);
	}
	else strcpy(win32path, BINARY_CODECS_PATH);
#endif /*__CYGWIN__*/
	mp_msg(MSGT_WIN32, MSGL_V, "Setting PATH to %s\n", win32path);
	if (!SetEnvironmentVariableA("PATH", win32path))
		mp_msg(MSGT_WIN32, MSGL_WARN, "Cannot set PATH!");
}
#endif /* (defined(__MINGW32__) || defined(__CYGWIN__)) && defined(CONFIG_WIN32DLL) */

char *codec_path = BINARY_CODECS_PATH;

static int needs_free = 0;

void set_codec_path(const char *path)
{
    if (needs_free)
        free(codec_path);
    if (path == 0) {
        codec_path = BINARY_CODECS_PATH;
        needs_free = 0;
        return;
    }
    codec_path = malloc(strlen(path) + 1);
    strcpy(codec_path, path);
    needs_free = 1;
}

/**
 * @brief Returns the basename substring of a path.
 */
const char *mp_basename(const char *path)
{
    char *s;

#if HAVE_DOS_PATHS
    s = strrchr(path, '\\');
    if (s)
        path = s + 1;
    s = strrchr(path, ':');
    if (s)
        path = s + 1;
#endif
    s = strrchr(path, '/');
    return s ? s + 1 : path;
}

/**
 * @brief Allocates a new buffer containing the directory name
 * @param path Original path. Must be a valid string.
 *
 * @note The path returned always contains a trailing slash '/'.
 *       On systems supporting DOS paths, '\' is also considered as a directory
 *       separator in addition to the '/'.
 */
char *mp_dirname(const char *path)
{
    const char *base = mp_basename(path);
    size_t len = base - path;
    char *dirname;

    if (len == 0)
        return strdup("./");
    dirname = malloc(len + 1);
    if (!dirname)
        return NULL;
    strncpy(dirname, path, len);
    dirname[len] = '\0';
    return dirname;
}

/**
 * @brief Join two paths if path is not absolute.
 * @param base File or directory base path.
 * @param path Path to concatenate with the base.
 * @return New allocated string with the path, or NULL in case of error.
 * @warning Do not forget the trailing path separator at the end of the base
 *          path if it is a directory: since file paths are also supported,
 *          this separator will make the distinction.
 * @note Paths of the form c:foo, /foo or \foo will still depends on the
 *       current directory on Windows systems, even though they are considered
 *       as absolute paths in this function.
 */
char *mp_path_join(const char *base, const char *path)
{
    char *ret, *tmp;

#if HAVE_DOS_PATHS
    if ((path[0] && path[1] == ':') || path[0] == '\\' || path[0] == '/')
#else
    if (path[0] == '/')
#endif
        return strdup(path);

    ret = mp_dirname(base);
    if (!ret)
        return NULL;
    tmp = realloc(ret, strlen(ret) + strlen(path) + 1);
    if (!tmp) {
        free(ret);
        return NULL;
    }
    ret = tmp;
    strcat(ret, path);
    return ret;
}

/**
 * @brief Same as mp_path_join but always treat the first parameter as a
 *        directory.
 * @param dir Directory base path.
 * @param append Right part to append to dir.
 * @return New allocated string with the path, or NULL in case of error.
 */
char *mp_dir_join(const char *dir, const char *append)
{
    char *tmp, *ret;
    size_t dirlen = strlen(dir);
    size_t i      = dirlen - 1;

#if HAVE_DOS_PATHS
    if ((dirlen == 2 && dir[0] && dir[1] == ':') // "X:" only
        || dirlen == 0 || dir[i] == '\\' || dir[i] == '/')
#else
    if (dirlen == 0 || dir[i] == '/')
#endif
        return mp_path_join(dir, append);

    tmp = malloc(dirlen + 2);
    if (!tmp)
        return NULL;
    strcpy(tmp, dir);
    strcpy(tmp + dirlen, "/");
    ret = mp_path_join(tmp, append);
    free(tmp);
    return ret;
}
