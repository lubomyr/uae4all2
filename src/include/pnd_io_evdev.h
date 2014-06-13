
#ifndef h_pnd_io_evdev_h
#define h_pnd_io_evdev_h

// some handy routines for open/close/monitor of the evdev controller devices ..
// ie: so you can watch the analogs and d-pads pretty easily, for when you don't
// want to rely on SDL or whatever
//
// this technique works fine for nubs, d-pads, even keyboard and various buttons
// this API may only handle some parts for now, but you coudl easily duplicate some of the
// code to extend for adding keyboard.
//
// dpad, nubs, start/select/pandora, triggers all work fine as of Mar 2010.
//

// can get analog nubs, d-pads, or even keyboard A-Z type keys here
// some special ones like Power are on other devices than you'd expect, but all good

typedef enum {
  pnd_evdev_dpads = 0,   // for d-pad and d-pad-buttons
  pnd_evdev_nub1,
  pnd_evdev_nub2,
  pnd_evdev_power,
  pnd_evdev_max
} pnd_evdev_e;

unsigned char pnd_evdev_open ( pnd_evdev_e device ); // returns 0 on error, >0 success
void pnd_evdev_close ( pnd_evdev_e device );
void pnd_evdev_closeall ( void );
int pnd_evdev_get_fd ( unsigned char handle ); // obtain actual fd from handle
int pnd_evdev_open_by_name ( char *devname ); // internal but handy; see device names in pnd_device.h

typedef enum {
  pnd_evdev_left = (1<<0),     // these are bitmask; ex: (pnd_evdev_left | pnd_evdev_up)
  pnd_evdev_right = 1<<1,
  pnd_evdev_up = 1<<2,
  pnd_evdev_down = 1<<3,
  pnd_evdev_x = 1<<4,
  pnd_evdev_y = 1<<5,
  pnd_evdev_a = 1<<6,
  pnd_evdev_b = 1<<7,
  pnd_evdev_ltrigger = 1<<8,
  pnd_evdev_rtrigger = 1<<9,
  pnd_evdev_start = 1<<10,
  pnd_evdev_select = 1<<11,
  pnd_evdev_pandora = 1<<12
} pnd_evdev_dpad_e;

typedef struct {
  int x;
  int y;
} pnd_nubstate_t;

// catchup() - catch up any pending events
// return 0 if something weird happened, like device error; in that case, closeall and reopen?
// return 1 if looks like state is all up to date now (ie: no more events)
unsigned char pnd_evdev_catchup ( unsigned char blockp ); // will do all open devices

// fetch dpad state -- a mask of what buttons are pressed currently
// return -1 if device not open
unsigned int pnd_evdev_dpad_state ( pnd_evdev_e device ); // returns bitmask of pnd_evdev_dpad_e

// try to obtain X/Y axis for the requested nub
// r_nubstate best not be null or the behaviour is undefined. (Well, it is defined .. *catch fire*)
// return 1 when state is copied over
// return -1 when device not opened
int pnd_evdev_nub_state ( pnd_evdev_e nubdevice, pnd_nubstate_t *r_nubstate );

#endif
