EE_BIN  = fontm_scroll.elf
EE_OBJS = fontm_scroll.o

all: $(EE_BIN)

test: all
	ps2client -h $(PS2_IP) -t 1 execee host:fontm_scroll.elf

reset: clean
	ps2client -h $(PS2_IP) reset

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

install:

include ../../Makefile.pref
include ../../Makefile.global
