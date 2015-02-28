 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Interface to the Tcl/Tk GUI
  *
  * Copyright 1996 Bernd Schmidt
  */

extern int gui_init (void);
extern int gui_update (void);
extern void gui_exit (void);
extern void gui_led (int, int);
extern void gui_handle_events (void);
extern void gui_purge_events (void);
extern void gui_filename (int, const char *);
extern void gui_fps (int fps);
extern void gui_lock (void);
extern void gui_unlock (void);
extern void gui_set_message(const char *msg, int t);
extern void gui_show_window_bar(int per, int max, int case_title);

extern unsigned int gui_ledstate;
extern int show_message;
extern int no_gui;
extern char *show_message_str;
#define HDLED_OFF		0
#define HDLED_READ		1
#define HDLED_WRITE		2
#define HDLED_TIMEOUT	3	/* HD LED off delay [frames] */

struct gui_info
{
    uae_u8 drive_motor[4];          /* motor on off */
    uae_u8 drive_track[4];          /* rw-head track */
    uae_u8 drive_writing[4];        /* drive is writing */
    uae_u8 powerled;                /* state of power led */
    uae_u8 hdled;					/* state of hard disk access */
    uae_u16 fps;
};

#ifndef _GUI_CPP
extern struct gui_info gui_data;

extern char uae4all_image_file0[256];
extern char uae4all_image_file1[256];
extern char uae4all_image_file2[256];
extern char uae4all_image_file3[256];

extern char uae4all_hard_dir[256];
extern char uae4all_hard_file[256];

#endif

/* Functions to be called when prefs are changed by non-gui code.  */
extern void uae4all_update_time(void);
extern void uae4all_show_time(void);
