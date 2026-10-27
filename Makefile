# make predefines CC=cc, so "?=" would never fire; replace only that built-in default.
ifeq ($(origin CC),default)
CC := clang
endif
CLANG_FORMAT ?= clang-format
LDFLAGS ?= -no-pie

CFLAGS  := -std=c17 -O2 -Wall -Wextra -Werror -ffreestanding -fno-stack-protector -fno-pic -Iinclude
RISCV   := --target=riscv64-unknown-elf -march=rv64gc -mabi=lp64d -mcmodel=medany
X86     := --target=x86_64-unknown-none -mno-red-zone

HEADERS := include/alloc/frame.h
BUILD   := build

.PHONY: test cross format-check clean

test: $(BUILD)/frame_test
	./$(BUILD)/frame_test

$(BUILD)/frame_test: src/frame.c test/frame_test.c $(HEADERS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(LDFLAGS) src/frame.c test/frame_test.c -o $@

cross: $(BUILD)/frame-riscv64.o $(BUILD)/frame-x86_64.o

$(BUILD)/frame-riscv64.o: src/frame.c $(HEADERS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(RISCV) -c src/frame.c -o $@

$(BUILD)/frame-x86_64.o: src/frame.c $(HEADERS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(X86) -c src/frame.c -o $@

format-check:
	$(CLANG_FORMAT) --dry-run --Werror src/frame.c test/frame_test.c $(HEADERS)

clean:
	rm -rf $(BUILD)
