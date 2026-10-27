# alloc

Physical page-frame allocator for the [linur](https://github.com/linur-io/linur)
kernel: a freestanding C leaf module with one byte of bookkeeping per allocatable
frame and no libc dependency.

## ABI

| Symbol | Signature | Meaning |
| --- | --- | --- |
| `frame_init` | `void frame_init(unsigned char *map, unsigned long first, unsigned long end)` | record `map`, `first`, `end`; zero `map[0..end - first)` |
| `frame_alloc` | `long frame_alloc(void)` | allocate the highest free frame; return its ppn, or `-1` |
| `frame_free` | `int frame_free(unsigned long ppn)` | drop one owner; return `0`, or `-1` if invalid or free |

## Semantics

- One byte per allocatable page frame, indexed by **`ppn - first`**: `0` means
  free, `>= 1` means owner count. The map is sized to the allocatable window
  (`end - first` bytes), not to the whole physical address space: the kernel
  needs 32 KiB for the QEMU virt RAM window instead of a map that also covers
  the hole below RAM.
- The caller owns the map storage and guarantees it covers bytes
  `[0, end - first)`. `frame_init` zeroes the range; `first <= end` is the
  caller's promise.
- `first` and `end` are absolute page-frame numbers, `end` exclusive, not
  addresses. Every map access goes through `map[ppn - first]`.
- `frame_alloc` scans **backwards** from `end - 1` down to `first` (Linux 0.11
  `get_free_page` order), sets the first zero byte to `1`, and returns its ppn;
  it returns `-1` when every frame is owned.
- `frame_free` decrements a nonzero count; a count of 2 is legal input. One free
  leaves the frame owned, the second makes it allocatable again (M4.1 will use
  this for COW sharing).
- Frames outside `[first, end)` and already free frames make `frame_free`
  return `-1` without touching the map.
- No locking, no interrupt handling, no statistics: the caller serialises.

## Build and test

```sh
make test CC=clang-19      # host build + asserts
make test CC=gcc
make CC=clang-19 cross     # riscv64-unknown-elf and x86_64-unknown-none objects
make CC=clang-19 format-check
make clean
```

The cross targets use the same flags as the kernel's `build.rs` so the objects
link into the kernel unchanged. The library itself never calls libc: the only
imports of the host tests are `assert` and `printf`.
