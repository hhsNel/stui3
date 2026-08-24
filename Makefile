CC ?= cc
ABI_VERSION := 2

-include config.mk

SRCDIR := src
MODULEDIR := modules
INCLUDEDIR := include
REPLDIR := repls
BUILDDIR := build

CFLAGS := -std=c99 -DABI_VERSION=$(ABI_VERSION) -I$(SRCDIR) -I$(INCLUDEDIR) -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Werror
LDFLAGS := 

SRCS := $(shell find $(SRCDIR) -name "*.c")
REPL_SRCS := $(shell find $(REPLDIR) -name "*.c")

OBJS := $(SRCS:%.c=$(BUILDDIR)/%.o)
MODULE_SOS := 
REPL_OBJS := $(REPL_SRCS:%.c=$(BUILDDIR)/%.o)
REPL_ELFS := $(REPL_SRCS:$(REPLDIR)/%.c=repl-%)

BUILTIN_MODULES :=
LOADABLE_MODULES :=
CONFIG :=

ifeq ($(MODULE_DUMMY),builtin)
  BUILTIN_MODULES += dummy
  CFLAGS += -DBUILTIN_DUMMY
  CONFIG += Y
else ifeq ($(MODULE_DUMMY),module)
  LOADABLE_MODULES += dummy
  CONFIG += M
else
  CONFIG += N
endif
ifeq ($(MODULE_DUMMY2),builtin)
  BUILTIN_MODULES += dummy2
  CFLAGS += -DBUILTIN_DUMMY2
  CONFIG += Y
else ifeq ($(MODULE_DUMMY2),module)
  LOADABLE_MODULES += dummy2
  CONFIG += M
else
  CONFIG += N
endif

BUILTIN_MODULE_SRCS := $(foreach BUILTIN_MODULE, $(BUILTIN_MODULES), $(shell find $(MODULEDIR)/$(BUILTIN_MODULE) -name "*.c"))
SRCS += $(BUILTIN_MODULE_SRCS)
OBJS += $(BUILTIN_MODULE_SRCS:$(MODULEDIR)/%.c=$(BUILDDIR)/builtin/%.o)
CFLAGS += -DALL_BUILTIN_MODULES="$(subst $(space),$(comma),$(BUILTIN_MODULES))"
MODULE_SOS += $(foreach LOADABLE_MODULE, $(LOADABLE_MODULES), $(LOADABLE_MODULE).so)

all: $(ALL_CLEAN_DEP) all-modules repl-elfs

.config: FORCE
	@if [ "$$(cat $@ 2>/dev/null || true)" != "$(CONFIG)" ]; then \
		echo "$(CONFIG)" > $@; \
	fi

all-modules: $(MODULE_SOS)

repl-elfs: $(REPL_ELFS)

$(BUILDDIR)/builtin/%.o: $(MODULEDIR)/%.c .config
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -DMODULE_BUILTIN=1 -DMODULE_LOADABLE=0 -c -o $@ $<

$(BUILDDIR)/loadable/%.o: $(MODULEDIR)/%.c .config
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -DMODULE_BUILTIN=0 -DMODULE_LOADABLE=1 -fPIC -c -o $@ $<

$(BUILDDIR)/%.o: %.c .config
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

define LOADABLE_MODULE_template
$(1).so: $(patsubst $(MODULEDIR)/%.c, $(BUILDDIR)/loadable/%.o, $(shell find $(MODULEDIR)/$(1) -name "*.c"))
	$(CC) $(LDFLAGS) -shared -o $$@ $$^
endef
$(foreach LOADABLE_MODULE, $(LOADABLE_MODULES), $(eval $(call LOADABLE_MODULE_template,$(LOADABLE_MODULE))))

repl-%: $(BUILDDIR)/$(REPLDIR)/%.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -rf $(BUILDDIR) $(MODULE_SOS) $(REPL_ELFS)

.PHONY: all all-modules repl-elfs clean FORCE

