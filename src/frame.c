/* SPDX-License-Identifier: GPL-3.0-only */
#include <alloc/frame.h>

static unsigned char *frame_map;
static unsigned long frame_first;
static unsigned long frame_end;

void frame_init(unsigned char *map, unsigned long first, unsigned long end) {
  unsigned long ppn;

  frame_map = map;
  frame_first = first;
  frame_end = end;
  for (ppn = first; ppn < end; ppn++)
    frame_map[ppn - first] = 0;
}

long frame_alloc(void) {
  unsigned long ppn = frame_end;

  while (ppn > frame_first) {
    ppn--;
    if (frame_map[ppn - frame_first] == 0) {
      frame_map[ppn - frame_first] = 1;
      return (long)ppn;
    }
  }
  return -1;
}

int frame_free(unsigned long ppn) {
  if (ppn < frame_first || ppn >= frame_end ||
      frame_map[ppn - frame_first] == 0)
    return -1;
  frame_map[ppn - frame_first]--;
  return 0;
}
