CC = gcc-11
LD = $(CC) $(KBUILD_CFLAGS) $(KBUILD_LDLAGS) -pthread -lpthread

NOSTDINC_FLAGS :=
LINUXINCLUDE := -I$(src)/include
KBUILD_LDFLAGS := -Wl,--Ttext-segment=0x20000000
KBUILD_CFLAGS := -g -O3 -Wall -Werror -D__LINUX_COMPILER_TYPES_H -muintr -mgeneral-regs-only -minline-all-stringops -static

always-y := hello
subdir-y := libs/

ULIBS := $(obj)/libs/ulib.a

$(obj)/%: $(obj)/%.o $(ULIBS) FORCE
	$(call if_changed,ld)
