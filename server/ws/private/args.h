#ifndef __KIVIJALKA_PRIVATE_ARGS_H__
#define __KIVIJALKA_PRIVATE_ARGS_H__

#ifdef __cplusplus
#error This header must not be included directly by C++ code
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "parse.h"

struct args {
	struct __attribute__((gcc_struct,packed)) {
		uint32_t width;
		uint32_t height;
	} display;
	struct __attribute__((gcc_struct,packed)) {
		char *path;
	} cache;
	struct __attribute__((gcc_struct,packed)) {
		char *path;
	} capture;
	struct __attribute__((gcc_struct,packed)) {
		char *path;
	} output;
	struct __attribute__((gcc_struct,packed)) {
		char     *addr;
		uint16_t  port;
	} server;
} __attribute__((gcc_struct,packed));

#define ARGS_INIT {\
	.display = {\
		.width = 0,\
		.height = 0\
	},\
	.cache = {\
		.path = NULL\
	},\
	.capture = {\
		.path = NULL\
	},\
	.output = {\
		.path = NULL\
	},\
	.server = {\
		.addr = NULL,\
		.port = 0\
	}\
}

__attribute__((always_inline))
static inline bool
_geo2d_arg (char     *str,
            uint32_t *x,
            uint32_t *y)
{
	unsigned int i = 0;
	uint32_t _x, _y;

	if (_parse_u32 (str, &i, &_x)) {
		if (str[i] == 'x') {
			++i;
			if (_parse_u32 (str, &i, &_y)) {
				if (str[i] == '\0') {
					*x = _x;
					*y = _y;
					return true;
				}
			}
		}
	}

	fprintf (stderr, "%s: invalid display geometry\n", __func__);

	return false;
}

	__attribute__((always_inline))
static inline bool
_args_parse (struct args  *a,
             int           argc,
             char        **argv)
{
	char *p;

	for (int i = 1; i < argc; ++i) {
		switch (*(p = argv[i])) {
			case '-':
				switch (*++p) {
				case 'a':
					if (*++p == '\0') {
						if (++i == argc) {
							return false;
						}
						a->server.addr = argv[i];
					} else {
						a->server.addr = p;
					}
					break;

				case 'b':
					if (*++p == '\0') {
						if (++i == argc) {
							return false;
						}
						a->cache.path = argv[i];
					} else {
						a->cache.path = p;
					}
					break;

				case 'c':
					if (*++p == '\0') {
						if (++i == argc) {
							return false;
						}
						a->capture.path = argv[i];
					} else {
						a->capture.path = p;
					}
					break;

				case 'g':
					if (*++p == '\0') {
						if (++i == argc) {
							return false;
						}
						p = argv[i];
					}
					if (!_geo2d_arg (p, &a->display.width,
					                 &a->display.height)) {
						return false;
					}
					break;

				case 'o':
					if (*++p == '\0') {
						if (++i == argc) {
							return false;
						}
						a->output.path = argv[i];
					} else {
						a->output.path = p;
					}
					break;

				case 'p':
					if (*++p == '\0') {
						if (++i == argc) {
							return false;
						}
						p = argv[i];
					}
					if (!_parse_u16 (p, NULL, &a->server.port)) {
						return false;
					}
					break;
				}
				break;

			default:
				return false;
		}
	}

	return true;
}

__attribute__((always_inline))
static inline uint32_t
_args_get_display_width (struct args *a)
{
	return a->display.width;
}

__attribute__((always_inline))
static inline uint32_t
_args_get_display_height (struct args *a)
{
	return a->display.height;
}

__attribute__((always_inline))
static inline const char *
_args_get_cache_path (struct args *a)
{
	return (a->cache.path)
	       ? (const char *) a->cache.path
	       : "/usr/share/kivijalka/banners";
}

__attribute__((always_inline))
static inline const char *
_args_get_capture_path (struct args *a)
{
	return (const char *) a->capture.path;
}

__attribute__((always_inline))
static inline const char *
_args_get_output_path (struct args *a)
{
	return (const char *) a->output.path;
}

__attribute__((always_inline))
static inline const char *
_args_get_server_addr (struct args *a)
{
	return (a->server.addr) ? (const char *) a->server.addr : "127.0.0.1";
}

__attribute__((always_inline))
static inline uint16_t
_args_get_server_port (struct args *a)
{
	return (a->server.port > 0) ? a->server.port : 8001;
}

#endif // __KIVIJALKA_PRIVATE_ARGS_H__
