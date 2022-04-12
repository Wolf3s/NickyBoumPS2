/*
 * Adapted by cosmito @ 2010 from:
 * Nicky - Nicky Boum engine rewrite
 * Copyright (C) 2006-2007 Gregory Montoir
 */
#include "game.h"
#include "sound.h"
#include "input.h"
#include "fileio.h"
#include "systemstub.h"

#include <SDL_mixer.h>

#include <SDL/SDL.h>
#include <SDL/SDL_endian.h>
#include <SDL_mixer.h>
#include <romfs_io.h>
#include <debug.h>
#include "libpad.h"
#include <sifrpc.h>
#include <kernel.h>

// cosmito : If == 1, only music and no sfx will play.
int _sfxMute = 1;

static const char *USAGE = 
	"Usage: %s [OPTIONS]...\n"
	"  --datapath=PATH   Path to data files (default 'DATA')\n"
	"  --level=NUM       Start at level NUM\n"
	"  --record_input    Record inputs to NICKY-LEVEL%%d.COD\n"
	"  --replay_input    Read inputs from NICKY-LEVEL%%d.COD";

static char padBuf[256] __attribute__((aligned(64)));
static char actAlign[6];
static int actuators;
int initializePad(int port, int slot);
int waitPadReady(int port, int slot);
int padUtils_ReadButtonWait(int port, int slot, u32 old_pad, u32 new_pad);
int padUtils_ReadButton(int port, int slot, u32 old_pad, u32 new_pad);

static int parse_option(const char *arg, const char *long_cmd, const char **opt) {
	if (strlen(arg) > 2 && arg[0] == '-' && arg[1] == '-') {
		if (strncmp(arg, long_cmd, strlen(long_cmd)) == 0) {
			*opt = arg + strlen(long_cmd);
			return 1;
		}
	}
	return 0;
}

static int parse_flag(const char *arg, const char *long_cmd, int *flag) {
	if (strlen(arg) > 2 && arg[0] == '-' && arg[1] == '-') {
		if (strncmp(arg, long_cmd, strlen(long_cmd)) == 0) {
			*flag = 1;
			return 1;
		}
	}
	return 0;
}

game_version_e detected_game_version;

static void detect_game_version() {
	if (fio_exists("REF1.REF")) {
		detected_game_version = GAME_VER_NICKY1;
	} else if (fio_exists("REF1REF.SQX")) {
		detected_game_version = GAME_VER_NICKY2;
	}
}

static void print_format()
{
    int audio_channels;
    int audio_rate;
    Uint16 audio_format;

    Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
    printf("Opened audio at %d Hz %d bit %s\n", audio_rate,
        (audio_format & 0xFF),
        (audio_channels > 2) ? "surround" : (audio_channels > 1) ? "stereo" : "mono");
}


void SelectMusicOrSfx() 
{
    int i;
    i = SifLoadModule("rom0:XSIO2MAN", 0, NULL);
    if (i < 0) {
        printf("sifLoadModule sio failed: %d\n", i);
        SleepThread();
    }
    i = SifLoadModule("rom0:XPADMAN", 0, NULL);
    if (i < 0) {
        printf("sifLoadModule pad failed: %d\n", i);
        SleepThread();
    }

    printf("padInit(0);\n");
    padInit(0);

    int port = 0; // 0 -> Connector 1, 1 -> Connector 2
    printf("int port = 0;\n");

    int slot = 0; // Always zero if not using multitap

    printf("PortMax: %d\n", padGetPortMax());
    printf("SlotMax: %d\n", padGetSlotMax(port));

    if((i = padPortOpen(port, slot, padBuf)) == 0) {
        printf("padOpenPort failed: %d\n", i);
        SleepThread();
    }

    if(!initializePad(port, slot)) {
        printf("pad initalization failed!\n");
        SleepThread();
    }

    init_scr();
    scr_printf("\n\n  NickyBoumPS2 v.0.2.0a by cosmito @ 2010\n");
    scr_printf("  Adapted from NickyBoumPSP by deniska\n");
    scr_printf("  Adapted from Nicky - Nicky Boum engine rewrite (C) 2006-2007 Gregory Montoir\n\n\n");
    scr_printf("  Select 'X' for Music or 'O' for SFX\n\n  ");
    
    int butres = 0;
    u32 old_pad = 0;
    u32 new_pad = 0;

    int timeout = 1000000;
    int wait_for_user = 0;
    while(timeout != 0 || wait_for_user == 1)
    {
        butres = padUtils_ReadButtonWait(port, slot, old_pad, new_pad);
        if(butres != 0)
        {
            wait_for_user = 1;
        }

        if(butres == PAD_CROSS)
        {
            scr_printf("Music only selected\n\n");
            _sfxMute = 1;
            break;
        }
        else if(butres == PAD_CIRCLE)
        {
            scr_printf("SFX selected\n\n");
            _sfxMute = 0;
            break;
        }
        if(timeout != 0)
            timeout--;
    }

    padPortClose(port, slot);
}

#undef main
int SDL_main( int argc, char *argv[] ) {
//int main(int argc, char *argv[]) {
	int i, level_num;
	const char *game_title = "";
    //const char *data_path = "host:data";
    const char *data_path = "data";
	const char *level_num_str = "0";
	int record_input_flag = 0;
	int replay_input_flag = 0;
	input_recording_state_e rec_state = IRS_NONE;

    // romfs
    rioInit();

	/* parse arguments */
	for (i = 1; i < argc; ++i) {
		int ret = 0;
		ret |= parse_option(argv[i], "--datapath=", &data_path);
		ret |= parse_option(argv[i], "--level=", &level_num_str);
		ret |= parse_flag(argv[i], "--record_input", &record_input_flag);
		ret |= parse_flag(argv[i], "--replay_input", &replay_input_flag);
		if (!ret) {
			printf(USAGE, argv[0]);
			return 0;
		}
	}
	if (record_input_flag) {
		rec_state = IRS_RECORD;
	}
	if (replay_input_flag) {
		rec_state = IRS_REPLAY;
	}
	level_num = atoi(level_num_str);
	if (level_num >= 8) {
		level_num = 0;
	}

    SelectMusicOrSfx();

	/* start game */
#ifdef NICKY_DEBUG
	util_debug_mask = DBG_GAME | DBG_RESOURCE | DBG_SOUND | DBG_MODPLAYER | DBG_INPUT;
#else
	util_debug_mask = 0;
#endif
	fio_init(data_path);
	detect_game_version();
	if (NICKY1) {
		game_title = "Nicky Boum";
	}
	if (NICKY2) {
		game_title = "Nicky 2";
	}
    
    //SDL_SYS_TimerInit();        // cosmito : not needed if SDL_INIT_TIMER flag is used at SDL_Init
	
    snd_init();
	sys_init(GAME_SCREEN_W, GAME_SCREEN_H, game_title);	
    inp_init(rec_state);
	game_init();
	game_run(level_num);
	game_destroy();
	sys_destroy();
	snd_stop();
	return 0;
}


int waitPadReady(int port, int slot)
{
    int state;
    int lastState;
    char stateString[16];

    state = padGetState(port, slot);
    lastState = -1;
    while((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1)) {
        if (state != lastState) {
            padStateInt2String(state, stateString);
            printf("Please wait, pad(%d,%d) is in state %s\n", 
                port, slot, stateString);
        }
        lastState = state;
        state=padGetState(port, slot);
    }
    // Were the pad ever 'out of sync'?
    if (lastState != -1) {
        printf("Pad OK!\n");
    }
    return 0;
}

int initializePad(int port, int slot)
{
    int ret;
    int modes;
    int i;

    waitPadReady(port, slot);

    // How many different modes can this device operate in?
    // i.e. get # entrys in the modetable
    modes = padInfoMode(port, slot, PAD_MODETABLE, -1);
    printf("The device has %d modes\n", modes);

    if (modes > 0) {
        printf("( ");
        for (i = 0; i < modes; i++) {
            printf("%d ", padInfoMode(port, slot, PAD_MODETABLE, i));
        }
        printf(")");
    }

    printf("It is currently using mode %d\n", 
        padInfoMode(port, slot, PAD_MODECURID, 0));

    // If modes == 0, this is not a Dual shock controller 
    // (it has no actuator engines)
    if (modes == 0) {
        printf("This is a digital controller?\n");
        return 1;
    }

    // Verify that the controller has a DUAL SHOCK mode
    i = 0;
    do {
        if (padInfoMode(port, slot, PAD_MODETABLE, i) == PAD_TYPE_DUALSHOCK)
            break;
        i++;
    } while (i < modes);
    if (i >= modes) {
        printf("This is no Dual Shock controller\n");
        return 1;
    }

    // If ExId != 0x0 => This controller has actuator engines
    // This check should always pass if the Dual Shock test above passed
    ret = padInfoMode(port, slot, PAD_MODECUREXID, 0);
    if (ret == 0) {
        printf("This is no Dual Shock controller??\n");
        return 1;
    }

    printf("Enabling dual shock functions\n");

    // When using MMODE_LOCK, user cant change mode with Select button
    padSetMainMode(port, slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);

    waitPadReady(port, slot);
    printf("infoPressMode: %d\n", padInfoPressMode(port, slot));

    waitPadReady(port, slot);        
    printf("enterPressMode: %d\n", padEnterPressMode(port, slot));

    waitPadReady(port, slot);
    actuators = padInfoAct(port, slot, -1, 0);
    printf("# of actuators: %d\n",actuators);

    if (actuators != 0) {
        actAlign[0] = 0;   // Enable small engine
        actAlign[1] = 1;   // Enable big engine
        actAlign[2] = 0xff;
        actAlign[3] = 0xff;
        actAlign[4] = 0xff;
        actAlign[5] = 0xff;

        waitPadReady(port, slot);
        printf("padSetActAlign: %d\n", 
            padSetActAlign(port, slot, actAlign));
    }
    else {
        printf("Did not find any actuators.\n");
    }

    waitPadReady(port, slot);

    return 1;
}

int padUtils_ReadButtonWait(int port, int slot, u32 old_pad, u32 new_pad)
{
    int butres = 0, read = 0;
    read = padUtils_ReadButton(port, slot, old_pad, new_pad);

    if(read != 0)
    {
        butres = read;      // memorize pressed button
        while (padUtils_ReadButton(port, slot, old_pad, new_pad) != 0) {};
    }
    return butres;
}

int padUtils_ReadButton(int port, int slot, u32 old_pad, u32 new_pad)
{
    struct padButtonStatus buttons;
    int ret;
    u32 paddata;

    ret = padRead(port, slot, &buttons);
    if (ret != 0)
    {
        paddata = 0xffff ^ buttons.btns;

        new_pad = paddata & ~old_pad;
        old_pad = paddata;


        if (new_pad & PAD_LEFT)
        {
            return(PAD_LEFT);
        }
        if (new_pad & PAD_DOWN)
        {
            return(PAD_DOWN);
        }
        if (new_pad & PAD_RIGHT)
        {
            return(PAD_RIGHT);
        }
        if (new_pad & PAD_UP)
        {
            return(PAD_UP);
        }
        if (new_pad & PAD_START)
        {
            return(PAD_START);
        }
        if (new_pad & PAD_R3)
        {
            return(PAD_R3);
        }
        if (new_pad & PAD_L3)
        {
            return(PAD_L3);
        }
        if (new_pad & PAD_SELECT)
        {
            return(PAD_SELECT);
        }
        if (new_pad & PAD_SQUARE)
        {
            return(PAD_SQUARE);
        }
        if (new_pad & PAD_CROSS)
        {
            return(PAD_CROSS);
        }
        if (new_pad & PAD_CIRCLE)
        {
            return(PAD_CIRCLE);
        }
        if (new_pad & PAD_TRIANGLE)
        {
            return(PAD_TRIANGLE);
        }
        if (new_pad & PAD_R1)
        {
            return(PAD_R1);
        }
        if (new_pad & PAD_L1)
        {
            return(PAD_L1);
        }
        if (new_pad & PAD_R2)
        {
            return(PAD_R2);
        }
        if (new_pad & PAD_L2)
        {
            return(PAD_L2);
        }
    }
    else
        return -1;

    return 0;   // 0 means no button was pressed
}
