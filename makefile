.POSIX:

HFILES := $(wildcard *.h)

MUSL_CC := musl-gcc
CC ?= gcc

ifeq ($(OS),Windows_NT)
WINDOWS_CC ?= gcc
.DEFAULT_GOAL := windows
else
WINDOWS_CC ?= x86_64-w64-mingw32-gcc
.DEFAULT_GOAL := linux
endif

CFLAGS := -std=c99 -pedantic -pipe \
	-DHEXPROC_DATE="\"$(shell export TZ=GMT; date --rfc-3339=seconds)\"" \
	-DHEXPROC_VERSION="\"$(shell cat VERSION)\""

ANALYSIS_FLAGS := -Wfloat-equal -Wwrite-strings \
	-Wswitch-enum -Wstrict-overflow=4 -DCLEANUP

SANITIZE_FLAGS := -Og -g -fsanitize=undefined -fsanitize=leak \
	-fsanitize=address -DCLEANUP

DEBUG_FLAGS := -Og -g -DCLEANUP
RELEASE_FLAGS := -O2 -s
CHECK_FLAGS := --std=c99 --std=c11 --enable=all -j6 --quiet

VALGRIND_FLAGS := --leak-check=full --leak-resolution=high --show-reachable=yes

.PHONY: linux windows musl test benchmark benchmark-linux benchmark-windows benchmark-musl check valgrind sanitize analyze doc clean

###############################################################
########################   COMPILING   ########################
###############################################################

all: linux
# Release targets
linux: build/linux/hexproc
build/linux/hexproc: build/linux/hexproc.o
	@mkdir -p build/linux
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -o $@ $^ -lm

build/linux/%.o: %.c $(HFILES)
	@mkdir -p build/linux
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -c -o $@ $<


musl: build/musl/hexproc
build/musl/hexproc: build/musl/hexproc.o
	@mkdir -p build/musl
	$(MUSL_CC) $(CFLAGS) $(RELEASE_FLAGS) -o $@ $^ -lm

build/musl/%.o: %.c $(HFILES)
	@mkdir -p build/musl
	$(MUSL_CC) $(CFLAGS) $(RELEASE_FLAGS) -c -o $@ $<


windows: build/windows/hexproc.exe
build/windows/hexproc.exe: build/windows/hexproc.o
	@mkdir -p build/windows
	$(WINDOWS_CC) $(CFLAGS) $(RELEASE_FLAGS) -o $@ $^ -lm

build/windows/%.o: %.c $(HFILES)
	@mkdir -p build/windows
	$(WINDOWS_CC) $(CFLAGS) $(RELEASE_FLAGS) -c -o $@ $<

# Sanitized executables for finding bugs
build/sanitized/hexproc: build/sanitized/hexproc.o
	@mkdir -p build/sanitized
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -o $@ $^ -lm

build/sanitized/%.o: %.c $(HFILES)
	@mkdir -p build/sanitized
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -c -o $@ $<

# Debug targets for Valgrind, etc.
build/debug/hexproc: build/debug/hexproc.o
	@mkdir -p build/debug
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $^ -lm

build/debug/%.o: %.c $(HFILES)
	@mkdir -p build/debug
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c -o $@ $<

###############################################################
#########################   TESTING   #########################
###############################################################

test: build/linux/hexproc
	$(SHELL) test/test.sh
ifeq ($(OS),Windows_NT)
benchmark: benchmark-windows
else
benchmark: benchmark-linux
endif

benchmark-linux: build/linux/hexproc
	$(SHELL) test/benchmark.sh $<

benchmark-musl: build/musl/hexproc
	$(SHELL) test/benchmark.sh $<

benchmark-windows: build/windows/hexproc.exe
	$(SHELL) test/benchmark.sh $<

valgrind: build/debug/hexproc
	valgrind $(VALGRIND_FLAGS) ./$< example/showcase.hxp > /dev/null

# Runs the sanitized executable
sanitize: build/sanitized/hexproc
	./$< example/showcase.hxp > /dev/null

analyze: hexproc.c $(HFILES)
	cppcheck $(CHECK_FLAGS) $^
	$(CC) $(CFLAGS) $(ANALYSIS_FLAGS) -fsyntax-only $<

check: analyze valgrind sanitize test

###############################################################
#####################   DOCUMENTATION   #######################
###############################################################

doc: man/hexproc.1 man/hexproc.html

man/%.1: man/%.adoc
	asciidoctor -b manpage $<

man/%.html: man/%.adoc
	asciidoctor -b html5 $<

###############################################################
#######################   CLEANUP   ###########################
###############################################################

clean:
	@rm -r -v build || true
	@rm -v man/hexproc.1 || true
	@rm -v man/hexproc.html || true
