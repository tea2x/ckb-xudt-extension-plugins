TARGET := riscv64-unknown-linux-gnu
CC := $(TARGET)-gcc
LD := $(TARGET)-gcc
OBJCOPY := $(TARGET)-objcopy
CFLAGS := -fPIC -O3 -fno-builtin-printf -fno-builtin-memcmp -nostdinc -nostdlib -nostartfiles -fvisibility=hidden -fdata-sections -ffunction-sections -I deps/ckb-c-std-lib -I deps/ckb-c-std-lib/libc -I deps/ckb-c-std-lib/molecule -I c -I build -Wall -Werror -Wno-nonnull -Wno-nonnull-compare -Wno-unused-function -g
LDFLAGS := -Wl,-static -fdata-sections -ffunction-sections -Wl,--gc-sections

PROTOCOL_HEADER := c/blockchain.h
PROTOCOL_SCHEMA := c/blockchain.mol
PROTOCOL_VERSION := d75e4c56ffa40e17fd2fe477da3f98c5578edcd1
PROTOCOL_URL := https://raw.githubusercontent.com/nervosnetwork/ckb/${PROTOCOL_VERSION}/util/types/schemas/blockchain.mol
MOLC := moleculec
MOLC_VERSION := 0.7.0

# RSA/mbedtls
CFLAGS_MBEDTLS := $(subst ckb-c-std-lib,ckb-c-stdlib-20210413,$(CFLAGS)) -I deps/mbedtls/include
LDFLAGS_MBEDTLS := $(LDFLAGS)
PASSED_MBEDTLS_CFLAGS := -O3 -fPIC -nostdinc -nostdlib -DCKB_DECLARATION_ONLY -I ../../ckb-c-stdlib-20210413/libc -fdata-sections -ffunction-sections

# docker pull nervos/ckb-riscv-gnu-toolchain:gnu-bionic-20191012
BUILDER_DOCKER := nervos/ckb-riscv-gnu-toolchain@sha256:aae8a3f79705f67d505d1f1d5ddc694a4fd537ed1c7e9622420a470d59ba2ec3
CLANG_FORMAT_DOCKER := kason223/clang-format@sha256:3cce35b0400a7d420ec8504558a02bdfc12fd2d10e40206f140c4545059cd95d

all: build/hard_cap.so build/ramt_cell_lock

all-via-docker: ${PROTOCOL_HEADER}
	docker run --rm -v `pwd`:/code ${BUILDER_DOCKER} bash -c "cd /code && make"

ALL_C_SOURCE := $(wildcard c/hard_cap.c c/ramt_cell_lock.c)

build/hard_cap.so: c/hard_cap.c
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $@ $<
	$(OBJCOPY) --only-keep-debug $@ $@.debug
	$(OBJCOPY) --strip-debug --strip-all $@

build/ramt_cell_lock: c/ramt_cell_lock.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	$(OBJCOPY) --only-keep-debug $@ $@.debug
	$(OBJCOPY) --strip-debug --strip-all $@

clean:
	rm build/hard_cap.so*
	
.PHONY: all all-via-docker dist clean package-clean package publish
