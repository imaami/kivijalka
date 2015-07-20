/** \file display.c
 *
 * Graphical display.
 */

#include "private/display.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

__attribute__((always_inline))
static inline bool
_display_init (struct display *d,
               const uint32_t  width,
               const uint32_t  height)
{
	size_t buf_bytes = (width * height) << 2;

	if (!(d->pixbuf = aligned_alloc (16, buf_bytes))) {
		fprintf (stderr, "%s: aligned_alloc failed\n", __func__);
		return false;
	}

	d->width = width;
	d->height = height;

	(void) memset ((void *) d->pixbuf, 0, buf_bytes);

	return true;
}

__attribute__((always_inline))
static inline void
_display_fini (struct display *d)
{
	if (d->pixbuf) {
		(void) memset ((void *) d->pixbuf, 0,
		               (d->width * d->height) << 2);
		free (d->pixbuf);
		d->pixbuf = NULL;
	}
	d->width = 0;
	d->height = 0;
}

struct display *
display_create (const uint32_t width,
                const uint32_t height)
{
	if (width < 1 || height < 1 || (SIZE_MAX / 4) / width < height) {
		fprintf (stderr, "%s: invalid arguments\n", __func__);
		return NULL;
	}

	struct display *d;

	if (!(d = aligned_alloc (16, sizeof (struct display)))) {
		fprintf (stderr, "%s: aligned_alloc failed\n", __func__);
		return NULL;
	}

	if (!_display_init (d, width, height)) {
		fprintf (stderr, "%s: _display_init failed\n", __func__);
		free (d);
		d = NULL;
	} else {
		printf ("%s: display width=%u, height=%u\n",
		        __func__, d->width, d->height);
	}

	return d;
}

void
display_destroy (struct display *d)
{
	if (d) {
		_display_fini (d);
		free (d);
		d = NULL;
	}
}

uint32_t
display_width (struct display *d)
{
	return (d) ? d->width : 0;
}

uint32_t
display_height (struct display *d)
{
	return (d) ? d->height : 0;
}

uint32_t *
display_pixbuf (struct display *d)
{
	return (d) ? d->pixbuf : NULL;
}


__attribute__((always_inline))
static inline void
_display_render_bg (uint32_t *dst,
                    uint32_t  w,
                    uint32_t  h,
                    uint32_t *bg_data)
{
	size_t n = w * h;
	uint32_t *d = dst, *b = bg_data;
	for (size_t i = 0; i < n; ++i) {
		*d++ = *b++;
	}
}

bool
display_render_bg (struct display *d,
                   uint32_t       *bg_data)
{
	if (d && bg_data) {
		_display_render_bg (d->pixbuf, d->width, d->height, bg_data);
		return true;
	}
	fprintf (stderr, "%s: invalid arguments\n", __func__);
	return false;
}

typedef union {
	uint32_t u32;
	struct {
		uint8_t b: 8;
		uint8_t g: 8;
		uint8_t r: 8;
		uint8_t a: 8;
	} __attribute__((gcc_struct,packed));
} argb_t;

__attribute__((always_inline))
static inline void
_display_render (uint32_t *dst,
                 uint32_t  w,
                 uint32_t  h,
                 uint32_t *bg_data,
                 uint32_t *ol_data,
                 uint32_t  ol_w,
                 uint32_t  ol_h,
                 uint32_t  ol_x,
                 uint32_t  ol_y)
{
	size_t x, y, ow = ol_x + ol_w, oh = ol_y + ol_h;

	// vertical section above overlay
	for (y = 0; y < ol_y; ++y) {
		for (x = 0; x < w; ++x) {
			dst[y * w + x] = bg_data[y * w + x];
		}
	}

	// vertical section containing overlay
	for (uint32_t *_o = ol_data; y < oh; ++y) {
		// horizontal section at left side of overlay
		for (x = 0; x < ol_x; ++x) {
			dst[y * w + x] = bg_data[y * w + x];
		}

		// overlay
		for (; x < ow; ++x, ++_o) {
			// A over B premultiplied pixel sum
			argb_t a = { .u32 = *_o },
			       b = { .u32 = bg_data[y * w + x] };
			float f = 1.0f - (a.a / 255.0f);
			argb_t c = {
				.a = 255,
				.r = a.r + (uint8_t) (b.r * f),
				.g = a.g + (uint8_t) (b.g * f),
				.b = a.b + (uint8_t) (b.b * f)
			};
			dst[y * w + x] = c.u32;
		}

		// horizontal section at right side of overlay
		for (; x < w; ++x) {
			dst[y * w + x] = bg_data[y * w + x];
		}
	}

	// vertical section below overlay
	for (; y < h; ++y) {
		for (x = 0; x < w; ++x) {
			dst[y * w + x] = bg_data[y * w + x];
		}
	}
}

bool
display_render (struct display *d,
                uint32_t       *bg_data,
                uint32_t       *ol_data,
                uint32_t        ol_w,
                uint32_t        ol_h,
                uint32_t        ol_x,
                uint32_t        ol_y)
{
	if (d && bg_data && ol_data) {
		_display_render (d->pixbuf, d->width, d->height, bg_data,
		                 ol_data, ol_w, ol_h, ol_x, ol_y);
		return true;
	}
	fprintf (stderr, "%s: invalid arguments\n", __func__);
	return false;
}
