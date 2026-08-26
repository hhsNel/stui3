CC ?= cc
ABI_VERSION := 5

-include config.mk

empty :=
space := $(empty) $(empty)
comma := ,

SERVERDIR := server
LIBDIR := lib
COMMONDIR := common
MODULEDIR := modules
INCLUDEDIR := include
REPLDIR := repls
BUILDDIR := build

GLOBAL_CFLAGS := -std=c99 -DABI_VERSION=$(ABI_VERSION) -I$(INCLUDEDIR) -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Werror
GLOBAL_LDFLAGS := 
SERVER_CFLAGS := $(GLOBAL_CFLAGS) -I$(SERVERDIR) -I$(COMMONDIR)
LIB_CFLAGS := $(GLOBAL_CFLAGS) -I$(LIBDIR) -I$(COMMONDIR)
COMMON_CFLAGS := $(GLOBAL_CFLAGS) -I$(COMMONDIR)
MODULE_CFLAGS := $(GLOBAL_CFLAGS) -I$(SERVERDIR)
SERVER_LDFLAGS := $(GLOBAL_LDFLAGS)
LIB_LDFLAGS := $(GLOBAL_LDFLAGS)
COMMON_LDFLAGS := $(GLOBAL_LDFLAGS)
MODULE_LDFLAGS := $(GLOBAL_LDFLAGS)

SERVER_SRCS := $(shell find $(SERVERDIR) -name "*.c")
LIB_SRCS := $(shell find $(LIBDIR) -name "*.c")
COMMON_SRCS := $(shell find $(COMMONDIR) -name "*.c")
REPL_SRCS := $(shell find $(REPLDIR) -name "*.c")

SERVER_OBJS := $(SERVER_SRCS:%.c=$(BUILDDIR)/%.o)
LIB_OBJS := $(LIB_SRCS:%.c=$(BUILDDIR)/%.o)
COMMON_OBJS := $(COMMON_OBJS:%.c=$(BUILDDIR)/%.o)
MODULE_SOS := 
REPL_OBJS := $(REPL_SRCS:%.c=$(BUILDDIR)/%.o)
REPL_ELFS := $(REPL_SRCS:$(REPLDIR)/%.c=repl-%)

BUILTIN_MODULES :=
LOADABLE_MODULES :=
CONFIG :=

ifeq ($(MODULE_DUMMY),builtin)
  BUILTIN_MODULES += dummy
  CONFIG += Y
else ifeq ($(MODULE_DUMMY),module)
  LOADABLE_MODULES += dummy
  CONFIG += M
else
  CONFIG += N
endif
ifeq ($(MODULE_DUMMY2),builtin)
  BUILTIN_MODULES += dummy2
  CONFIG += Y
else ifeq ($(MODULE_DUMMY2),module)
  LOADABLE_MODULES += dummy2
  CONFIG += M
else
  CONFIG += N
endif

BUILTIN_MODULE_SRCS := $(foreach BUILTIN_MODULE, $(BUILTIN_MODULES), $(shell find $(MODULEDIR)/$(BUILTIN_MODULE) -name "*.c"))
SERVER_SRCS += $(BUILTIN_MODULE_SRCS)
SERVER_OBJS += $(BUILTIN_MODULE_SRCS:$(MODULEDIR)/%.c=$(BUILDDIR)/builtin/%.o)
SERVER_CFLAGS += -DALL_BUILTIN_MODULES="$(subst $(space),$(comma),$(BUILTIN_MODULES))"
MODULE_SOS += $(foreach LOADABLE_MODULE, $(LOADABLE_MODULES), $(LOADABLE_MODULE).so)

all: $(ALL_CLEAN_DEP) all-modules repl-elfs

.config: FORCE
	@if [ "$$(cat $@ 2>/dev/null || true)" != "$(CONFIG)" ]; then \
		echo "$(CONFIG)" > $@; \
	fi

all-modules: $(MODULE_SOS)

repl-elfs: $(REPL_ELFS)

$(BUILDDIR)/builtin/%.o: $(MODULEDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(MODULE_CFLAGS) -DCOMPILE_MODULE_BUILTIN=1 -DCOMPILE_MODULE_LOADABLE=0 -DCOMPILE_MODULE_NAME=$(firstword $(subst /, ,$*)) -c -o $@ $<

$(BUILDDIR)/loadable/%.o: $(MODULEDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(MODULE_CFLAGS) -DCOMPILE_MODULE_BUILTIN=0 -DCOMPILE_MODULE_LOADABLE=1 -DCOMPILE_MODULE_NAME=$(firstword $(subst /, ,$*)) -fPIC -c -o $@ $<

$(BUILDDIR)/$(SERVERDIR)/%.o: $(SERVERDIR)/%.c .config
	@mkdir -p $(@D)
	$(CC) $(SERVER_CFLAGS) -c -o $@ $<
$(BUILDDIR)/$(LIBDIR)/%.o: $(LIBDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(LIB_CFLAGS) -c -o $@ $<
$(BUILDDIR)/$(COMMONDIR)/%.o: $(COMMONDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(COMMON_CFLAGS) -c -o $@ $<
$(BUILDDIR)/$(REPLDIR)/%.o: $(REPLDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(GLOBAL_CFLAGS) -I$(SERVERDIR) -I$(LIBDIR) -I$(COMMONDIR) -c -o $@ $<

define LOADABLE_MODULE_template
$(1).so: $(patsubst $(MODULEDIR)/%.c, $(BUILDDIR)/loadable/%.o, $(shell find $(MODULEDIR)/$(1) -name "*.c"))
	$(CC) $(MODULE_LDFLAGS) -shared -o $$@ $$^
endef
$(foreach LOADABLE_MODULE, $(LOADABLE_MODULES), $(eval $(call LOADABLE_MODULE_template,$(LOADABLE_MODULE))))

repl-%: $(BUILDDIR)/$(REPLDIR)/%.o $(SERVER_OBJS)
	$(CC) $(MODULE_LDFLAGS) -o $@ $^

clean:
	rm -rf $(BUILDDIR) *.so $(REPL_ELFS)

.PHONY: all all-modules repl-elfs clean FORCE

