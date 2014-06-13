 /*
  * UAE - The Un*x Amiga Emulator
  *
  * routines to handle compressed file automatically
  *
  * (c) 1996 Samuel Devulder
  */
struct zfile;

extern struct zfile *zfile_open (const char *, const char *);
extern int zfile_fclose (struct zfile *);
extern int zfile_fseek (struct zfile *z, long offset, int mode);
extern long zfile_ftell (struct zfile *z);
extern size_t zfile_fread (void *b, size_t l1, size_t l2, struct zfile *z);
extern size_t zfile_fwrite (void *b, size_t l1, size_t l2, struct zfile *z);
extern void zfile_exit (void);

extern size_t uae4all_fread( void *ptr, size_t tam, size_t nmiemb, FILE *flujo);
extern size_t uae4all_fwrite( void *ptr, size_t tam, size_t nmiemb, FILE *flujo);
extern int uae4all_fseek( FILE *flujo, long desplto, int origen);
extern long uae4all_ftell( FILE *flujo);

extern int uae4all_init_rom(const char *name);
extern size_t uae4all_rom_fread( void *ptr, size_t tam, size_t nmiemb, FILE *flujo);
extern int uae4all_rom_fseek( FILE *flujo, long desplto, int origen);
extern FILE *uae4all_rom_fopen(const char *name, const char *mode);
extern int uae4all_rom_fclose(FILE *flujo);
extern void uae4all_rom_reinit(void);
extern void uae4all_flush_disk(int n);
