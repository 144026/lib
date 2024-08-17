DIRS := .
SRCS := $(foreach dir,$(DIRS),$(wildcard $(dir)/*.c))
HEADERS := $(foreach dir,$(DIRS),$(wildcard $(dir)/*.h))

CFLAGS := $(foreach dir,$(DIRS),-I$(dir))
CFLAGS += -fcommon -Wall -g -ggdb
OBJS := $(foreach src,$(SRCS),$(patsubst %.c,%.o,$(src)))

LDFLAGS := -lpthread
PROGS := testlib

Q = @
quiet = quiet

ifeq ($(origin V),command line)
ifneq ($(V),)
Q =
quiet =
endif
endif

cmd_run_cc = $Q$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<
cmd_cc_quiet = @echo "  CC      $@"

cmd_run_ld = $Q$(CC) -o $@ $^ $(LDFLAGS)
cmd_ld_quiet = @echo "  LD      $@"

cmd_run_rm = $Q$(RM) $1
cmd_rm_quiet = @echo "  RM      $1"

define cmd
$(call cmd_run_$1,$2)
$(call cmd_$1_$(quiet),$2)
endef


all: CFLAGS += -O2
all: $(PROGS)

static: CFLAGS += -O2
static: LDFLAGS += -static
static: $(PROGS)

debug: CFLAGS += -O0 -fsanitize=address -fno-omit-frame-pointer
debug: LDFLAGS += -fsanitize=address
debug: $(PROGS)

$(OBJS): %.o : %.c $(HEADERS) Makefile
	$(call cmd,cc)

clean:
	$(call cmd,rm,$(PROGS))
	$(call cmd,rm,$(OBJS))

.PHONY: all static debug clean


testlib: $(OBJS)
	$(call cmd,ld)

