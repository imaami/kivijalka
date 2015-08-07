#include "private/parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

int
main (int    argc,
      char **argv)
{
	if (argc < 2 || !argv) {
		return EXIT_FAILURE;
	}

	puts ("arg  pos  type  value");

	unsigned int i = 1;
	while (i < argc) {
		char *ptr = argv[i];
		unsigned int pos = 0;
		while (ptr[pos]) {
			if (ptr[pos] == 'u') {
				if (ptr[pos + 1] == '8'
				    && ptr[pos + 2] == ':') {
					pos += 3;
					uint8_t v;
					printf ("%3.u  %3.d   u8   ", i, pos);
					if (!_parse_u8 (ptr, &pos, &v)) {
						printf ("%s\n",
							(v == UINT8_MAX)
							? "overflow"
							: "error");
						goto _next_arg;
					}
					printf ("%" PRIu8 "\n", v);

				} else if (ptr[pos + 1] == '1'
				           && ptr[pos + 2] == '6'
				           && ptr[pos + 3] == ':') {
					pos += 4;
					uint16_t v;
					printf ("%3.u  %3.d  u16   ", i, pos);
					if (!_parse_u16 (ptr, &pos, &v)) {
						printf ("%s\n",
							(v == UINT16_MAX)
							? "overflow"
							: "error");
						goto _next_arg;
					}
					printf ("%" PRIu16 "\n", v);

				} else if (ptr[pos + 1] == '3'
				           && ptr[pos + 2] == '2'
				           && ptr[pos + 3] == ':') {
					pos += 4;
					uint32_t v;
					printf ("%3.u  %3.d  u32   ", i, pos);
					if (!_parse_u32 (ptr, &pos, &v)) {
						printf ("%s\n",
							(v == UINT32_MAX)
							? "overflow"
							: "error");
						goto _next_arg;
					}
					printf ("%" PRIu32 "\n", v);

				} else if (ptr[pos + 1] == '6'
				           && ptr[pos + 2] == '4'
				           && ptr[pos + 3] == ':') {
					pos += 4;
					uint64_t v;
					printf ("%3.u  %3.d  u64   ", i, pos);
					if (!_parse_u64 (ptr, &pos, &v)) {
						printf ("%s\n",
							(v == UINT64_MAX)
							? "overflow"
							: "error");
						goto _next_arg;
					}
					printf ("%" PRIu64 "\n", v);

				} else {
					++pos;
				}

			} else if (ptr[pos] == 'i') {
				if (ptr[pos + 1] == '1'
				    && ptr[pos + 2] == '6'
				    && ptr[pos + 3] == ':') {
					pos += 4;
					int16_t v;
					printf ("%3.u  %3.d  i16   ", i, pos);
					if (!_parse_i16 (ptr, &pos, &v)) {
						printf ("%s\n",
						        (v == INT16_MAX)
						        ? "overflow"
						        : (v == INT16_MIN)
						          ? "underflow"
						          : "error");
						goto _next_arg;
					}
					printf ("%" PRId16 "\n", v);

				} else if (ptr[pos + 1] == '3'
				           && ptr[pos + 2] == '2'
				           && ptr[pos + 3] == ':') {
					pos += 4;
					int32_t v;
					printf ("%3.u  %3.d  i32   ", i, pos);
					if (!_parse_i32 (ptr, &pos, &v)) {
						printf ("%s\n",
						        (v == INT32_MAX)
						        ? "overflow"
						        : (v == INT32_MIN)
						          ? "underflow"
						          : "error");
						goto _next_arg;
					}
					printf ("%" PRId32 "\n", v);

				} else if (ptr[pos + 1] == '6'
				           && ptr[pos + 2] == '4'
				           && ptr[pos + 3] == ':') {
					pos += 4;
					int64_t v;
					printf ("%3.u  %3.d  i64   ", i, pos);
					if (!_parse_i64 (ptr, &pos, &v)) {
						printf ("%s\n",
						        (v == INT64_MAX)
						        ? "overflow"
						        : (v == INT64_MIN)
						          ? "underflow"
						          : "error");
						goto _next_arg;
					}
					printf ("%" PRId64 "\n", v);

				} else {
					++pos;
				}

			} else {
				++pos;
			}
		}

	_next_arg:
		++i;
	}

	return EXIT_SUCCESS;
}
