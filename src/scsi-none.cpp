#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "thread.h"
#include "options.h"
#include "memory-uae.h"
#include "custom.h"
#include "m68k/m68k_intrf.h"
#include "disk.h"
#include "autoconf.h"
#include "filesys.h"
#include "execlib.h"
#include "scsidev.h"

uaecptr scsidev_startup (uaecptr resaddr) { return resaddr; }
void scsidev_install (void) {}
void scsidev_reset (void) {}
void scsidev_start_threads (void) {}

