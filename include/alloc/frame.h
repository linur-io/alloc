/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ALLOC_FRAME_H
#define ALLOC_FRAME_H

/*
 * Physical page-frame allocator. One byte per allocatable page frame, indexed
 * by ppn - first: 0 means free, >= 1 means owner count. The caller owns the
 * map, sizes it to the [first, end) window, and serialises every call.
 */

/* Record map, first and end, then zero map[0..end - first). */
void frame_init(unsigned char *map, unsigned long first, unsigned long end);

/* Allocate the highest free frame; return its ppn, or -1 when none is free. */
long frame_alloc(void);

/* Drop one owner of ppn; return 0 on success, -1 if invalid or already free. */
int frame_free(unsigned long ppn);

#endif /* ALLOC_FRAME_H */
