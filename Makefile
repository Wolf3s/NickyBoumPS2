EE_BIN  = nickyPS2.elf

EE_OBJS =  fileio_std.o game.o input.o main.o mixer.o op_action.o op_anim_helper.o op_collide.o op_logic.o \
           player_mod.o resource.o scaler.o sequence.o sound.o sqx_decoder.o staticres.o systemstub_sdl.o util.o

EE_INCS = -I$(PS2SDK)/ports/include \
          -I$(PS2SDK)/ports/include/SDL

EE_CFLAGS = -DNICKY_SDL_VERSION -Wall -Wno-uninitialized  -ffast-math  -DNOMUSIC -DUSE_RWOPS    #-DNICKY_DEBUG -DUSE_RWOPS -DHAVE_MIXER

EE_LDFLAGS = -L$(PS2SDK)/ports/lib
EE_LIBS = -lstdc++ -lsdlmixer -lSDL_image -lpng -lz -ljpeg -lSDL_gfx -lsdl -lsdlmain -lm -lmc -lc -ldebug

all: $(EE_BIN)

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

run:
	ps2client execee host:$(EE_BIN)

reset:
	ps2client reset

include Makefile.romfs
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
