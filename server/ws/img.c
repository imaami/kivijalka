/** \file img.c
 *
 * Image manipulation.
 */

#define _GNU_SOURCE

#include "img.h"

#include <stdio.h>
#include <wand/MagickWand.h>

#define img_exception(_w) { \
	char *_d; \
	ExceptionType _s; \
	_d = MagickGetException (_w, &_s); \
	(void) fprintf (stderr, "%s %s %lu %s\n", \
	                GetMagickModule(), _d); \
	_d = (char *) MagickRelinquishMemory(_d); \
}

bool
img_init (img_t *im)
{
	MagickWandGenesis();

	if (im) {
		im->screen = (void *) NewMagickWand();
		im->banner = (void *) NewMagickWand();
		return true;
	}

	return false;
}

bool
img_load_screen (img_t      *im,
                 const char *path)
{
	ClearMagickWand ((MagickWand *) im->screen);

	if (MagickReadImage ((MagickWand *) im->screen, path)
	    == MagickFalse) {
		img_exception ((MagickWand *) im->screen);
		return false;
	}

	return true;
}

size_t
img_get_screen_width (img_t *im)
{
	return (im) ? MagickGetImageWidth (im->screen) : 0;
}

size_t
img_get_screen_height (img_t *im)
{
	return (im) ? MagickGetImageHeight (im->screen) : 0;
}

bool
img_load_banner (img_t         *im,
                 const uint8_t *data,
                 const size_t   size)
{
	ClearMagickWand ((MagickWand *) im->banner);

	if (MagickReadImageBlob ((MagickWand *) im->banner,
	                         (const void *) data,
	                         size)
	    == MagickFalse) {
		img_exception ((MagickWand *) im->banner);
		return false;
	}

	return true;
}

bool
img_render_thumb (img_t         *im,
                  const ssize_t  banner_x,
                  const ssize_t  banner_y,
                  const size_t   thumb_w,
                  const size_t   thumb_h)
{
	if (im) {
		if (MagickCompositeImage ((MagickWand *) im->screen,
		                          (MagickWand *) im->banner,
		                          OverCompositeOp,
		                          banner_x, banner_y) == MagickTrue
		    && MagickScaleImage ((MagickWand *) im->screen,
		                         thumb_w, thumb_h) == MagickTrue) {
			return true;
		}

		img_exception ((MagickWand *) im->screen);
	}

	return false;
}

bool
img_write (img_t      *im,
           const char *file)
{
	if (!im) {
		return false;
	}

	if (MagickWriteImage ((MagickWand *) im->screen, file) == MagickFalse) {
		img_exception ((MagickWand *) im->screen);
		return false;
	}

	return true;
}

void
img_destroy (img_t *im)
{
	if (im) {
		if (im->banner) {
			im->banner = (void *) DestroyMagickWand((MagickWand *) im->banner);
		}
		im->banner = NULL;

		if (im->screen) {
			im->screen = (void *) DestroyMagickWand((MagickWand *) im->screen);
		}
		im->screen = NULL;
	}

	MagickWandTerminus();
}
