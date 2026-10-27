/* SPDX-License-Identifier: GPL-3.0-only */
#include <alloc/frame.h>

#include <assert.h>
#include <stdio.h>

#define MAP_BYTES 2048
#define FILL_FRAMES 1024
#define SOAK_FIRST 4096
#define SOAK_FRAMES 1024
#define SOAK_OPS 100000

static unsigned long long rng_state = 0x9e3779b97f4a7c15ULL;

/* xorshift64: deterministic pseudo-random stream for the soak tests. */
static unsigned long rng_next(void) {
  rng_state ^= rng_state << 13;
  rng_state ^= rng_state >> 7;
  rng_state ^= rng_state << 17;
  return (unsigned long)rng_state;
}

/* Fresh range: bytes ppn - first, scanned backwards from end - 1, then -1. */
static void test_backwards_scan(void) {
  unsigned char map[MAP_BYTES];
  unsigned long ppn;

  map[16] = 0xa5; /* sentinel just past the window, must stay untouched */
  frame_init(map, 100, 116);
  for (ppn = 116; ppn-- > 100;) {
    assert(frame_alloc() == (long)ppn);
    assert(map[ppn - 100] == 1);
  }
  assert(frame_alloc() == -1);
  assert(map[16] == 0xa5);
}

/* A freed frame is handed out again before any lower frame. */
static void test_free_then_alloc(void) {
  unsigned char map[MAP_BYTES];

  frame_init(map, 1000, 1008);
  assert(frame_alloc() == 1007);
  assert(frame_alloc() == 1006);
  assert(frame_free(1007) == 0);
  assert(frame_alloc() == 1007);
  assert(frame_free(1007) == 0);
  assert(frame_free(1006) == 0);
  assert(frame_alloc() == 1007);
  assert(frame_alloc() == 1006);
}

/* Invalid frees: out of range, double free, and never-allocated frames. */
static void test_invalid_free(void) {
  unsigned char map[MAP_BYTES];

  frame_init(map, 4, 9);
  assert(frame_free(3) == -1);
  assert(frame_free(9) == -1);
  assert(frame_free(1000) == -1);
  assert(frame_free(5) == -1);
  assert(frame_alloc() == 8);
  assert(map[4] == 1);
  assert(frame_free(8) == 0);
  assert(frame_free(8) == -1);
}

/* Count 2: one free keeps the frame owned, the second makes it allocatable. */
static void test_refcount_two(void) {
  unsigned char map[MAP_BYTES];

  frame_init(map, 100, 108);
  map[5] = 2; /* byte for frame 105, as if two owners shared it */
  assert(frame_alloc() == 107);
  assert(frame_alloc() == 106);
  assert(frame_alloc() == 104);
  assert(frame_free(105) == 0);
  assert(frame_alloc() == 103);
  assert(frame_alloc() == 102);
  assert(frame_alloc() == 101);
  assert(frame_alloc() == 100);
  assert(frame_alloc() == -1);
  assert(frame_free(105) == 0);
  assert(frame_alloc() == 105);
}

/* Re-init with another map and range replaces the whole state. */
static void test_reinit(void) {
  unsigned char map_a[MAP_BYTES];
  unsigned char map_b[MAP_BYTES];

  frame_init(map_a, 0, 16);
  assert(frame_alloc() == 15);

  frame_init(map_b, 200, 204);
  assert(frame_alloc() == 203);
  assert(frame_alloc() == 202);
  assert(frame_alloc() == 201);
  assert(frame_alloc() == 200);
  assert(frame_alloc() == -1);
  assert(frame_free(15) == -1);
  assert(map_a[15] == 1);
}

/* A 1024-frame range: fill to exhaustion, free shuffled, then refill. */
static void test_exhaustive_refill(void) {
  unsigned char map[MAP_BYTES];
  unsigned long order[FILL_FRAMES];
  unsigned long i, j, tmp;

  frame_init(map, 0, FILL_FRAMES);
  for (i = FILL_FRAMES; i-- > 0;) {
    order[i] = i;
    assert(frame_alloc() == (long)i);
  }
  assert(frame_alloc() == -1);

  for (i = FILL_FRAMES - 1; i > 0; i--) {
    j = rng_next() % (i + 1);
    tmp = order[i];
    order[i] = order[j];
    order[j] = tmp;
  }
  for (i = 0; i < FILL_FRAMES; i++)
    assert(frame_free(order[i]) == 0);
  assert(frame_free(order[0]) == -1);

  for (i = FILL_FRAMES; i-- > 0;)
    assert(frame_alloc() == (long)i);
  assert(frame_alloc() == -1);
}

/* Randomised alloc/free soak against a reference model of the byte map. */
static void test_soak(void) {
  unsigned char map[SOAK_FRAMES];
  unsigned char model[SOAK_FRAMES] = {0};
  unsigned long i, j, ppn;
  long expected, got;

  frame_init(map, SOAK_FIRST, SOAK_FIRST + SOAK_FRAMES);
  for (i = 0; i < SOAK_OPS; i++) {
    if (rng_next() & 1) {
      ppn = SOAK_FRAMES;
      while (ppn > 0 && model[ppn - 1] != 0)
        ppn--;
      expected = ppn > 0 ? (long)(SOAK_FIRST + ppn - 1) : -1;
      got = frame_alloc();
      assert(got == expected);
      if (got >= 0)
        model[(unsigned long)got - SOAK_FIRST] = 1;
    } else {
      ppn = rng_next() % SOAK_FRAMES;
      expected = model[ppn] != 0 ? 0 : -1;
      got = frame_free(SOAK_FIRST + ppn);
      assert(got == expected);
      if (got == 0)
        model[ppn]--;
    }
    for (j = 0; j < SOAK_FRAMES; j++)
      assert(map[j] == model[j]);
  }
}

int main(void) {
  test_backwards_scan();
  test_free_then_alloc();
  test_invalid_free();
  test_refcount_two();
  test_reinit();
  test_exhaustive_refill();
  test_soak();
  printf("frame_test: 7 groups, 1024-frame soak of %d ops, all passed\n",
         SOAK_OPS);
  return 0;
}
