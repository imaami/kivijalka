/** \file buf.h
 *
 * Memory buffer.
 */

#ifndef __KIVIJALKA_BUF_H__
#define __KIVIJALKA_BUF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef struct buf buf_t;

/**
 * @brief Create (allocate) a new data buffer.
 * @param size Number of bytes the buffer should have available.
 * @return Pointer to newly allocated buffer object if the operation
 *         succeeded, NULL otherwise.
 */
extern buf_t *
buf_create (size_t size);

/**
 * @brief Destroy a buffer object.
 * @param b Pointer to the buffer pointer.
 */
extern void
buf_destroy (buf_t **b);

extern size_t
buf_size (buf_t *b);

extern size_t
buf_used (buf_t *b);

extern size_t
buf_unused (buf_t *b);

extern uint8_t *
buf_ptr (buf_t *b);

/**
 * @brief Allocate (reserve) space within an existing buffer object.
 * @param b Pointer to the buffer pointer.
 * @param size Number of bytes to allocate.
 * @return Pointer to the first byte of the allocated space
 *         if the operation succeeded, NULL otherwise. Do not
 *         free() the returned pointer - it is just a pointer
 *         into the buffer object's internal data array, not
 *         an allocated object.
 */
extern uint8_t *
buf_alloc (buf_t  *b,
           size_t  size);

/**
 * @brief Reset a buffer's used bytes counter. No data is zeroed out,
 *        but the next call to \a buf_alloc will return a pointer
 *        which points the beginning of the data array, and writing to
 *        it will therefore overwrite previously written bytes.
 * @param b Pointer to the buffer pointer.
 * @return Pointer to the first byte of the buffer's data array.
 */
extern char *
buf_reset (buf_t *b);

#ifdef __cplusplus
}
#endif

#endif // __KIVIJALKA_BUF_H__
