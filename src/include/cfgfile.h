#ifndef CFGFILE_H_
#define CFGFILE_H_

extern char * make_hard_dir_cfg_line (char *dst);
extern char * make_hard_file_cfg_line (char *dst);
extern void parse_filesys_spec (int readonly, char *spec);
extern void parse_hardfile_spec (char *spec);

#endif
