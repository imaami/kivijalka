#ifndef __KIVIJALKA_PRIVATE_BANNER_CACHE_H__
#define __KIVIJALKA_PRIVATE_BANNER_CACHE_H__

#ifdef __cplusplus
#error This header must not be included directly by C++ code
#endif

#include "../banner_cache.h"
#include "../list.h"
#include "sha1.h"
#include "banner.h"
#include "json.h"
#include "hex.h"
#include "img.h"
#include "mem.h"
#include "parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <inttypes.h>

#ifndef _DIRENT_HAVE_D_TYPE
#error The dirent structure does not have a d_type field on this platform
#endif

struct bucket {
	list_head_t hook;
	list_head_t list;
} __attribute__((gcc_struct,packed));

struct table {
	list_head_t    in_use;
	struct bucket *lookup_table;
} __attribute__((gcc_struct,packed));

struct banner_cache {
	char          *root_path;
	struct table   by_uuid;
	struct table   by_hash;
	struct bucket *data;
} __attribute__((gcc_struct,packed));

__attribute__((always_inline))
static inline bool
_textfile_read (const char *path,
                uint8_t    *dest,
                size_t      size,
                size_t     *len)
{
	bool r;
	file_t *f;
	size_t l;

	printf ("%s: path=%s\n", __func__, path);

	if (!(f = file_create (path))) {
		fprintf (stderr, "%s: file_create failed\n", __func__);
		return false;
	}

	if (!file_open (f, "r")) {
		fprintf (stderr, "%s: file_open failed\n", __func__);
		r = false;
		goto destroy_file;
	}

	if (!file_read (f, size, dest, &l)) {
		fprintf (stderr, "%s: file_read failed\n", __func__);
		r = false;
		goto close_file;
	}

	dest[l] = '\0';
	*len = l;
	r = true;

	printf ("%s: \"%s\"\n", __func__, dest);

close_file:
	if (!file_close (f)) {
		fprintf (stderr, "%s: file_close failed\n", __func__);
	}

destroy_file:
	file_destroy (&f);

	return r;
}

__attribute__((always_inline))
static inline size_t
_trim_name_string (uint8_t *str)
{
	size_t dest = 0, src = 0;

	// skip leading whitespace
	for (;;) {
		switch (str[src]) {
		case 0x09: case 0x20:
			++src;
			continue;
		}

		break;
	}

	for (;;) {
		switch (str[src]) {
		case 0x09: case 0x20:
			for (;;) {
				switch (str[++src]) {
				case 0x09: case 0x20:
					continue;

				case 0x00 ... 0x08:
				case 0x0a ... 0x1f:
				case 0x7f:
				case 0xf8 ... 0xff:
					goto _at_end;
				}

				break;
			}

			str[dest++] = 0x20;
			break;

		// C0 controls
		case 0x00 ... 0x08:
		case 0x0a ... 0x1f:
		// delete
		case 0x7f:
		// continuation char in wrong place
		//case 0x80 ... 0xbf:
		// invalid UTF-8
		case 0xf8 ... 0xff:
		_at_end:
			str[dest] = '\0';
			return dest;

		}

		str[dest++] = str[src++];
	}
}

__attribute__((always_inline))
static inline void
_banner_cache_import_banner (struct banner_cache *bc,
                             char                *path,
                             size_t               path_alloc,
                             size_t               path_pos,
                             struct dirent       *entry,
                             uuid_t               uuid)
{
	DIR *dirp;

	if (!(dirp = opendir (path))) {
		fprintf (stderr, "%s: opendir failed: %s\n", __func__,
		         strerror (errno));
		return;
	}

	uint8_t buf[256];
	size_t size;
	sha1_t hash = SHA1_INITIALIZER;
	uint32_t w = 0, h = 0;
	int32_t x = 0, y = 0;
	char *banner_name = NULL;

	for (int i;;) {
		struct dirent *result;
		if ((i = readdir_r (dirp, entry, &result))) {
			fprintf (stderr,
			         "%s: readdir_r failed: %s\n",
			         __func__, strerror (i));
			result = NULL;
			i = 0;
			goto close_dir;
		}

		if (!result) {
			// end of directory stream
			break;
		}

		if (result->d_type != DT_REG) {
			// these are not the entries you're looking for
			continue;
		}

		const char *name = (const char *) result->d_name;

		switch (name[0]) {
		case 'h':
			if (!name[1]) {
				path[path_pos] = 'h';
				path[path_pos + 1] = '\0';
				size = 0;
				if (_textfile_read (path, buf, 256, &size)) {
					unsigned int j = 0;
					if (!_parse_u32 ((char *) buf, &j, &h)) {
						h = 0;
					}
				}
				path[path_pos] = '\0';
			}
			continue;

		case 'i':
			if (!name[1]) {
				path[path_pos] = 'i';
				path[path_pos + 1] = '\0';
				size = 0;
				if (_textfile_read (path, buf, 256, &size)) {
					if (!_sha1_parse ((const char *) buf, &hash)) {
						_sha1_init (&hash);
					}
				}
				path[path_pos] = '\0';
			}
			continue;

		case 'n':
			if (!name[1]) {
				path[path_pos] = 'n';
				path[path_pos + 1] = '\0';
				size = 0;
				if (_textfile_read (path, buf, 256, &size)) {
					(void) _trim_name_string (buf);
					banner_name = strdup ((const char *) buf);
				}
				path[path_pos] = '\0';
			}
			continue;

		case 'w':
			if (!name[1]) {
				path[path_pos] = 'w';
				path[path_pos + 1] = '\0';
				size = 0;
				if (_textfile_read (path, buf, 256, &size)) {
					unsigned int j = 0;
					if (!_parse_u32 ((char *) buf, &j, &w)) {
						w = 0;
					}
				}
				path[path_pos] = '\0';
			}
			continue;

		case 'x':
			if (!name[1]) {
				path[path_pos] = 'x';
				path[path_pos + 1] = '\0';
				size = 0;
				if (_textfile_read (path, buf, 256, &size)) {
					unsigned int j = 0;
					if (!_parse_i32 ((char *) buf, &j, &x)) {
						x = 0;
					}
				}
				path[path_pos] = '\0';
			}
			continue;

		case 'y':
			if (!name[1]) {
				path[path_pos] = 'y';
				path[path_pos + 1] = '\0';
				size = 0;
				if (_textfile_read (path, buf, 256, &size)) {
					unsigned int j = 0;
					if (!_parse_i32 ((char *) buf, &j, &y)) {
						y = 0;
					}
				}
				path[path_pos] = '\0';
			}
		}
	}

	_sha1_str (&hash, (char *) buf);
	printf ("name:   \"%s\"\n"
	        "hash:   %s\n"
	        "width:  %" PRIu32 "\n"
	        "height: %" PRIu32 "\n"
	        "x_offs: %" PRId32 "\n"
	        "y_offs: %" PRId32 "\n",
	        banner_name, buf, w, h, x, y);
	if (banner_name) {
		free (banner_name);
	}

close_dir:
	if (closedir (dirp) == -1) {
		fprintf (stderr, "%s: closedir failed: %s\n",
		         __func__, strerror (errno));
	}

	dirp = NULL;
}

__attribute__((always_inline))
static inline void
_banner_cache_import_img (struct banner_cache *bc,
                          char                *path,
                          size_t               path_alloc,
                          size_t               path_pos,
                          struct dirent       *entry,
                          sha1_t               hash)
{
	DIR *dirp;

	if ((dirp = opendir (path))) {
		for (int i;;) {
			struct dirent *result;
			if ((i = readdir_r (dirp, entry, &result))) {
				fprintf (stderr,
				         "%s: readdir_r failed: %s\n",
				         __func__, strerror (i));
				result = NULL;
				i = 0;
				goto close_dir;
			}

			if (!result) {
				// end of directory stream
				break;
			}

			const char *name;

			switch (result->d_type) {
			case DT_REG:
				name = (const char *) result->d_name;
				unsigned int k;

				for (k = 0; name[k]; ++k) {
					path[path_pos + k] = name[k];
					switch (name[k]) {
					case '.':
						++k;
						path[path_pos + k] = name[k];
						switch (name[k]) {
						case '\0':
							goto _skip;
						case 'G': case 'g':
							++k;
							path[path_pos + k] = name[k];
							switch (name[k]) {
							case '\0':
								goto _skip;
							case 'I': case 'i':
								++k;
								path[path_pos + k] = name[k];
								switch (name[k]) {
								case '\0':
									goto _skip;
								case 'F': case 'f':
									++k;
									path[path_pos + k] = name[k];
									if (!name[k]) {
										printf ("%s: GIF: %s\n",
											__func__, name);
										goto _skip;
									}
								}
							}
							break;

						case 'J': case 'j':
							++k;
							path[path_pos + k] = name[k];
							switch (name[k]) {
							case '\0':
								goto _skip;
							case 'P': case 'p':
								++k;
								path[path_pos + k] = name[k];
								switch (name[k]) {
								case '\0':
									goto _skip;
								case 'E': case 'e':
									++k;
									path[path_pos + k] = name[k];
									switch (name[k]) {
									case '\0':
										goto _skip;
									case 'G': case 'g':
										goto _maybe_jpeg;
									}
									break;

								case 'G': case 'g':
								_maybe_jpeg:
									++k;
									path[path_pos + k] = name[k];
									if (!name[k]) {
										printf ("%s: JPG: %s\n",
											__func__, name);
										goto _skip;
									}
								}
							}
							break;

						case 'P': case 'p':
							++k;
							path[path_pos + k] = name[k];
							switch (name[k]) {
							case '\0':
								goto _skip;
							case 'N': case 'n':
								++k;
								path[path_pos + k] = name[k];
								switch (name[k]) {
								case '\0':
									goto _skip;
								case 'G': case 'g':
									++k;
									path[path_pos + k] = name[k];
									if (!name[k]) {
										printf ("%s: JPG: %s\n",
											__func__, name);
									}
								}
							}
						}
					}
				}

				path[path_pos + k] = name[k];

			default:
			_skip:
				continue;
			}
		}

	close_dir:
		if (closedir (dirp) == -1) {
			fprintf (stderr, "%s: closedir failed: %s\n",
			         __func__, strerror (errno));
		}

		path[path_pos] = '\0';
		dirp = NULL;

	} else {
		fprintf (stderr, "%s: opendir failed: %s\n", __func__,
		         strerror (errno));
	}

/*
	struct img *im = _img_read_file ((const char *) path);

	if (!im) {
		fprintf (stderr, "%s: _img_read_file failed\n", __func__);
		return;
	}

	char str[41];
	_sha1_str (&hash, str);
	printf ("%s: %s\n", __func__, str);

	if (!_sha1_cmp (&hash, &im->hash)) {
		fprintf (stderr, "%s: hash mismatch\n", __func__);
	}

	_img_destroy (im);
	im = NULL;
*/
}

__attribute__((always_inline))
static inline bool
_banner_cache_import_subdir (struct banner_cache *bc,
                             char                *path,
                             size_t               path_alloc,
                             size_t               path_pos,
                             struct dirent       *entry,
                             uint8_t              id)
{
	bool r;
	DIR *dirp;

	if ((dirp = opendir (path))) {
		for (int i;;) {
			struct dirent *result;
			if ((i = readdir_r (dirp, entry, &result))) {
				fprintf (stderr,
				         "%s: readdir_r failed: %s\n",
				         __func__, strerror (i));
				result = NULL;
				i = 0;
				r = false;
				goto close_dir;
			}

			if (!result) {
				// end of directory stream
				break;
			}

			const char *name;
			size_t k;
			uint8_t c;
			union {
				uint8_t u8[20];
				sha1_t  sha1;
				uuid_t  uuid;
			} v;

			switch (result->d_type) {
			case DT_DIR:
				name = (const char *) result->d_name;
				k = 0;
				v.u8[0] = id;

				do {
					switch (name[k]) {
					case '0' ... '9':
						c = name[k] - '0';
						break;
					case 'A' ... 'F':
						c = 10 + (name[k] - 'A');
						break;
					case 'a' ... 'f':
						c = 10 + (name[k] - 'a');
						break;
					default:
						goto _skip;
					}

					path[path_pos + k] = name[k];
					c <<= 4;
					++k;

					switch (name[k]) {
					case '0' ... '9':
						c |= name[k] - '0';
						break;
					case 'A' ... 'F':
						c |= 10 + (name[k] - 'A');
						break;
					case 'a' ... 'f':
						c |= 10 + (name[k] - 'a');
						break;
					default:
						goto _skip;
					}

					path[path_pos + k] = name[k];
					v.u8[++k >> 1] = c;
				} while (k < 30);

				if (!name[k]) {
					path[path_pos + k++] = '/';
					path[path_pos + k] = '\0';
					_banner_cache_import_banner (bc, path, path_alloc, path_pos + k, entry, v.uuid);
					continue;
				}

				do {
					switch (name[k]) {
					case '0' ... '9':
						c = name[k] - '0';
						break;
					case 'A' ... 'F':
						c = 10 + (name[k] - 'A');
						break;
					case 'a' ... 'f':
						c = 10 + (name[k] - 'a');
						break;
					default:
						goto _skip;
					}

					path[path_pos + k] = name[k];
					c <<= 4;
					++k;

					switch (name[k]) {
					case '0' ... '9':
						c |= name[k] - '0';
						break;
					case 'A' ... 'F':
						c |= 10 + (name[k] - 'A');
						break;
					case 'a' ... 'f':
						c |= 10 + (name[k] - 'a');
						break;
					default:
						goto _skip;
					}

					path[path_pos + k] = name[k];
					v.u8[++k >> 1] = c;
				} while (k < 38);

				if (!name[k]) {
					path[path_pos + k++] = '/';
					path[path_pos + k] = '\0';
					_banner_cache_import_img (bc, path, path_alloc, path_pos + k, entry, v.sha1);
				}

			default:
			_skip:
				continue;
			}
		}

		r = true;

	close_dir:
		if (closedir (dirp) == -1) {
			fprintf (stderr, "%s: closedir failed: %s\n",
			         __func__, strerror (errno));
		}

		path[path_pos] = '\0';
		dirp = NULL;

	} else {
		fprintf (stderr, "%s: opendir failed: %s\n", __func__,
		         strerror (errno));
		r = false;
	}

	path[path_pos] = '\0';

	return r;
}

__attribute__((always_inline))
static inline bool
_banner_cache_import (struct banner_cache *bc)
{
	const char *root_path = bc->root_path;
	size_t root_len = strlen (root_path), path_alloc;
	char *path;

	if (!(path = _mem_new (6, root_len + 42 + 256, &path_alloc))) {
		return false;
	}

	strncpy (path, root_path, root_len);
	path[root_len] = '\0';

	errno = 0;
	long name_max = pathconf (root_path, _PC_NAME_MAX);
	size_t len;

	if (-1 == name_max) {
		if (errno) {
			fprintf (stderr, "%s: pathconf failed: %s\n", __func__,
			         strerror (errno));
		}
		goto len_to_default;
	} else if (BUFSIZ > offsetof (struct dirent, d_name) + name_max + 1) {
	len_to_default:
		len = BUFSIZ;
	} else {
		len = offsetof (struct dirent, d_name) + name_max + 1;
	}

	name_max = 0;

	bool r;

	struct dirent *entry;
	if ((entry = malloc (len))) {
		DIR *dirp;
		if ((dirp = opendir (root_path))) {
			for (int i;;) {
				struct dirent *result;
				if ((i = readdir_r (dirp, entry, &result))) {
					fprintf (stderr,
					         "%s: readdir_r failed: %s\n",
					         __func__, strerror (i));
					result = NULL;
					i = 0;
					r = false;
					goto close_dir;
				}

				if (!result) {
					// end of directory stream
					break;
				}

				const char *name;
				uint8_t c;

				switch (result->d_type) {
				case DT_DIR:
					name = (const char *) result->d_name;
					switch (name[0]) {
					case '0' ... '9':
						c = name[0] - '0';
						goto _2nd_char;
					case 'A' ... 'F':
						c = 10 + (name[0] - 'A');
						goto _2nd_char;
					case 'a' ... 'f':
						c = 10 + (name[0] - 'a');
					_2nd_char:
						c <<= 4;
						switch (name[1]) {
						case '0' ... '9':
							c |= name[1] - '0';
							goto _3rd_char;
						case 'A' ... 'F':
							c |= 10 + (name[1] - 'A');
							goto _3rd_char;
						case 'a' ... 'f':
							c |= 10 + (name[1] - 'a');
						_3rd_char:
							if (name[2] == '\0') {
								path[root_len] = name[0];
								path[root_len+1] = name[1];
								path[root_len+2] = '/';
								path[root_len+3] = '\0';
								_banner_cache_import_subdir (bc, path, path_alloc, root_len + 3, entry, c);
							}
						default:
							break;
						}
					default:
						break;
					}

				case DT_LNK: // TODO: handle symlinks
				default:
					break;
				}
			}

			r = true;

		close_dir:
			if (closedir (dirp) == -1) {
				fprintf (stderr, "%s: closedir failed: %s\n",
				         __func__, strerror (errno));
			}

			dirp = NULL;
		} else {
			fprintf (stderr, "%s: opendir failed: %s\n", __func__,
			         strerror (errno));
			r = false;
		}

		free (entry);
		entry = NULL;
	} else {
		fprintf (stderr, "%s: malloc failed: %s\n",
		         __func__, strerror (errno));
		r = false;
	}

	_mem_del (&path);

	return r;
}

__attribute__((always_inline))
static inline char *
_banner_cache_path_dup (const char *path)
{
	char *str;
	size_t len = strlen (path);

	if (len <= 0) {
		path = ".";
		len = 1;
	}

	size_t append_slash = (path[len-1] != '/') ? 1 : 0;

	if ((str = malloc (len + append_slash + 1))) {
		strncpy (str, path, len);
		if (append_slash) {
			str[len++] = '/';
		}
		str[len] = '\0';
	} else {
		fprintf (stderr, "%s: malloc failed: %s\n", __func__,
		         strerror (errno));
	}

	return str;
}

__attribute__((always_inline))
static inline struct banner_cache *
_banner_cache_create (const char *path)
{
	struct banner_cache *bc;

	if (!(bc = aligned_alloc (sizeof (struct banner_cache),
	                          sizeof (struct banner_cache)))) {
		fprintf (stderr, "%s: object allocation failed\n", __func__);
	} else if (!(bc->data = aligned_alloc (sizeof (struct bucket),
	                                       512 * sizeof (struct bucket)))) {
		fprintf (stderr, "%s: lookup table allocation failed\n", __func__);
		goto fail_free_bc;
	} else if (!(bc->root_path = _banner_cache_path_dup (path))) {
		fprintf (stderr, "%s: failed to copy path\n", __func__);
		free (bc->data);
		bc->data = NULL;
	fail_free_bc:
		free (bc);
		bc = NULL;
	} else {
		list_init (&bc->by_uuid.in_use);
		bc->by_uuid.lookup_table = bc->data;
		list_init (&bc->by_hash.in_use);
		bc->by_hash.lookup_table = &bc->data[256];
		for (unsigned int i = 0; i < 512; ++i) {
			list_init (&bc->data[i].hook);
			list_init (&bc->data[i].list);
		}
		(void) _banner_cache_import (bc);
	}

	return bc;
}

__attribute__((always_inline))
static inline void
_banner_cache_destroy (struct banner_cache *bc)
{
	if (bc->root_path) {
		free (bc->root_path);
		bc->root_path = NULL;
	}

	struct bucket *bkt, *tmp;
	struct banner *b, *tmp2;

	list_for_each_entry_safe (bkt, tmp, &bc->by_uuid.in_use, hook) {
		list_for_each_entry_safe (b, tmp2, &bkt->list, by_uuid) {
			_banner_destroy (b);
		}
		__list_del_entry (&bkt->hook);
	}
	bc->by_uuid.in_use.next = NULL;
	bc->by_uuid.in_use.prev = NULL;

	list_for_each_entry_safe (bkt, tmp, &bc->by_hash.in_use, hook) {
		list_for_each_entry_safe (b, tmp2, &bkt->list, by_hash) {
			_banner_destroy (b);
		}
		__list_del_entry (&bkt->hook);
	}
	bc->by_hash.in_use.next = NULL;
	bc->by_hash.in_use.prev = NULL;

	bkt = NULL;
	tmp = NULL;
	b = NULL;
	tmp2 = NULL;

	if (bc->data) {
		(void) memset (bc->data, 0, 512 * sizeof (struct bucket));
		free (bc->data);
		bc->data = NULL;
	}

	free (bc);
	bc = NULL;
}

__attribute__((always_inline))
static inline const char *
_banner_cache_path (struct banner_cache *bc)
{
	return (const char *) bc->root_path;
}

__attribute__((always_inline))
static inline struct bucket *
_bucket_by_uuid (struct banner_cache *bc,
                 const uuid_t         uuid)
{
	return &bc->by_uuid.lookup_table[uuid[0]];
}

__attribute__((always_inline))
static inline struct bucket *
_bucket_by_hash (struct banner_cache *bc,
                 sha1_t              *hash)
{
	return &bc->by_hash.lookup_table[hash->u8[0]];
}

__attribute__((always_inline))
static inline struct banner *
_banner_by_uuid (list_head_t  *list,
                 const uuid_t  uuid)
{
	struct banner *b;
	list_for_each_entry (b, list, by_uuid) {
		if (uuid_compare (b->uuid, uuid) == 0) {
			return b;
		}
	}
	return NULL;
}

__attribute__((always_inline))
static inline struct banner *
_banner_by_hash (list_head_t *list,
                 sha1_t      *hash)
{
	struct banner *b;
	list_for_each_entry (b, list, by_hash) {
		if (_sha1_cmp (&b->hash, hash)) {
			return b;
		}
	}
	return NULL;
}

__attribute__((always_inline))
static inline struct banner *
_banner_cache_find_by_uuid (struct banner_cache *bc,
                            const uuid_t         uuid)
{
	struct bucket *bkt = _bucket_by_uuid (bc, uuid);
	return _banner_by_uuid (&bkt->list, uuid);
}

__attribute__((always_inline))
static inline struct banner *
_banner_cache_find_by_hash (struct banner_cache *bc,
                            sha1_t              *hash)
{
	struct bucket *bkt = _bucket_by_hash (bc, hash);
	return _banner_by_hash (&bkt->list, hash);
}

__attribute__((always_inline))
static inline bool
_banner_cache_mkdir (struct banner_cache *bc,
                     struct banner       *banner)
{
	bool r;
	const char *root_path = bc->root_path;
	size_t len = strlen (root_path), k;
	char *path;
	unsigned int i;
	struct stat sb;
	int e;

	if (!(path = malloc (len + 2 + 1 + 30 + 1 + 4 + 1))) {
		return false;
	}

	strncpy (path, root_path, len);

	i = banner->uuid[0];
	path[len++] = _hex_char (i >> 4);
	path[len++] = _hex_char (i & 0x0f);
	path[len] = '/';

	k = len + 1;
	for (size_t j = 0; j < 15;) {
		i = banner->uuid[++j];
		path[k++] = _hex_char (i >> 4);
		path[k++] = _hex_char (i & 0x0f);
	}
	path[k] = '\0';

	printf ("%s: target: %s\n", __func__, path);
	path[len] = '\0';

	/* check for, and if necessary create, first part of banner path */
	if (stat (path, &sb) == -1) {
		if ((e = errno) == ENOENT) {
			printf ("%s: trying to mkdir: %s\n", __func__, path);
			errno = 0;

			if (mkdir (path,
			           S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) == -1) {
				fprintf (stderr, "%s: mkdir: %s\n", __func__,
				         strerror (errno));
				goto fail;
			}

			printf ("%s: created path: %s\n", __func__, path);
			path[len] = '/';
			goto create_subdir;
		}

		fprintf (stderr, "%s: stat: %s\n", __func__, strerror (e));
		goto fail;

	} else if (!S_ISDIR(sb.st_mode)) {
		fprintf (stderr, "%s: not a directory: %s\n", __func__, path);
		goto fail;
	} else {
		printf ("%s: path exists: %s\n", __func__, path);
	}

	/* check for, and if necessary create, remaining part of banner path */
	path[len] = '/';
	if (stat (path, &sb) == -1) {
		if ((e = errno) == ENOENT) {
		create_subdir:
			printf ("%s: trying to mkdir: %s\n", __func__, path);
			errno = 0;

			if (mkdir (path,
			           S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) == -1) {
				fprintf (stderr, "%s: mkdir: %s\n", __func__,
				         strerror (errno));
				goto fail;
			}

			printf ("%s: created path: %s\n", __func__, path);
			goto store_banner_data;
		}

		fprintf (stderr, "%s: stat: %s\n", __func__, strerror (e));
		goto fail;

	} else if (!S_ISDIR(sb.st_mode)) {
		fprintf (stderr, "%s: not a directory: %s\n", __func__, path);
	fail:
		r = false;
		goto end;
	} else {
		printf ("%s: path exists: %s\n", __func__, path);
	}

store_banner_data:
	printf ("%s: store banner data to: %s\n", __func__, path);
	r = true;

end:
	free (path);
	path = NULL;

	return r;
}

__attribute__((always_inline))
static inline bool
_banner_cache_add_banner (struct banner_cache *bc,
                          struct banner       *banner)
{
	struct bucket *bkt = _bucket_by_hash (bc, &banner->hash);
	struct banner *b;

	if ((b = _banner_by_hash (&bkt->list, &banner->hash))) {
		char str[41], str2[41];
		_banner_hash_unparse (banner, str);
		_banner_hash_unparse (b, str2);
		fprintf (stderr,
		         "hash collision: %p SHA1=%s\n"
		         "                   name=%s\n"
		         "            vs. %p SHA1=%s\n"
		         "                   name=%s\n"
		         " ^ data bytes%smatch\n",
		         banner, str, banner_name (banner),
		         b, str2, banner_name (b),
		         (img_data_cmp (banner->data, b->data))
		         ? " ": " do not ");
		return false;
	}

	list_add (&banner->by_hash, &bkt->list);

	if (bkt->hook.next == &bkt->hook) {
		list_add (&bkt->hook, &bc->by_hash.in_use);
	}

	uuid_t uuid;

	do {
		uuid_generate (uuid);
		bkt = _bucket_by_uuid (bc, uuid);
	} while (_banner_by_uuid (&bkt->list, uuid));

	uuid_copy (banner->uuid, uuid);
	list_add (&banner->by_uuid, &bkt->list);

	if (bkt->hook.next == &bkt->hook) {
		list_add (&bkt->hook, &bc->by_uuid.in_use);
	}

	_banner_cache_mkdir (bc, banner);

	return true;
}

__attribute__((always_inline))
static inline struct banner *
_banner_cache_most_recent (struct banner_cache *bc)
{
	list_head_t *h;
	if ((h = bc->by_hash.in_use.next) != &bc->by_hash.in_use) {
		struct bucket *bkt = list_entry (h, struct bucket, hook);
		if ((h = bkt->list.next) != &bkt->list) {
			return list_entry (h, struct banner, by_hash);
		}
	}
	return NULL;
}

__attribute__((always_inline))
static inline char *
_banner_cache_json (struct banner_cache *bc)
{
	size_t pos, len = BUFSIZ;
	union {
		uint8_t *u;
		char    *c;
	} buf;

	if (!(buf.u = malloc (len))) {
		fprintf (stderr, "%s: malloc: %s\n",
		         __func__, strerror (errno));
		return NULL;
	}

	buf.c[0] = '[';
	pos = 1;

	list_head_t *h = &bc->by_uuid.in_use;
	struct bucket *bkt;
	list_for_each_entry (bkt, h, hook) {
		list_head_t *h2 = &bkt->list;
		struct banner *b;
		list_for_each_entry (b, h2, by_uuid) {
			if (pos > 1) {
				buf.c[pos++] = ',';
			}
			(void) _banner_serialize (b, &pos, &len, &buf.u);
		}
	}

	buf.c[pos++] = ']';
	buf.c[pos] = '\0';

	return buf.c;
}

#endif // __KIVIJALKA_PRIVATE_BANNER_CACHE_H__
