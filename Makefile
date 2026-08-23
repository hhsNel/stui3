CC ?= cc
ABI_VERSION := 1

SRCDIR := src
MODULEDIR := modules
INCLUDEDIR := include
REPLDIR := repls
BUILDDIR := build

CFLAGS := -std=c99 -DABI_VERSION=$(ABI_VERSION) -I$(SRCDIR) -I$(INCLUDEDIR) -D_POSIX_C_SOURCE=200809L
LDFLAGS := 
SRCS := $(shell find $(SRCDIR) -name "*.c")
OBJS := $(SRCS:%.c=$(BUILDDIR)/%.o)
REPL_SRCS := $(shell find $(REPLDIR) -name "*.c")
REPL_OBJS := $(REPL_SRCS:%.c=$(BUILDDIR)/%.o)
REPL_ELFS := $(REPL_SRCS:$(REPLDIR)/%.c=repl-%)

all: repl-elfs

repl-elfs: $(REPL_ELFS)

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $^

repl-%: $(BUILDDIR)/$(REPLDIR)/%.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -rf $(BUILDDIR) $(REPL_ELFS)

.PHONY: all repl-elfs clean

