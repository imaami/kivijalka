#ifndef __KIVIJALKA_SHA1_H__
#define __KIVIJALKA_SHA1_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

typedef union {
	uint32_t u32[5];
	uint16_t u16[10];
	uint8_t   u8[20];
} sha1_t;

#define SHA1_INITIALIZER {.u32 = {0, 0, 0, 0, 0}}

extern void
sha1_init (sha1_t *hash);

extern void
sha1_gen (sha1_t  *hash,
          size_t   size,
          uint8_t *data);

extern void
sha1_str (sha1_t *hash,
          char   *dest);

#ifdef __cplusplus
}
#endif

#endif // __KIVIJALKA_SHA1_H__
