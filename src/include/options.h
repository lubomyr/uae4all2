 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Stuff
  *
  * Copyright 1995, 1996 Ed Hanway
  * Copyright 1995-2001 Bernd Schmidt
  */

#define UAEMAJOR 0
#define UAEMINOR 8
#define UAESUBREV 22

extern long int version;

/**************************************************************
 * UAE4ALL header
 **************************************************************/
extern int produce_sound;
extern int changed_produce_sound;
extern char prefs_df[4][256];
extern char changed_df[4][256];
extern int real_changed_df[4];
extern char romfile[256];
extern char romkeyfile[256]; 
extern char extfile[256];
extern int prefs_cpu_model;
extern unsigned prefs_chipmem_size;
extern unsigned prefs_bogomem_size;
extern unsigned prefs_fastmem_size;
extern int prefs_gfx_framerate, changed_gfx_framerate;
extern const char *kickstarts_rom_names[5];
extern const char *extended_rom_names[5];
#ifdef ANDROIDSDL
extern const char *af_kickstarts_rom_names[4]; 
#endif


#define PREFS_GFX_WIDTH 320
#define PREFS_GFX_HEIGHT 240

#define PREFS_GFX_SCALED_WIDTH 328
#define PREFS_GFX_SCALED_HEIGHT 256

typedef enum { DRV_NONE = -1, DRV_35_DD = 0, DRV_35_HD, DRV_525_SD, DRV_35_DD_ESCOM } drive_type;

/**************************************************************/ 

struct uaedev_mount_info;

struct uae_prefs {
    unsigned int chipset_mask;

    int cpu_level;

#if !( defined(PANDORA) || defined(ANDROIDSDL) )
    uae_u32 z3fastmem_size;
#endif
    uae_u32 fastmem_size;
#if !( defined(PANDORA) || defined(ANDROIDSDL) )
    uae_u32 gfxmem_size;
#endif

    struct uaedev_mount_info *mountinfo;
};

extern void default_prefs_uae (struct uae_prefs *);
extern void discard_prefs_uae (struct uae_prefs *);

extern void check_prefs_changed_custom (void);
extern void check_prefs_changed_audio (void);

extern struct uae_prefs currprefs, changed_prefs;

#define MAX_COLOR_MODES 5

/* #define NEED_TO_DEBUG_BADLY */

#if !defined(USER_PROGRAMS_BEHAVE)
#define USER_PROGRAMS_BEHAVE 0
#endif
