CC      ?= cc
AR      ?= ar
INSTALL ?= install

-include config.mk

PREFIX        ?= /usr/local
INCDIR        := $(PREFIX)/include
LIBDIR        := $(PREFIX)/lib
SRCDIR        := src
INCPATH       := include
OBJDIR        := build/obj
STATIC_OBJDIR := $(OBJDIR)/static
PIC_OBJDIR    := $(OBJDIR)/pie
LIBDIR_OUT    := build/lib

MEM_SHM           ?= 1
MEM_LOCAL         ?= 0
AGFX_UNICODE      ?= 1
AGFX_ASCII        ?= 1
AGFX_FB           ?= 0
AGFX_DRM          ?= 0
AGFX_X_STANDALONE ?= 0
AGFX_X_LEECH      ?= 0
INPUT_STDIN       ?= 1
INPUT_X           ?= 0
INPUT_EVDEV       ?= 0
REPL_MODE         ?= 0

DEBUG             ?= 0
SHARED            ?= 0

USES_X            =  0

CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -Wno-unused-parameter -I$(INCPATH) -I$(SRCDIR) -D_GNU_SOURCE
LDFLAGS := 
LIBS    := -lc

ifeq ($(DEBUG),1)
  CFLAGS += -g3 -O0 -fsanitize=address,undefined -DDEBUG=1
  LDFLAGS += -fsanitize=address,undefined
else
  CFLAGS += -O2 -DDEBUG=0
endif

CORE_SRCS := \
#    $(SRCDIR)/core/window.c       \
#    $(SRCDIR)/core/element.c      \
#    $(SRCDIR)/core/container.c    \
#    $(SRCDIR)/core/layout.c       \
#    $(SRCDIR)/core/event.c        \
#    $(SRCDIR)/core/render.c       \
#    $(SRCDIR)/core/utf8.c

ifeq ($(REPL_MODE),1)
  CFLAGS += -DSTUI_REPL_MODE=1
  CORE_SRCS += $(SRCDIR)/core/repl.c
else
  CFLAGS += -DSTUI_REPL_MODE=0
endif

MEM_SRCS := $(SRCDIR)/mem/mem.c

ifeq ($(MEM_SHM),1)
  CFLAGS += -DMEM_BACKEND_SHM=1
  MEM_SRCS += $(SRCDIR)/mem/shm.c
  LIBS += -lrt # TODO idk if it's required
else
  CFLAGS += -DMEM_BACKEND_SHM=0
endif

ifeq ($(MEM_LOCAL),1)
  CFLAGS += -DMEM_BACKEND_LOCAL=1
  MEM_SRCS += $(SRCDIR)/mem/local.c
else
  CFLAGS += -DMEM_BACKEND_LOCAL=0
endif

MEM_ANY := $(filter 1, $(MEM_SHM) $(MEM_LOCAL))
ifeq ($(MEM_ANY),)
  $(error No memory backend selected)
endif

AGFX_SRCS := $(SRCDIR)/agfx/agfx.c

ifeq ($(AGFX_UNICODE),1)
  CFLAGS += -DAGFX_BACKEND_UNICODE=1
  AGFX_SRCS += $(SRCDIR)/agfx/backend_unicode.c
else
  CFLAGS += -DAGFX_BACKEND_UNICODE=0
endif

ifeq ($(AGFX_ASCII),1)
  CFLAGS += -DAGFX_BACKEND_ASCII=1
  AGFX_SRCS += $(SRCDIR)/agfx/backend_ascii.c
else
  CFLAGS += -DAGFX_BACKEND_ASCII=0
endif

ifeq ($(AGFX_FB),1)
  CFLAGS += -DAGFX_BACKEND_FB=1
  AGFX_SRCS += $(SRCDIR)/agfx/backend_fb.c
else
  CFLAGS += -DAGFX_BACKEND_FB=0
endif

ifeq ($(AGFX_DRM),1)
  CFLAGS += -DAGFX_BACKEND_DRM=1
  AGFX_SRCS += $(SRCDIR)/agfx/backend_drm.c
else
  CFLAGS += -DAGFX_BACKEND_DRM=0
endif

ifeq ($(AGFX_X_STANDALONE),1)
  CFLAGS += -DAGFX_BACKEND_X_STANDALONE=1
  AGFX_SRCS += $(SRCDIR)/agfx/backend_x_standalone.c
  USES_X = 1
else
  CFLAGS += -DAGFX_BACKEND_X_STANDALONE=0
endif

ifeq ($(AGFX_X_LEECH),1)
  CFLAGS += -DAGFX_BACKEND_X_LEECH=1
  AGFX_SRCS += $(SRCDIR)/agfx/backend_x_leech.c
  USES_X = 1
else
  CFLAGS += -DAGFX_BACKEND_X_LEECH=0
endif

AGFX_ANY := $(filter 1, $(AGFX_UNICODE) $(AGFX_ASCII) $(AGFX_FB) $(AGFX_DRM) $(AGFX_X_STANDALONE) $(AGFX_X_LEECH))
ifeq ($(AGFX_ANY),)
  $(error No AGFX backend selected)
endif

INPUT_SRCS := $(SRCDIR)/input/input.c

ifeq ($(INPUT_STDIN),1)
  CFLAGS += -DINPUT_BACKEND_STDIN=1
  INPUT_SRCS += $(SRCDIR)/input/backend_stdin.c
else
  CFLAGS += -DINPUT_BACKEND_STDIN=0
endif

ifeq ($(INPUT_X),1)
  CFLAGS += -DINPUT_BACKEND_X=1
  INPUT_SRCS += $(SRCDIR)/input/backend_x.c
  USES_X = 1
else
  CFLAGS += -DINPUT_BACKEND_X=0
endif

ifeq ($(INPUT_EVDEV),1)
  CFLAGS += -DINPUT_BACKEND_EVDEV=1
  INPUT_SRCS += $(SRCDIR)/input/backend_evdev.c
else
  CFLAGS += -DINPUT_BACKEND_EVDEV=0
endif

INPUT_ANY := $(filter 1, $(INPUT_STDIN) $(INPUT_X) $(INPUT_EVDEV))
ifeq ($(INPUT_ANY),)
  $(error No input backend selected)
endif

ifeq ($(USES_X),1)
  X11_CFLAGS := $(shell pkg-config --cflags x11 2>/dev/null || echo "")
  X11_LIBS   := $(shell pkg-config --libs   x11 2>/dev/null || echo "-lX11")
  CFLAGS  += $(X11_CFLAGS)
  LIBS    += $(X11_LIBS)
endif

ALL_SRCS := $(CORE_SRCS) $(MEM_SRCS) $(AGFX_SRCS) $(INPUT_SRCS)
ALL_STATIC_OBJS := $(patsubst $(SRCDIR)/%.c, $(STATIC_OBJDIR)/%.o, $(ALL_SRCS))
ALL_PIC_OBJS := $(patsubst $(SRCDIR)/%.c, $(PIC_OBJDIR)/%.o, $(ALL_SRCS))

LIB_STATIC := $(LIBDIR_OUT)/libstui3.a
LIB_SHARED := $(LIBDIR_OUT)/libstui3.so

ALL_TARGETS := $(LIB_STATIC)
ifeq ($(SHARED),1)
  ALL_TARGETS += $(LIB_SHARED)
endif

.PHONY: all clean install uninstall info help

all: $(ALL_TARGETS)

$(LIB_STATIC): $(ALL_STATIC_OBJS)
	@mkdir -p $(@D)
	$(AR) rcs $@ $^
# @echo "  AR   $@"

$(LIB_SHARED): $(ALL_PIC_OBJS)
	@mkdir -p $(@D)
	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LIBS)
# @echo "  LD   $@"

$(STATIC_OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS)       -c -o $@ $<
# @echo "  CC   $@"

$(PIC_OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<
# @echo "  CC   $@ (PIC)"

install: $(ALL_TARGETS)
	$(INSTALL) -d $(DESTDIR)$(INCDIR)/stui3
	$(INSTALL) -m 644 $(INCPATH)/stui3/*.h $(DESTDIR)$(INCDIR)/stui3/
	$(INSTALL) -d $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 644 $(LIB_STATIC) $(DESTDIR)$(LIBDIR)/
ifeq ($(SHARED),1)
	$(INSTALL) -m 755 $(LIB_SHARED) $(DESTDIR)$(LIBDIR)/
endif

uninstall:
	rm -rf  $(DESTDIR)$(INCDIR)/stui3
	rm -f   $(DESTDIR)$(LIBDIR)/libstui3.a
	rm -f   $(DESTDIR)$(LIBDIR)/libstui3.so

clean:
	rm -rf build/

info:
	@echo "========================================"
	@echo "  stui3 build configuration"
	@echo "========================================"
	@echo "  CC              = $(CC)"
	@echo "  CFLAGS          = $(CFLAGS)"
	@echo "  LDFLAGS         = $(LDFLAGS)"
	@echo "  LIBS            = $(LIBS)"
	@echo ""
	@echo "  Memory backends"
	@echo "    SHM           = $(MEM_SHM)"
	@echo "    LOCAL         = $(MEM_LOCAL)"
	@echo ""
	@echo "  AGFX backends"
	@echo "    UNICODE       = $(AGFX_UNICODE)"
	@echo "    ASCII         = $(AGFX_ASCII)"
	@echo "    FB            = $(AGFX_FB)"
	@echo "    DRM           = $(AGFX_DRM)"
	@echo "    X_STANDALONE  = $(AGFX_X_STANDALONE)"
	@echo "    X_LEECH       = $(AGFX_X_LEECH)"
	@echo ""
	@echo "  Input backends"
	@echo "    STDIN         = $(INPUT_STDIN)"
	@echo "    X             = $(INPUT_X)"
	@echo "    EVDEV         = $(INPUT_EVDEV)"
	@echo ""
	@echo "  Misc"
	@echo "    REPL_MODE     = $(REPL_MODE)"
	@echo "    DEBUG         = $(DEBUG)"
	@echo "    SHARED        = $(SHARED)"
	@echo "    PREFIX        = $(PREFIX)"
	@echo ""
	@echo "  Source files ($(words $(ALL_SRCS)) total)"
	@$(foreach f,$(ALL_SRCS),echo "    $(f)";)
	@echo "========================================"

help:
	@echo "Usage: make [TARGET] [VAR=value ...]"
	@echo ""
	@echo "Targets:"
	@echo "  all             build static library (+ shared if SHARED=1)  [default]"
	@echo "  install         install headers and library to PREFIX"
	@echo "  uninstall       remove installed files"
	@echo "  clean           remove build/"
	@echo "  info            print resolved build configuration"
	@echo "  help            this message"
	@echo ""
	@echo "Memory backends (each 1|0):"
	@echo "  MEM_SHM                   POSIX shared memory (shm_open)         [1]"
	@echo "  MEM_LOCAL                 local allocator                        [0]"
	@echo ""
	@echo "AGFX backends (each 1|0):"
	@echo "  AGFX_UNICODE              halfblock pixel rendering              [1]"
	@echo "  AGFX_ASCII                space+bgcolour pixel rendering         [1]"
	@echo "  AGFX_FB                   /dev/fb0 framebuffer                   [0]"
	@echo "  AGFX_DRM                  DRM/KMS via UAPI                       [0]"
	@echo "  AGFX_X_STANDALONE         own X11 window                         [0]"
	@echo "  AGFX_X_LEECH              draw into terminal emulator's window   [0]"
	@echo ""
	@echo "Input backends (each 1|0):"
	@echo "  INPUT_STDIN               termios / stdin                        [1]"
	@echo "  INPUT_X                   X11 keyboard + pointer                 [0]"
	@echo "  INPUT_EVDEV               /dev/input/eventX                      [0]"
	@echo ""
	@echo "Build knobs:"
	@echo "  REPL_MODE                 also build support for REPL mode       [0]"
	@echo "  DEBUG=1                   -g3 + ASan/UBSan                       [0]"
	@echo "  SHARED=1                  also build libstui3.so                 [0]"
	@echo "  PREFIX=/path              install prefix                         [/usr/local]"
	@echo ""
	@echo "Persistent overrides: create config.mk alongside this Makefile."
	@echo "Example config.mk:"
	@echo "  DEBUG=1"
	@echo "  AGFX_DRM=1"
	@echo "  INPUT_EVDEV=1"
