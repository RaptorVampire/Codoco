# =============================================================================
#  Codoco -- Makefile
#  https://github.com/RaptorVampire/Codoco
#  SPDX-License-Identifier: Apache-2.0 OR MIT
# =============================================================================

NAME    := codoco
VERSION := $(shell grep '^#define CODOCO_VERSION[[:space:]]' include/version.h 2>/dev/null | head -1 | cut -d'"' -f2)
ifeq ($(strip $(VERSION)),)
VERSION := 0.0.0
endif
TAGLINE := Count every line. Know every project.
URL     := https://github.com/RaptorVampire/Codoco
AUTHOR  := RaptorVampire
LICENSE := Apache-2.0 OR MIT

# -----------------------------------------------------------------------------
#  Verbosity / colors
# -----------------------------------------------------------------------------
V ?= 0
ifeq ($(V),1)
Q :=
else
Q := @
endif

NO_COLOR ?= 0
IS_TTY   := $(shell [ -t 1 ] && echo 1 || echo 0)
ifeq ($(NO_COLOR),1)
  C := 0
else
  C := $(IS_TTY)
endif

ifeq ($(C),1)
CRST  := \033[0m
CBOLD := \033[1m
CDIM  := \033[2m
CRED  := \033[31m
CGRN  := \033[32m
CYEL  := \033[33m
CCYN  := \033[36m
CBCYN := \033[96m
CBGRN := \033[92m
CBYEL := \033[93m
CBMAG := \033[95m
else
CRST  :=
CBOLD :=
CDIM  :=
CRED  :=
CGRN  :=
CYEL  :=
CCYN  :=
CBCYN :=
CBGRN :=
CBYEL :=
CBMAG :=
endif

# -----------------------------------------------------------------------------
#  Toolchain
# -----------------------------------------------------------------------------
CC      ?= $(shell command -v cc    2>/dev/null || \
                  command -v gcc   2>/dev/null || \
                  command -v clang 2>/dev/null || echo cc)
STRIP   ?= $(shell command -v strip   2>/dev/null || echo strip)
INSTALL ?= $(shell command -v install 2>/dev/null || echo install)
TAR     ?= $(shell command -v tar     2>/dev/null || echo tar)
RM      ?= rm -f

UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
UNAME_M := $(shell uname -m 2>/dev/null || echo unknown)

ifeq ($(UNAME_S),Darwin)
  ifeq ($(origin CC),default)
    CC := clang
  endif
endif

# -----------------------------------------------------------------------------
#  Flags
# -----------------------------------------------------------------------------
STD  ?= -std=c11
OPT  ?= -O2

WARN := -Wall -Wextra -Wpedantic \
        -Wshadow -Wcast-qual -Wwrite-strings \
        -Wstrict-prototypes -Wmissing-prototypes \
        -Wno-unused-parameter

DEFS := -D_GNU_SOURCE
INC  := -Iinclude

CPPFLAGS += $(DEFS) $(INC)
CFLAGS  += $(STD) $(OPT) $(WARN) $(EXTRA_CFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)
LDLIBS   +=

# -----------------------------------------------------------------------------
#  Paths
# -----------------------------------------------------------------------------
SRC_DIR := src
OBJ_DIR := build
SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS    := $(OBJS:.o=.d)
BIN     := $(NAME)

# rpm target architecture (passed as --target to rpmbuild)
RPM_ARCH ?=

PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin
DESTDIR ?=
DOCDIR  ?= $(PREFIX)/share/doc/$(NAME)

USER_PREFIX := $(HOME)/.local
USER_BINDIR := $(USER_PREFIX)/bin

DIST_DIR := dist

# -----------------------------------------------------------------------------
#  Cross-compile map
# -----------------------------------------------------------------------------
CC_aarch64 := aarch64-linux-gnu-gcc
CC_arm64   := $(CC_aarch64)
CC_armhf   := arm-linux-gnueabihf-gcc
CC_armv7   := $(CC_armhf)
CC_x86_64  := x86_64-linux-gnu-gcc
CC_amd64   := $(CC_x86_64)
CC_i686    := i686-linux-gnu-gcc
CC_i386    := $(CC_i686)
CC_riscv64 := riscv64-linux-gnu-gcc

CROSS_ARCHS := x86_64 i686 aarch64 armv7 riscv64

# -----------------------------------------------------------------------------
#  Recipe prefix helper
#  Usage: $(call MSG,Verb,File,Color)
# -----------------------------------------------------------------------------
MSG = printf '  $(3)%-10s$(CRST) $(CDIM)%s$(CRST)  %s\n' "$(1)" "$(DOTS)" "$(2)"
DOTS := . . . . . . . . . .

VERB_BUILD   := Compiling
VERB_LINK    := Linking
VERB_CLEAN   := Removing
VERB_INSTALL := Installing
VERB_STRIP   := Stripping

# =============================================================================
#  Phony targets
# =============================================================================
.PHONY: all help info version authors license \
        clean distclean \
        debug release release-lto static portable strip \
        install install-user install-strip uninstall setup-path \
        test check format lint memcheck bench \
        tarball deb rpm apk pkg \
        build-all-arches deb-all-arches \
        print-%

.DEFAULT_GOAL := help

# =============================================================================
#  Help
# =============================================================================
help:
	@printf '\n  $(CBOLD)%s v%s$(CRST)\n' "$(NAME)" "$(VERSION)"
	@printf '  %s\n' "$(TAGLINE)"
	@printf '  $(CDIM)%s$(CRST)\n\n' "$(URL)"
	@printf '  $(CBOLD)Usage:$(CRST)  make $(CDIM)[target] [VAR=value]$(CRST)\n'

	@printf '\n  $(CBOLD)$(CCYN)BUILD$(CRST)\n'
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "all"            "Build the release binary"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "debug"          "Build with -O0 -g + ASan/UBSan"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "release"        "Build with -O3 -march=native"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "release-lto"    "Release with Link-Time Optimization"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "static"         "Static-linked binary"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "portable"       "Build without -march=native"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "strip"          "Strip the built binary"

	@printf '\n  $(CBOLD)$(CCYN)CROSS-COMPILE$(CRST)\n'
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "build-x86_64"   "Cross-build for x86_64"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "build-i686"     "Cross-build for i686"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "build-aarch64"  "Cross-build for aarch64"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "build-armv7"    "Cross-build for armv7 (armhf)"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "build-riscv64"  "Cross-build for riscv64"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "build-all-arches" "Try every cross toolchain available"

	@printf '\n  $(CBOLD)$(CCYN)INSTALL$(CRST)\n'
	@printf '    $(CBGRN)%-18s$(CRST) %s $(CDIM)(PREFIX=$(PREFIX))$(CRST)\n' "install" "Install binary + docs"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "install-user"   "Install into ~/.local (no sudo)"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "install-strip"  "Install, then strip"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "uninstall"      "Remove installed files"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "setup-path"     "Add ~/.local/bin to PATH"

	@printf '\n  $(CBOLD)$(CCYN)QUALITY$(CRST)\n'
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "test"           "Run the test suite"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "check"          "Static analysis (cppcheck/clang-tidy)"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "format"         "Format with clang-format"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "lint"           "Lint Makefile and shell"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "memcheck"       "Run under valgrind"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "bench"          "Benchmark on this project"

	@printf '\n  $(CBOLD)$(CCYN)PACKAGE$(CRST)\n'
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "tarball"        "Create dist/$(NAME)-$(VERSION).tar.gz"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "deb"            "Build .deb for host architecture"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "rpm"            "Build .rpm for host architecture"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "apk"            "Build .apk (Alpine)"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "pkg"            "Build every available format"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "deb-amd64"      "Cross .deb: amd64"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "deb-i386"       "Cross .deb: i386"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "deb-arm64"      "Cross .deb: arm64"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "deb-armhf"      "Cross .deb: armhf"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "deb-riscv64"    "Cross .deb: riscv64"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "deb-all-arches" "Every .deb that can be built"

	@printf '\n  $(CBOLD)$(CCYN)INFO$(CRST)\n'
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "help"           "Show this help"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "info"           "System / toolchain / install info"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "version"        "Print version"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "authors"        "Print authors"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "license"        "Print license"

	@printf '\n  $(CBOLD)$(CCYN)CLEAN$(CRST)\n'
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "clean"          "Remove build artifacts"
	@printf '    $(CBGRN)%-18s$(CRST) %s\n' "distclean"      "clean + dist/ + binary"

	@printf '\n  $(CBOLD)$(CCYN)VARIABLES$(CRST) $(CDIM)(make VAR=value)$(CRST)\n'
	@printf '    $(CBMAG)%-18s$(CRST) %s\n' "PREFIX"         "Install prefix"
	@printf '    $(CBMAG)%-18s$(CRST) %s\n' "BINDIR"         "Binary directory"
	@printf '    $(CBMAG)%-18s$(CRST) %s\n' "DESTDIR"        "Staging dir (packaging)"
	@printf '    $(CBMAG)%-18s$(CRST) %s\n' "CC"             "C compiler"
	@printf '    $(CBMAG)%-18s$(CRST) %s\n' "CFLAGS"         "Extra compile flags"
	@printf '    $(CBMAG)%-18s$(CRST) %s\n' "LDFLAGS"        "Extra link flags"
	@printf '    $(CBMAG)%-18s$(CRST) %s\n' "V"              "Verbose (V=1)"
	@printf '    $(CBMAG)%-18s$(CRST) %s\n' "NO_COLOR"       "Disable colors (NO_COLOR=1)"
	@printf '\n'

# =============================================================================
#  Info
# =============================================================================
info:
	@printf '\n  $(CBOLD)System$(CRST)\n'
	@printf '    %-20s %s\n' "OS"                 "$(UNAME_S)"
	@printf '    %-20s %s\n' "Architecture"       "$(UNAME_M)"
	@printf '    %-20s %s\n' "Hostname"           "$(shell hostname 2>/dev/null || echo -)"

	@printf '\n  $(CBOLD)Toolchain$(CRST)\n'
	@printf '    %-20s %s\n' "CC"                 "$(CC)"
	@printf '    %-20s %s\n' "CC version"         "$(shell $(CC) --version 2>/dev/null | head -1 || echo -)"
	@printf '    %-20s %s\n' "STRIP"              "$(STRIP)"
	@printf '    %-20s %s\n' "LTO support"        "attempted on demand"

	@printf '\n  $(CBOLD)Install layout$(CRST)\n'
	@printf '    %-20s %s\n' "PREFIX"             "$(PREFIX)"
	@printf '    %-20s %s\n' "BINDIR"             "$(BINDIR)"
	@printf '    %-20s %s\n' "User BINDIR"        "$(USER_BINDIR)"
	@printf '    %-20s %s\n' ".local in PATH"     "$(shell echo $$PATH | grep -q .local/bin && echo yes || echo no)"
	@printf '    %-20s %s\n' "codoco on PATH"     "$(shell command -v $(NAME) 2>/dev/null || echo -)"

	@printf '\n  $(CBOLD)Cross toolchains$(CRST)\n'
	@for a in $(CROSS_ARCHS); do \
	   eval "cc=\$$CC_$$a"; \
	   if command -v "$$cc" >/dev/null 2>&1; then \
	     printf '    $(CBGRN)%-14s$(CRST) %s\n' "$$a" "$$cc"; \
	   else \
	     printf '    $(CDIM)%-14s %s (missing)$(CRST)\n' "$$a" "$$cc"; \
	   fi; \
	 done

	@printf '\n  $(CBOLD)Optional tools$(CRST)\n'
	@for t in cppcheck clang-tidy clang-format valgrind \
	          dpkg-deb rpmbuild abuild; do \
	   if command -v "$$t" >/dev/null 2>&1; then \
	     printf '    $(CBGRN)%-14s$(CRST) %s\n' "$$t" "$$(command -v $$t)"; \
	   else \
	     printf '    $(CDIM)%-14s (missing)$(CRST)\n' "$$t"; \
	   fi; \
	 done
	@printf '\n'

# =============================================================================
#  Version / authors / license
# =============================================================================
version:
	$(Q)printf '%s v%s\n' "$(NAME)" "$(VERSION)"

authors:
	$(Q)printf '%s\n' "$(AUTHOR)"

license:
	$(Q)printf '%s is dual-licensed under %s.\n' "$(NAME)" "$(LICENSE)"
	$(Q)printf 'See LICENSE-MIT and LICENSE-APACHE in the source tree.\n'

# =============================================================================
#  Build
# =============================================================================
all: $(BIN)
	@printf '  $(CBGRN)%-10s$(CRST) $(CDIM)%s$(CRST)  %s v%s -> %s\n' \
	         "Done" "$(DOTS)" "$(NAME)" "$(VERSION)" "$(BIN)"

$(BIN): $(OBJS)
	$(Q)$(call MSG,$(VERB_LINK),$@,$(CBGRN))
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(Q)$(call MSG,$(VERB_BUILD),$<,$(CBCYN))
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

$(OBJ_DIR):
	$(Q)mkdir -p $@

-include $(DEPS)

# -----------------------------------------------------------------------------
#  Build variants
# -----------------------------------------------------------------------------
debug:
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory \
	    OPT="-O0 -g3" \
	    CFLAGS+="-fsanitize=address,undefined -fno-omit-frame-pointer" \
	    LDFLAGS+="-fsanitize=address,undefined" \
	    all

release:
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory OPT="-O3 -march=native" all

release-lto:
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory \
	    OPT="-O3 -march=native -flto" \
	    LDFLAGS+="-flto" \
	    all

static:
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory LDFLAGS+="-static" all

portable:
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory OPT="-O2" all

strip: $(BIN)
	$(Q)$(call MSG,$(VERB_STRIP),$(BIN),$(CBCYN))
	$(Q)$(STRIP) $(BIN)

# =============================================================================
#  Cross-compile
# =============================================================================
build-%:
	$(Q)cc_var="CC_$(subst -,_,$*)"; \
	 cc="$$(eval echo \$$$$cc_var)"; \
	 if [ -z "$$cc" ]; then \
	   printf '$(CRED)  unknown architecture: $*$(CRST)\n'; \
	   printf '  known: $(CROSS_ARCHS)\n'; exit 1; fi; \
	 if ! command -v "$$cc" >/dev/null 2>&1; then \
	   printf '$(CBYEL)  cross toolchain not found:$(CRST) %s\n' "$$cc"; \
	   printf '  install e.g.: sudo apt install gcc-%s-linux-gnu\n' "$*"; \
	   exit 1; fi; \
	 printf '  $(CBCYN)%-10s$(CRST) %-14s -> %s\n' "Cross" "$*" "$$cc"; \
	 $(MAKE) --no-print-directory clean >/dev/null 2>&1; \
	 $(MAKE) --no-print-directory CC="$$cc" all >/dev/null; \
	 mkdir -p $(DIST_DIR); \
	 out="$(DIST_DIR)/$(NAME)-$(VERSION)-linux-$*"; \
	 mv $(BIN) "$$out"; \
	 printf '  $(CBGRN)%-10s$(CRST) %s\n' "Ok" "$$out"

build-all-arches:
	@printf '  $(CBCYN)%-10s$(CRST) trying every available cross toolchain\n' "Matrix"
	@found=0; \
	 for a in $(CROSS_ARCHS); do \
	   eval "cc=\$$CC_$$a"; \
	   if command -v "$$cc" >/dev/null 2>&1; then \
	     found=1; \
	     $(MAKE) --no-print-directory build-$$a || true; \
	   else \
	     printf '  $(CDIM)%-10s %-14s (%s not installed)$(CRST)\n' "Skip" "$$a" "$$cc"; \
	   fi; \
	 done; \
	 if [ "$$found" = 0 ]; then \
	   printf '  $(CBYEL)%-10s no cross-compilers installed$(CRST)\n' "Note"; \
	   printf '            see: make info\n'; fi

# =============================================================================
#  Install
# =============================================================================
install: all
	$(Q)$(call MSG,$(VERB_INSTALL),$(DESTDIR)$(BINDIR)/$(BIN),$(CBCYN))
	$(Q)$(INSTALL) -d "$(DESTDIR)$(BINDIR)"
	$(Q)$(INSTALL) -d "$(DESTDIR)$(DOCDIR)"
	$(Q)$(INSTALL) -m 0755 "$(BIN)" "$(DESTDIR)$(BINDIR)/$(BIN)"
	$(Q)$(INSTALL) -m 0644 LICENSE-MIT LICENSE-APACHE "$(DESTDIR)$(DOCDIR)/"
	$(Q)printf '  $(CBGRN)%-10s$(CRST) %s v%s -> %s\n' \
	          "Done" "$(NAME)" "$(VERSION)" "$(BINDIR)/$(BIN)"

install-user: all
	$(Q)$(call MSG,$(VERB_INSTALL),$(USER_BINDIR)/$(BIN),$(CBCYN))
	$(Q)$(INSTALL) -d "$(USER_BINDIR)"
	$(Q)$(INSTALL) -m 0755 "$(BIN)" "$(USER_BINDIR)/$(BIN)"
	$(Q)if echo "$$PATH" | grep -q "$$HOME/.local/bin"; then \
	    printf '  $(CBGRN)%-10s$(CRST) run: %s --help\n' "Ready" "$(NAME)"; \
	  else \
	    printf '  $(CBYEL)%-10s ~/.local/bin not in PATH$(CRST)\n' "Note"; \
	    printf '            run: make setup-path\n'; fi

install-strip: all
	$(Q)$(STRIP) $(BIN)
	@$(MAKE) --no-print-directory install

uninstall:
	$(Q)$(call MSG,$(VERB_CLEAN),$(DESTDIR)$(BINDIR)/$(BIN),$(CBCYN))
	$(Q)$(RM) "$(DESTDIR)$(BINDIR)/$(BIN)"
	$(Q)$(RM) -r "$(DESTDIR)$(DOCDIR)"

setup-path:
	@printf '  $(CBCYN)%-10s$(CRST) ensuring ~/.local/bin is on PATH\n' "Setup"
	@line='export PATH="$$HOME/.local/bin:$$PATH"'; \
	 marker='# Added by $(NAME) v$(VERSION)'; \
	 any=0; \
	 for rc in "$$HOME/.bashrc" "$$HOME/.zshrc" \
	           "$$HOME/.bash_profile" "$$HOME/.profile"; do \
	   [ -f "$$rc" ] || continue; any=1; \
	   if grep -qF '.local/bin' "$$rc"; then \
	     printf '  $(CDIM)%-10s %s$(CRST)\n' "Already" "$$rc"; \
	   else \
	     printf '\n%s\n%s\n' "$$marker" "$$line" >> "$$rc"; \
	     printf '  $(CBGRN)%-10s %s$(CRST)\n' "Updated" "$$rc"; \
	   fi; \
	 done; \
	 if [ "$$any" = 0 ]; then \
	   printf '  no shell rc found; add:\n    %s\n' "$$line"; fi
	@printf '  $(CBGRN)%-10s$(CRST) open a new shell or: source ~/.bashrc\n' "Done"

# =============================================================================
#  Quality
# =============================================================================
test: $(BIN)
	@if [ -x tests/run.sh ]; then tests/run.sh; \
	 elif [ -x test.sh ]; then ./test.sh; \
	 elif [ -f test.sh ]; then sh test.sh; \
	 else printf '$(CBYEL)  no test suite found$(CRST)\n'; exit 1; fi

check:
	@if command -v cppcheck >/dev/null 2>&1; then \
	   printf '  $(CBCYN)%-10s$(CRST) cppcheck\n' "Check"; \
	   cppcheck --enable=all --inconclusive \
	            --suppress=missingIncludeSystem \
	            -Iinclude $(SRC_DIR)/; \
	 elif command -v clang-tidy >/dev/null 2>&1; then \
	   printf '  $(CBCYN)%-10s$(CRST) clang-tidy\n' "Check"; \
	   clang-tidy $(SRC_DIR)/*.c -- -Iinclude; \
	 else printf '$(CBYEL)  no static analyzer found$(CRST)\n'; fi

format:
	@if ! command -v clang-format >/dev/null 2>&1; then \
	   printf '$(CRED)  clang-format not found$(CRST)\n'; exit 1; fi
	$(Q)printf '  $(CBCYN)%-10s$(CRST) src/ include/\n' "Format"
	$(Q)find $(SRC_DIR) include -type f \( -name '*.c' -o -name '*.h' \) \
	      -exec clang-format -i {} +

lint:
	@if command -v checkmake >/dev/null 2>&1; then \
	   printf '  $(CBCYN)%-10s$(CRST) Makefile\n' "Lint"; \
	   checkmake Makefile || true; \
	 else printf '$(CDIM)  lint Makefile:  checkmake not installed$(CRST)\n'; fi
	@if command -v shellcheck >/dev/null 2>&1; then \
	   printf '  $(CBCYN)%-10s$(CRST) shell scripts\n' "Lint"; \
	   find . -maxdepth 2 -name '*.sh' -type f \
	        -exec shellcheck -x {} + || true; \
	 else printf '$(CDIM)  lint shell: shellcheck not installed$(CRST)\n'; fi

memcheck: debug
	@if ! command -v valgrind >/dev/null 2>&1; then \
	   printf '$(CBYEL)  valgrind not found$(CRST)\n'; exit 1; fi
	$(Q)valgrind --leak-check=full --error-exitcode=1 \
	              ./$(BIN) --no-color --ascii . > /dev/null
	$(Q)printf '  $(CBGRN)%-10s$(CRST)\n' "Clean"

bench: release
	$(Q)printf '  $(CBCYN)%-10s$(CRST) self-scan\n' "Bench"
	$(Q)time ./$(BIN) --no-color --ascii . > /dev/null

# =============================================================================
#  Packaging
# =============================================================================
tarball: distclean
	$(Q)mkdir -p "$(DIST_DIR)/$(NAME)-$(VERSION)"
	$(Q)for f in Makefile LICENSE-MIT LICENSE-APACHE \
	              $(SRC_DIR) include examples; do \
	      [ -e "$$f" ] && cp -r "$$f" "$(DIST_DIR)/$(NAME)-$(VERSION)/"; \
	   done
	$(Q)cd "$(DIST_DIR)" && $(TAR) czf "$(NAME)-$(VERSION).tar.gz" \
	                          "$(NAME)-$(VERSION)"
	$(Q)$(RM) -r "$(DIST_DIR)/$(NAME)-$(VERSION)"
	$(Q)printf '  $(CBGRN)%-10s$(CRST) %s/$(NAME)-$(VERSION).tar.gz\n' \
	          "Ok" "$(DIST_DIR)"

# ---- .deb ------------------------------------------------------------------
deb: all
	@if ! command -v dpkg-deb >/dev/null 2>&1; then \
	   printf '$(CBYEL)  dpkg-deb not found (Debian/Ubuntu only)$(CRST)\n'; exit 1; fi
	@arch="$(ARCH)"; \
	 [ -n "$$arch" ] || arch=$$(dpkg --print-architecture 2>/dev/null || echo amd64); \
	 stage="$(DIST_DIR)/deb"; \
	 rm -rf "$$stage"; \
	 mkdir -p "$$stage/DEBIAN" "$$stage/usr/bin" \
	          "$$stage/usr/share/doc/$(NAME)"; \
	 $(INSTALL) -m 0755 "$(BIN)" "$$stage/usr/bin/$(BIN)"; \
	 $(INSTALL) -m 0644 LICENSE-MIT LICENSE-APACHE \
	             "$$stage/usr/share/doc/$(NAME)/"; \
	 { \
	   echo "Package: $(NAME)"; \
	   echo "Version: $(VERSION)"; \
	   echo "Architecture: $$arch"; \
	   echo "Maintainer: $(AUTHOR)"; \
	   echo "Section: devel"; \
	   echo "Priority: optional"; \
	   echo "Description: $(TAGLINE)"; \
	   echo " $(URL)"; \
	 } > "$$stage/DEBIAN/control"; \
	 out="$(DIST_DIR)/$(NAME)_$(VERSION)_$$arch.deb"; \
	 dpkg-deb --build "$$stage" "$$out" >/dev/null; \
	 rm -rf "$$stage"; \
	 printf '  $(CBGRN)%-10s$(CRST) %s\n' "Ok" "$$out"

# ---- .rpm ------------------------------------------------------------------
rpm: all
	@if ! command -v rpmbuild >/dev/null 2>&1; then \
	   printf '$(CBYEL)  rpmbuild not found (Fedora/RHEL/openSUSE only)$(CRST)\n'; exit 1; fi
	@top="$(CURDIR)/$(DIST_DIR)/rpm"; \
	 rm -rf "$$top"; \
	 for d in BUILD RPMS SOURCES SPECS SRPMS; do mkdir -p "$$top/$$d"; done; \
	 tar --transform "s,^,$(NAME)-$(VERSION)/," -cf - \
	     Makefile LICENSE-MIT LICENSE-APACHE \
	     $(SRC_DIR) include examples 2>/dev/null | \
	     gzip -9 > "$$top/SOURCES/$(NAME)-$(VERSION).tar.gz"; \
	 { \
	   echo 'Name:           $(NAME)'; \
	   echo 'Version:        $(VERSION)'; \
	   echo 'Release:        1%{?dist}'; \
	   echo 'Summary:        $(TAGLINE)'; \
	   echo 'License:        $(LICENSE)'; \
	   echo 'URL:            $(URL)'; \
	   echo 'Source0:        $(NAME)-$(VERSION).tar.gz'; \
	   echo '%global __strip /bin/true'; \
	   echo '%global __objdump /bin/true'; \
	   echo '%global __brp_strip %{nil}'; \
	   echo '%global __brp_strip_elf %{nil}'; \
	   echo '%global __brp_strip_static_archive %{nil}'; \
	   echo '%global debug_package %{nil}'; \
	   echo '%description'; \
	   echo '$(NAME) counts lines of code across many languages.'; \
	   echo '%prep'; \
	   echo '%setup -q'; \
	   echo '%build'; \
	   echo 'make all CC="%{getenv:CC}"'; \
	   echo '%install'; \
	   echo 'make install DESTDIR=%{buildroot} PREFIX=/usr'; \
	   echo '%files'; \
	   echo '/usr/bin/$(NAME)'; \
	   echo '/usr/share/doc/$(NAME)'; \
	 } > "$$top/SPECS/$(NAME).spec"; \
	 rpmbuild --nodeps --define "__os_install_post %{nil}" $(if $(RPM_ARCH),--target $(RPM_ARCH),) --define "_topdir $$top" -bb "$$top/SPECS/$(NAME).spec" >/dev/null; \
	 find "$$top/RPMS" -name '*.rpm' -exec cp {} "$(DIST_DIR)/" \; ; \
	 rm -rf "$$top"; \
	 printf '  $(CBGRN)%-10s$(CRST) $(DIST_DIR)/*.rpm\n' "Ok"

apk: all
	@if ! command -v abuild >/dev/null 2>&1; then \
	   printf '$(CBYEL)  abuild not found (Alpine Linux only)$(CRST)\n'; exit 1; fi
	@printf '  see https://wiki.alpinelinux.org/wiki/APKBUILD_Reference\n'

pkg:
	@printf '  $(CBCYN)%-10s$(CRST) building every available format\n' "Pkg"
	@$(MAKE) --no-print-directory tarball
	@command -v dpkg-deb >/dev/null 2>&1 && $(MAKE) --no-print-directory deb || \
	  printf '$(CDIM)  deb: skipped$(CRST)\n'
	@command -v rpmbuild >/dev/null 2>&1 && $(MAKE) --no-print-directory rpm || \
	  printf '$(CDIM)  rpm: skipped$(CRST)\n'

# ---- Cross-arch .deb -------------------------------------------------------
deb-amd64:    ARCH=amd64   CC=$(CC_amd64)
deb-x86_64:   ARCH=amd64   CC=$(CC_x86_64)
deb-i386:     ARCH=i386    CC=$(CC_i686)
deb-i686:     ARCH=i386    CC=$(CC_i686)
deb-arm64:    ARCH=arm64   CC=$(CC_aarch64)
deb-aarch64:  ARCH=arm64   CC=$(CC_aarch64)
deb-armhf:    ARCH=armhf   CC=$(CC_armhf)
deb-armv7:    ARCH=armhf   CC=$(CC_armv7)
deb-riscv64:  ARCH=riscv64 CC=$(CC_riscv64)

deb-amd64 deb-x86_64 deb-i386 deb-i686 \
deb-arm64 deb-aarch64 deb-armhf deb-armv7 deb-riscv64:
	@if ! command -v "$(CC)" >/dev/null 2>&1; then \
	   printf '$(CBYEL)  cross toolchain missing:$(CRST) %s\n' "$(CC)"; exit 1; fi
	@if ! command -v dpkg-deb >/dev/null 2>&1; then \
	   printf '$(CBYEL)  dpkg-deb not found$(CRST)\n'; exit 1; fi
	@printf '  $(CBCYN)%-10s$(CRST) deb: %-10s via %s\n' "Cross" "$(ARCH)" "$(CC)"
	@$(MAKE) --no-print-directory clean >/dev/null 2>&1
	@$(MAKE) --no-print-directory \
	    CC="$(CC)" \
	    ARCH="$(ARCH)" \
	    deb

deb-all-arches:
	@printf '  $(CBCYN)%-10s$(CRST) building every .deb we can\n' "Matrix"
	@for pair in amd64:$(CC_amd64) i386:$(CC_i686) \
	             arm64:$(CC_aarch64) armhf:$(CC_armhf) riscv64:$(CC_riscv64); do \
	   a=$${pair%%:*}; cc=$${pair##*:}; \
	   if command -v "$$cc" >/dev/null 2>&1; then \
	     $(MAKE) --no-print-directory deb-$$a || true; \
	   else \
	     printf '  $(CDIM)%-10s %-10s (%s not installed)$(CRST)\n' "Skip" "$$a" "$$cc"; \
	   fi; \
	 done

# =============================================================================
#  Clean
# =============================================================================
clean:
	$(Q)$(call MSG,$(VERB_CLEAN),$(OBJ_DIR)/,$(CBCYN))
	$(Q)$(RM) -r $(OBJ_DIR)

distclean: clean
	$(Q)$(call MSG,$(VERB_CLEAN),$(BIN) $(DIST_DIR)/,$(CBCYN))
	$(Q)$(RM) $(BIN)
	$(Q)$(RM) -r $(DIST_DIR)

# -----------------------------------------------------------------------------
#  Debug helper
# -----------------------------------------------------------------------------
print-%:
	@printf '%s = %s\n' "$*" "$($*)"