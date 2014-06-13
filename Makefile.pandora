PREFIX	=/usr

#SDL_BASE = $(PREFIX)/bin/
SDL_BASE = 
MORE_CFLAGS += -DGP2X -DPANDORA -DDOUBLEBUFFER -DUSE_ARMNEON -DUSE_ARMV7 -DUSE_SDLSOUND

NAME   = uae4all
O      = o
RM     = rm -f
#CC     = gcc
#CXX    = g++
#STRIP  = strip
#AS     = as

PROG   = $(NAME)

all: $(PROG)

FAME_CORE=1
FAME_CORE_C=1
GUICHAN_GUI=1
PANDORA=1

DEFAULT_CFLAGS = `$(SDL_BASE)sdl-config --cflags`
#LDFLAGS = -lSDL -lpthread  -lz -lSDL_image -lpng12 -L$(C64_TOOLS_ROOT)/libc64 -lc64
LDFLAGS = -lSDL -lpthread  -lz -lSDL_image -lpng12

MORE_CFLAGS +=   -Isrc -Isrc/gp2x -Isrc/menu -Isrc/include -Isrc/gp2x/menu -fomit-frame-pointer -Wno-unused -Wno-format -DUSE_SDL -DGCCCONSTFUNC="__attribute__((const))" -DUSE_UNDERSCORE -DUNALIGNED_PROFITABLE -DOPTIMIZED_FLAGS -DSHM_SUPPORT_LINKS=0 -DOS_WITHOUT_MEMORY_MANAGEMENT
ifdef GUICHAN_GUI
LDFLAGS +=  -lSDL_ttf -lguichan_sdl -lguichan
MORE_CFLAGS += -fexceptions
else
MORE_CFLAGS += -fno-exceptions
endif


MORE_CFLAGS += -DROM_PATH_PREFIX=\"./\" -DDATA_PREFIX=\"./data/\" -DSAVE_PREFIX=\"./saves/\"

MORE_CFLAGS += -msoft-float -ffast-math
ifndef DEBUG
MORE_CFLAGS += -O3
MORE_CFLAGS += -fstrict-aliasing -mstructure-size-boundary=32 -fexpensive-optimizations
MORE_CFLAGS += -fweb -frename-registers -fomit-frame-pointer
#MORE_CFLAGS += -falign-functions=32 -falign-loops -falign-labels -falign-jumps
MORE_CFLAGS += -falign-functions=32
MORE_CFLAGS += -finline -finline-functions -fno-builtin
#MORE_CFLAGS += -S
else
MORE_CFLAGS += -ggdb
endif

ASFLAGS += -mfloat-abi=soft

MORE_CFLAGS+= -DUSE_AUTOCONFIG
MORE_CFLAGS+= -DUSE_ZFILE
# Turrican3 becomes unstable if this is not enabled
MORE_CFLAGS+= -DSAFE_MEMORY_ACCESS
#MORE_CFLAGS+= -DDEBUG_SAVESTATE

CFLAGS  = $(DEFAULT_CFLAGS) $(MORE_CFLAGS)

OBJS =	\
	src/audio.o \
	src/autoconf.o \
	src/blitfunc.o \
	src/blittable.o \
	src/blitter.o \
	src/cfgfile.o \
	src/cia.o \
	src/savedisk.o \
	src/savestate.o \
	src/custom.o \
	src/disk.o \
	src/drawing.o \
	src/ersatz.o \
	src/expansion.o \
	src/filesys.o \
	src/fsdb.o \
	src/fsdb_unix.o \
	src/fsusage.o \
	src/gfxutil.o \
	src/hardfile.o \
	src/keybuf.o \
	src/main.o \
	src/memory.o \
	src/missing.o \
	src/native2amiga.o \
	src/neon_helper.o \
	src/gui.o \
	src/od-joy.o \
	src/scsi-none.o \
	src/sound_gp2x.o \
	src/sdlgfx.o \
	src/writelog.o \
	src/zfile.o \
	src/menu/fade.o \
	src/gp2x/memcpy.o \
	src/gp2x/memset.o \
	src/gp2x/gp2x.o \
	src/gp2x/inputmode.o \
	src/gp2x/menu/menu_helper.o \
	src/gp2x/menu/menu_config.o \
	src/gp2x/menu/menu.o
ifdef GUICHAN_GUI
CFLAGS+= -DUSE_GUICHAN
OBJS += src/menu_guichan/menu_guichan.o \
	src/menu_guichan/menuTabMain.o \
	src/menu_guichan/menuTabFloppy.o \
	src/menu_guichan/menuTabHD.o \
	src/menu_guichan/menuTabDisplaySound.o \
	src/menu_guichan/menuTabSavestates.o \
	src/menu_guichan/menuTabControl.o \
	src/menu_guichan/menuTabCustomCtrl.o \
	src/menu_guichan/menuMessage.o \
	src/menu_guichan/menuLoad_guichan.o \
	src/menu_guichan/menuConfigManager.o \
	src/menu_guichan/uaeradiobutton.o \
	src/menu_guichan/uaedropdown.o
ifdef PANDORA
OBJS += src/menu_guichan/sdltruetypefont.o
endif
else
OBJS += src/gp2x/menu/menu_fileinfo.o \
	src/gp2x/menu/menu_load.o \
	src/gp2x/menu/menu_main.o \
	src/gp2x/menu/menu_savestates.o \
	src/gp2x/menu/menu_misc.o \
	src/gp2x/menu/menu_controls.o \
	src/gp2x/menu/menu_display.o \
	src/gp2x/menu/menu_memory_disk.o
endif


CFLAGS+= -DUSE_FAME_CORE
CFLAGS+= -DWITH_TESTMODE

src/m68k/fame/famec.o: src/m68k/fame/famec.cpp
OBJS += src/m68k/fame/famec.o
OBJS += src/m68k/fame/m68k_intrf.o
OBJS += src/m68k/m68k_cmn_intrf.o

CPPFLAGS  = $(CFLAGS)

src/neon_helper.o: src/neon_helper.s
	$(CXX) -O3 -pipe -falign-functions=32 -march=armv7-a -mcpu=cortex-a8 -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp -Wall -o src/neon_helper.o -c src/neon_helper.s

$(PROG): $(OBJS)
	$(CXX) $(CFLAGS) -o $(PROG) $(OBJS) $(LDFLAGS)
ifndef DEBUG
	$(STRIP) $(PROG)
endif

clean:
	$(RM) $(PROG) $(OBJS)
