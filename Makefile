CC ?= cc
ABI_VERSION := 6

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

GLOBAL_CFLAGS := -std=c99 -DABI_VERSION=$(ABI_VERSION) -I$(INCLUDEDIR) -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE -Wall -Wextra -Wpedantic -Werror
GLOBAL_LDFLAGS := 

SERVER_SRCS := $(shell find $(SERVERDIR) -name "*.c")
LIB_SRCS := $(shell find $(LIBDIR) -name "*.c")
COMMON_SRCS := $(shell find $(COMMONDIR) -name "*.c")
REPL_SRCS := $(shell find $(REPLDIR) -name "*.c")

SERVER_OBJS := $(SERVER_SRCS:%.c=$(BUILDDIR)/%.o)
LIB_OBJS := $(LIB_SRCS:%.c=$(BUILDDIR)/%.o)
COMMON_OBJS := $(COMMON_SRCS:%.c=$(BUILDDIR)/%.o)
MODULE_SOS := 
REPL_OBJS := $(REPL_SRCS:%.c=$(BUILDDIR)/%.o)
REPL_ELFS := $(REPL_SRCS:$(REPLDIR)/%.c=repl-%)

BUILTIN_MODULES :=
LOADABLE_MODULES :=
CONFIG :=

define MODULE
ifeq ($(2),builtin)
  BUILTIN_MODULES += $(1)
  CONFIG += Y
else ifeq ($(2),module)
  LOADABLE_MODULES += $(1)
  CONFIG += M
else
  CONFIG += N
endif
endef

-include config.mk

DEBUG ?= 0
ifeq ($(DEBUG),1)
  GLOBAL_CFLAGS += -g
  GLOBAL_LDFLAGS += -g
endif

SERVER_CFLAGS := $(GLOBAL_CFLAGS) -I$(SERVERDIR) -I$(COMMONDIR)
LIB_CFLAGS := $(GLOBAL_CFLAGS) -I$(LIBDIR) -I$(COMMONDIR)
COMMON_CFLAGS := $(GLOBAL_CFLAGS) -I$(COMMONDIR)
MODULE_CFLAGS := $(GLOBAL_CFLAGS) -I$(COMMONDIR) -I$(SERVERDIR)
SERVER_LDFLAGS := $(GLOBAL_LDFLAGS)
LIB_LDFLAGS := $(GLOBAL_LDFLAGS) -shared
COMMON_LDFLAGS := $(GLOBAL_LDFLAGS)
MODULE_LDFLAGS := $(GLOBAL_LDFLAGS) -shared

BUILTIN_MODULE_SRCS := $(foreach BUILTIN_MODULE, $(BUILTIN_MODULES), $(shell find $(MODULEDIR)/$(BUILTIN_MODULE) -name "*.c"))
SERVER_SRCS += $(BUILTIN_MODULE_SRCS)
SERVER_OBJS += $(BUILTIN_MODULE_SRCS:$(MODULEDIR)/%.c=$(BUILDDIR)/builtin/%.o)
ifneq ($(BUILTIN_MODULES),)
  SERVER_CFLAGS += -DALL_BUILTIN_MODULES="$(subst $(space),$(comma),$(BUILTIN_MODULES))"
  SERVER_CFLAGS += -DBUILTIN_MODULES_ENABLED
endif
MODULE_SOS += $(foreach LOADABLE_MODULE, $(LOADABLE_MODULES), $(LOADABLE_MODULE).so)

all: stui3-server libstui3.so all-modules repl-elfs

.config: FORCE
	@if [ "$$(cat $@ 2>/dev/null || true)" != "$(CONFIG)" ]; then \
		echo "$(CONFIG)" > $@; \
	fi

stui3-server: $(SERVER_OBJS) $(COMMON_OBJS)
	$(CC) $(SERVER_LDFLAGS) -o $@ $^

libstui3.so: $(LIB_OBJS) $(COMMON_OBJS)
	$(CC) $(LIB_LDFLAGS) -o $@ $^

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
	$(CC) $(MODULE_LDFLAGS) -o $$@ $$^
endef
$(foreach LOADABLE_MODULE, $(LOADABLE_MODULES), $(eval $(call LOADABLE_MODULE_template,$(LOADABLE_MODULE))))

repl-%: $(BUILDDIR)/$(REPLDIR)/%.o $(filter-out $(BUILDDIR)/$(SERVERDIR)/main.o, $(SERVER_OBJS)) $(COMMON_OBJS) libstui3.so
	$(CC) $(GLOBAL_LDFLAGS) -L. -lstui3 -Wl,-rpath,'$$ORIGIN' -o $@ $^

clean:
	rm -rf $(BUILDDIR) stui3-server *.so $(REPL_ELFS)

.PHONY: all all-modules repl-elfs clean FORCE

