 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Config file handling
  * This still needs some thought before it's complete...
  *
  * Copyright 1998 Brian King, Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>

#include "config.h"
#include "options.h"
#include "thread.h"
#include "uae.h"
#include "autoconf.h"
#include "gui.h"


char * make_hard_dir_cfg_line (char *dst) {
	char buffer[256];
	int i;
	
	if (uae4all_hard_dir[0] != '\0') {
		for (i = strlen(uae4all_hard_dir); i > 0; i--)
			if ((uae4all_hard_dir[i] == '/')||(uae4all_hard_dir[i] == '\\'))
				break;
		if (i > 0) {
			strncpy(buffer, &uae4all_hard_dir[i+1], 256);
			strcat(buffer, ":");
			strncat(buffer, uae4all_hard_dir, 256 - strlen(buffer));
			strcpy(dst, buffer); 
		} else
			return NULL;
	}
	
	return dst;
}

char * make_hard_file_cfg_line (char *dst) {
	char buffer[256];
	
	if (uae4all_hard_file[0] != 0) {
		strcpy(buffer, "32:1:2:512:");
		strncat(buffer, uae4all_hard_file, 256 - strlen(buffer));
		strcpy(dst, buffer);
	}
	
	return dst;
}

/*static*/ void parse_filesys_spec (int readonly, char *spec)
{
	/* spec example (<UAE name>:<dir>):
	 * rw,AmigaHD:AmigaHD
	 */
    char buf[256];
    char *s2;

    strncpy (buf, spec, 255); buf[255] = 0;
    s2 = strchr (buf, ':');
    if (s2) {
	*s2++ = '\0';
#ifdef __DOS__
	{
	    char *tmp;
 
	    while ((tmp = strchr (s2, '\\')))
		*tmp = '/';
	}
#endif
	s2 = add_filesys_unit (currprefs.mountinfo, buf, s2, readonly, 0, 0, 0, 0);
	if (s2)
	    fprintf (stderr, "%s\n", s2);
    } else {
	fprintf (stderr, "Usage: [-m | -M] VOLNAME:mount_point\n");
    }
}

/*static*/ void parse_hardfile_spec (char *spec)
{
	/* spec example:
	 * rw,32:1:2:512:hdd/AmigaHD.hdf
	 */
    char *x0 = my_strdup (spec);
    char *x1, *x2, *x3, *x4;

    x1 = strchr (x0, ':');
    if (x1 == NULL)
	goto argh;
    *x1++ = '\0';
    x2 = strchr (x1 + 1, ':');
    if (x2 == NULL)
	goto argh;
    *x2++ = '\0';
    x3 = strchr (x2 + 1, ':');
    if (x3 == NULL)
	goto argh;
    *x3++ = '\0';
    x4 = strchr (x3 + 1, ':');
    if (x4 == NULL)
	goto argh;
    *x4++ = '\0';
    x4 = add_filesys_unit (currprefs.mountinfo, 0, x4, 0, atoi (x0), atoi (x1), atoi (x2), atoi (x3));
    if (x4)
	fprintf (stderr, "%s\n", x4);

    free (x0);
    return;

 argh:
    free (x0);
    fprintf (stderr, "Bad hardfile parameter specified - type \"uae -h\" for help.\n");
    return;
}
