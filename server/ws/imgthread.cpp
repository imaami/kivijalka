#include "imgthread.h"
#include "global.h"
#include "img_file.h"
#include "img.h"

#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <cstring>

ImgThread::ImgThread(QObject *parent) :
	QThread(parent)
{
}

ImgThread::~ImgThread()
{
}

void ImgThread::run()
{
	for (;;) {
		if (!sem_wait (&process_sem)) {
			img_data_t *capture_data, *banner_data;

			if (!img_file_steal_data (&capture_file,
			                          &capture_data)) {
				capture_data = NULL;
			}

			if (!img_file_steal_data (&banner_file,
			                          &banner_data)) {
				banner_data = NULL;
			}

			if (capture_data) {
				if (img_load_data (&img, 1,
				                   capture_data->data,
				                   capture_data->size)) {
					std::printf ("%s: loaded capture data\n", __func__);
				}

				img_data_free (capture_data);
				capture_data = NULL;

				if (banner_data) {
					goto have_banner_update;
				} else {
					goto do_render_update;
				}

			} else if (banner_data) {
			have_banner_update:
				if (img_load_data (&img, 2,
				                   banner_data->data,
				                   banner_data->size)) {
					std::printf ("%s: loaded banner data\n", __func__);
				}

				img_data_free (banner_data);
				banner_data = NULL;

			do_render_update:
				(void) img_file_post (&output_file);

			} else {
				std::printf ("%s: nothing to update\n", __func__);
			}

		} else if (errno != EINTR) {
			std::fprintf (stderr, "%s: sem_wait failed: %s\n",
			              __func__, std::strerror (errno));
		}
	}
}
