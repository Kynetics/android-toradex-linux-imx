/*
 * Copyright (c) 2011 Petteri Aimonen <jpa at nanopb.mail.kapsi.fi>
 *
 * This software is provided 'as-is', without any express or
 * implied warranty. In no event will the authors be held liable
 * for any damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you
 * must not claim that you wrote the original software. If you use
 * this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and
 *   must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 *   distribution.
 */
/* pb_common.h: Common support functions for pb_encode.c and pb_decode.c.
 * These functions are rarely needed by applications directly.
 */

#ifndef PB_COMMON_H_INCLUDED
#define PB_COMMON_H_INCLUDED

#include "pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Iterator for pb_field_t list */
struct pb_field_iter_s {
    const pb_field_t *start;       /* Start of the pb_field_t array */
    const pb_field_t *pos;         /* Current position of the iterator */
    unsigned required_field_index; /* Zero-based index that counts only the required fields */
    void *dest_struct;             /* Pointer to start of the structure */
    void *pData;                   /* Pointer to current field value */
    void *pSize;                   /* Pointer to count/has field */
};
typedef struct pb_field_iter_s pb_field_iter_t;

/* Initialize the field iterator structure to beginning.
 * Returns false if the message type is empty. */
bool pb_field_iter_begin(pb_field_iter_t *iter, const pb_field_t *fields, void *dest_struct);

/* Advance the iterator to the next field.
 * Returns false when the iterator wraps back to the first field. */
bool pb_field_iter_next(pb_field_iter_t *iter);

/* Advance the iterator until it points at a field with the given tag.
 * Returns false if no such field exists. */
bool pb_field_iter_find(pb_field_iter_t *iter, uint32_t tag);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

