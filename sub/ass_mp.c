/*
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
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

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "mp_msg.h"
#include "mpcommon.h"
#include "path.h"
#include "subreader.h"

#include "ass_mp.h"
#include "eosd.h"
#include "mpcommon.h"
#include "sub.h"
#include "help_mp.h"
#include "font_load.h"
#include "stream/stream.h"

#ifdef CONFIG_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

// libass-related command line options
ASS_Library* ass_library;
int ass_enabled = 0;
float ass_font_scale = 1.;
float ass_line_spacing = 0.;
int ass_top_margin = 0;
int ass_bottom_margin = 0;
int extract_embedded_fonts = 1;
char **ass_force_style_list = NULL;
int ass_use_margins = 0;
char* ass_color = NULL;
char* ass_border_color = NULL;
char* ass_styles_file = NULL;
int ass_hinting = ASS_HINTING_NATIVE + 4; // native hinting for unscaled osd

static void init_style(ASS_Style *style, const char *name, double playres)
{
	double fs;
	uint32_t c1, c2;
	style->Name = strdup(name);
	style->FontName = (font_fontconfig >= 0 && sub_font_name) ? strdup(sub_font_name) : (font_fontconfig >= 0 && font_name) ? strdup(font_name) : strdup("Sans");
	style->treat_fontname_as_pattern = 1;

	fs = playres * text_font_scale_factor / 100.;
	// approximate autoscale coefficients
	if (subtitle_autoscale == 2)
		fs *= 1.3;
	else if (subtitle_autoscale == 3)
		fs *= 1.4;
	style->FontSize = fs;

	if (ass_color) c1 = strtoll(ass_color, NULL, 16);
	else c1 = 0xFFFF0000;
	if (ass_border_color) c2 = strtoll(ass_border_color, NULL, 16);
	else c2 = 0x00000000;

	style->PrimaryColour = c1;
	style->SecondaryColour = c1;
	style->OutlineColour = c2;
	style->BackColour = 0x00000000;
	style->BorderStyle = 1;
	style->Alignment = 2;
	style->Outline = 2;
	style->MarginL = 10;
	style->MarginR = 10;
	style->MarginV = 5;
	style->ScaleX = 1.;
	style->ScaleY = 1.;
}

ASS_Track* ass_default_track(ASS_Library* library) {
	ASS_Track* track = ass_new_track(library);

	track->track_type = TRACK_TYPE_ASS;
	track->Timer = 100.;
	track->PlayResY = 288;
	track->WrapStyle = 0;

	if (track->n_styles == 0) {
		// stupid hack to stop libass to add a default track
		// in front in ass_read_styles - this makes it impossible
		// to completely override the "Default" track.
		int sid = ass_alloc_style(track);
		init_style(track->styles + sid, "MPlayerDummy", track->PlayResY);
	}

	if (ass_styles_file)
		ass_read_styles(track, ass_styles_file, sub_cp);

	if (track->default_style <= 0) {
		int sid = ass_alloc_style(track);
		init_style(track->styles + sid, "Default", track->PlayResY);
		track->default_style = sid;
	}

	ass_process_force_style(track);
	return track;
}

static int check_duplicate_plaintext_event(ASS_Track* track)
{
	int i;
	ASS_Event* evt = track->events + track->n_events - 1;

	for (i = 0; i<track->n_events - 1; ++i) // ignoring last event, it is the one we are comparing with
		if (track->events[i].Start == evt->Start &&
		    track->events[i].Duration == evt->Duration &&
		    strcmp(track->events[i].Text, evt->Text) == 0)
			return 1;
	return 0;
}

/**
 * \brief Convert subtitle to ASS_Event for the given track
 * \param track ASS_Track
 * \param sub subtitle to convert
 * \return event id
 * note: assumes that subtitle is _not_ fps-based; caller must manually correct
 *   Start and Duration in other case.
 **/
int ass_process_subtitle(ASS_Track* track, subtitle* sub)
{
        int eid;
        ASS_Event* event;
	int len = 0, j;
	char* p;
	char* end;

	eid = ass_alloc_event(track);
	event = track->events + eid;

	event->Start = sub->start * 10;
	event->Duration = (sub->end - sub->start) * 10;
	event->Style = track->default_style;

	for (j = 0; j < sub->lines; ++j)
		len += sub->text[j] ? strlen(sub->text[j]) : 0;

	len += 2 * sub->lines; // '\N', including the one after the last line
	len += 6; // {\anX}
	len += 1; // '\0'

	event->Text = malloc(len);
	end = event->Text + len;
	p = event->Text;

	if (sub->alignment)
		p += snprintf(p, end - p, "{\\an%d}", sub->alignment);

	for (j = 0; j < sub->lines; ++j)
		p += snprintf(p, end - p, "%s\\N", sub->text[j]);

	if (sub->lines > 0) p-=2; // remove last "\N"
	*p = 0;

	if (check_duplicate_plaintext_event(track)) {
		ass_free_event(track, eid);
		track->n_events--;
		return -1;
	}

	mp_msg(MSGT_ASS, MSGL_V, "plaintext event at %" PRId64 ", +%" PRId64 ": %s  \n",
			(int64_t)event->Start, (int64_t)event->Duration, event->Text);

	return eid;
}


/**
 * \brief Convert subdata to ASS_Track
 * \param subdata subtitles struct from subreader
 * \param fps video framerate
 * \return newly allocated ASS_Track, filled with subtitles from subdata
 */
ASS_Track* ass_read_subdata(ASS_Library* library, sub_data* subdata, double fps) {
	ASS_Track* track;
	int i;

	track = ass_default_track(library);
	track->name = subdata->filename ? strdup(subdata->filename) : 0;

	for (i = 0; i < subdata->sub_num; ++i) {
		int eid = ass_process_subtitle(track, subdata->subtitles + i);
		if (eid < 0)
			continue;
		if (!subdata->sub_uses_time) {
			track->events[eid].Start *= 100. / fps;
			track->events[eid].Duration *= 100. / fps;
		}
	}
	return track;
}

ASS_Track* ass_read_stream(ASS_Library* library, const char *fname, char *charset) {
	char *buf = NULL;
	ASS_Track *track;
	size_t sz = 0;
	size_t buf_alloc = 0;
	stream_t *fd;

	fd = open_stream(fname, NULL, NULL);
	if (!fd) {
		mp_msg(MSGT_ASS, MSGL_WARN, MSGTR_LIBASS_FopenFailed, fname);
		return NULL;
	}
	if (fd->end_pos > STREAM_BUFFER_SIZE)
		/* read entire file if size is known */
		buf_alloc = fd->end_pos;
	for (;;) {
		int i;
		if (buf_alloc >= 100*1024*1024) {
			mp_msg(MSGT_ASS, MSGL_INFO, MSGTR_LIBASS_RefusingToLoadSubtitlesLargerThan100M, fname);
			sz = 0;
			break;
		}
		if (buf_alloc < sz + STREAM_BUFFER_SIZE)
			buf_alloc += STREAM_BUFFER_SIZE;
		buf = realloc(buf, buf_alloc + 1);
		i = stream_read(fd, buf + sz, buf_alloc - sz);
		if (i <= 0) break;
		sz += i;
	}
	free_stream(fd);
	if (!sz) {
		free(buf);
		return NULL;
	}
	buf[sz] = 0;
	buf = realloc(buf, sz + 1);
	track = ass_read_memory(library, buf, sz, charset);
	if (track) {
		free(track->name);
		track->name = strdup(fname);
	}
	free(buf);
	return track;
}

void ass_configure(ASS_Renderer* priv, int w, int h, int unscaled) {
	int hinting;
	ass_set_frame_size(priv, w, h);
	ass_set_margins(priv, ass_top_margin, ass_bottom_margin, 0, 0);
	ass_set_use_margins(priv, ass_use_margins);
	ass_set_font_scale(priv, ass_font_scale);
	if (!unscaled && (ass_hinting & 4))
		hinting = 0;
	else
		hinting = ass_hinting & 3;
	ass_set_hinting(priv, hinting);
	ass_set_line_spacing(priv, ass_line_spacing);
}

void ass_configure_fonts(ASS_Renderer* priv) {
	char *dir, *path, *family;
	dir = get_path("fonts");
	if (font_fontconfig < 0 && sub_font_name) path = strdup(sub_font_name);
	else if (font_fontconfig < 0 && font_name) path = strdup(font_name);
	else path = get_path("subfont.ttf");
	if (font_fontconfig >= 0 && sub_font_name) family = strdup(sub_font_name);
	else if (font_fontconfig >= 0 && font_name) family = strdup(font_name);
	else family = 0;

        ass_set_fonts(priv, path, family, font_fontconfig, NULL, 1);

	free(dir);
	free(path);
	free(family);
}

static void message_callback(int level, const char *format, va_list va, void *ctx)
{
	int n;
	char *str;
	va_list dst;

	va_copy(dst, va);
	n = vsnprintf(NULL, 0, format, va);
	if (n > 0 && (str = malloc(n + 1))) {
		vsnprintf(str, n + 1, format, dst);
		mp_msg(MSGT_ASS, level, "[ass] %s\n", str);
		free(str);
	}
}

/**
 * Reset all per-file settings for next file.
 */
void ass_mp_reset_config(ASS_Library *l) {
	ass_set_extract_fonts(l, extract_embedded_fonts);
	ass_set_style_overrides(l, ass_force_style_list);
	ass_force_reload = 1;
}

ASS_Library* ass_init(void) {
	ASS_Library* priv;
	char* path = get_path("fonts");
	priv = ass_library_init();
	ass_set_message_cb(priv, message_callback, NULL);
	ass_set_fonts_dir(priv, path);
	ass_mp_reset_config(priv);
	free(path);
	return priv;
}

int ass_force_reload = 0; // flag set if global ass-related settings were changed

ASS_Image* ass_mp_render_frame(ASS_Renderer *priv, ASS_Track* track, long long now, int* detect_change) {
	if (ass_force_reload) {
		ass_set_margins(priv, ass_top_margin, ass_bottom_margin, 0, 0);
		ass_set_use_margins(priv, ass_use_margins);
		ass_set_font_scale(priv, ass_font_scale);
		ass_force_reload = 0;
	}
	return ass_render_frame(priv, track, now, detect_change);
}

/* EOSD source for ASS subtitles. */

static ASS_Renderer *ass_renderer;
static int prev_visibility;

static void eosd_ass_update(struct mp_eosd_source *src, const struct mp_eosd_settings *res, double ts)
{
	long long ts_ms = (ts + sub_delay) * 1000 + .5;
	ASS_Image *aimg;
	struct mp_eosd_image *img;
	if (res->changed || !src->initialized) {
		double dar = (double) (res->w - res->ml - res->mr) / (res->h - res->mt - res->mb);
		ass_configure(ass_renderer, res->w, res->h, res->unscaled);
		ass_set_margins(ass_renderer, res->mt, res->mb, res->ml, res->mr);
		ass_set_aspect_ratio(ass_renderer, dar, (double)res->srcw / res->srch);
		src->initialized = 1;
	}
	aimg = sub_visibility && ass_track && ts != MP_NOPTS_VALUE ?
		ass_mp_render_frame(ass_renderer, ass_track, ts_ms, &src->changed) :
		NULL;
	if (!aimg != !src->images)
		src->changed = 2;
	if (src->changed) {
		eosd_image_remove_all(src);
		while (aimg) {
			img = eosd_image_alloc();
			img->w      = aimg->w;
			img->h      = aimg->h;
			img->bitmap = aimg->bitmap;
			img->stride = aimg->stride;
			img->color  = aimg->color;
			img->dst_x  = aimg->dst_x;
			img->dst_y  = aimg->dst_y;
			eosd_image_append(src, img);
			aimg = aimg->next;
		}
	}
	prev_visibility = sub_visibility;
}

static void eosd_ass_uninit(struct mp_eosd_source *src)
{
	eosd_image_remove_all(src);
	ass_renderer_done(ass_renderer);
}

static struct mp_eosd_source eosd_ass = {
	.uninit  = eosd_ass_uninit,
	.update  = eosd_ass_update,
	.z_index = 10,
};

void eosd_ass_init(ASS_Library *ass_library)
{
	ass_renderer = ass_renderer_init(ass_library);
	if (!ass_renderer)
		return;
	ass_configure_fonts(ass_renderer);
	if (!eosd_registered(&eosd_ass))
		eosd_register(&eosd_ass);
}
