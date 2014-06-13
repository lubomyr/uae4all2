 /*
  * UAE - The Un*x Amiga Emulator
  *
  * routines to handle compressed file automatically
  *
  * (c) 1996 Samuel Devulder, Tim Gunn
  */

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"

#include "uae.h"
#include "options.h"
#include "memory-uae.h"
#include "zfile.h"

#include "savedisk.h"
#include "menu_config.h"

#include <zlib.h>

#ifdef ANDROIDSDL
#include <android/log.h>
#endif

#ifdef WIN32
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <windows.h>
#include <io.h>

int mkstemp(char *templ)
{
    char *temp;

    temp = _mktemp(templ);
    if (!temp)
        return -1;

    return _open(temp, _O_CREAT | _O_TEMPORARY | _O_EXCL | _O_RDWR, _S_IREAD | _S_IWRITE);
}
#endif


#define MAX_COMP_SIZE (1024*128)

extern char launchDir[300];

#define VMUFILE_PAD 0
#define prepare_save()
#define rebuild_paquete(A,B,C,D)
#define set_vmu_pad(A)
#define maple_first_vmu() 1


struct zfile
{
    struct zfile *next;
    struct zfile **pprev;
    FILE *f;
    char name[L_tmpnam];
};

static struct zfile *zlist = 0;




static unsigned getFreeBlocks(void)
{
	return 0x10000;
}

static void eliminate_file(char *filename)
{
	FILE *f=fopen(filename,"r");
	if (f)
	{
		fclose(f);
		unlink(filename);
	}
}

#define VRAM_MAX_LEN (384*1024)
#define MAX_DISK_LEN 1024*(1024-128)
#define MAX_ROM_LEN  11 + (MAX_DISK_LEN-VRAM_MAX_LEN)
static void *uae4all_rom_memory=NULL;
static unsigned uae4all_rom_len=0;
static unsigned uae4all_rom_pos=0;

static void *uae4all_disk_memory[4]={ NULL ,NULL ,NULL ,NULL };
static void *uae4all_extra_buffer=NULL;
static unsigned uae4all_disk_len[4]={ 0 ,0 ,0 ,0 };
static unsigned uae4all_disk_pos[4]={ 0 ,0 ,0 ,0 };
static unsigned char uae4all_disk_used[4]= { 0 ,0 ,0 ,0 };
static int uae4all_disk_writed[4]= { 0, 0, 0, 0 };
static int uae4all_disk_writed_now[4]= { 0, 0, 0, 0 };
static void *uae4all_disk_orig[4]={ NULL, NULL, NULL, NULL };
static unsigned uae4all_disk_crc[4]={ 0, 0, 0, 0 };
static unsigned uae4all_disk_actual_crc[4]={ 0, 0, 0, 0};

void zfile_exit (void)
{
    struct zfile *l;

    while ((l = zlist)) {
	zlist = l->next;
	fclose (l->f);
	unlink (l->name); /* sam: in case unlink () after fopen () fails */
	free (l);
    }
}

/*
 * fclose () but for a compressed file
 */
int zfile_fclose (struct zfile *f)
{
    int ret;

    if (f->next)
	f->next->pprev = f->pprev;
    (*f->pprev) = f->next;
    
    ret = fclose (f->f);
    unlink (f->name);

    free (f);

    return ret;
}

int zfile_fseek (struct zfile *z, long offset, int mode)
{
    return fseek (z->f, offset, mode);
}

long zfile_ftell (struct zfile *z)
{
    return ftell (z->f);
}

size_t zfile_fread (void *b, size_t l1, size_t l2, struct zfile *z)
{
    return fread (b, l1, l2, z->f);
}

size_t zfile_fwrite (void *b, size_t l1, size_t l2, struct zfile *z)
{
    return fwrite (b, l1, l2, z->f);
}


/*
 * gzip decompression
 */
static int gunzip (const char *decompress, const char *src, const char *dst)
{
    char cmd[1024];
    const char *ext = strrchr (src, '.');
    if (!dst)
	return 1;
    sprintf (cmd, "%s -c -d \"%s\" > \"%s\"", decompress, src, dst);
    return !system (cmd);
}


/*
 * bzip/bzip2 decompression
 */
static int bunzip (const char *decompress, const char *src, const char *dst)
{
    char cmd[1024];
    if (!dst)
	return 1;
    sprintf (cmd, "%s -c -d \"%s\" > \"%s\"", decompress, src, dst);
    return !system (cmd);
}

/*
 * lha decompression
 */
static int lha (const char *src, const char *dst)
{
    char cmd[1024];
    if (!dst)
	return 1;
    sprintf (cmd, "lha pq %s >%s", src, dst);
    return !system (cmd);
}

/*
 * (pk)unzip decompression
 */
static int unzip (const char *src, const char *dst)
{
    char cmd[1024];
    if (!dst)
	return 1;
    sprintf (cmd, "unzip -p '%s' *.adf *.ADF *.Adf >%s", src, dst);
    return !system (cmd);
}

/*
 * decompresses the file (or check if dest is null)
 */
static int uncompress (const char *name, char *dest)
{
    const char *ext = strrchr (name, '.');
    char nam[1024];

    if (ext != NULL && access (name, 0) >= 0) {
	ext++;
	if (strcasecmp (ext, "z") == 0
	    || strcasecmp (ext, "gz") == 0
	    || strcasecmp (ext, "adz") == 0
	    || strcasecmp (ext, "roz") == 0)
	    return gunzip ("gzip", name, dest);
	if (strcasecmp (ext, "bz") == 0)
	    return bunzip ("bzip", name, dest);
	if (strcasecmp (ext, "bz2") == 0)
	    return bunzip ("bzip2", name, dest);

	if (strcasecmp (ext, "lha") == 0
	    || strcasecmp (ext, "lzh") == 0)
	    return lha (name, dest);
	if (strcasecmp (ext, "zip") == 0)
	     return unzip (name, dest);
	if (strcasecmp (ext, "rp9") == 0)
	     return unzip (name, dest);
    }

    if (access (strcat (strcpy (nam, name), ".z"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".Z"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".gz"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".GZ"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".adz"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".roz"), 0) >= 0)
	return gunzip ("gzip", nam, dest);

    if (access (strcat (strcpy (nam, name), ".bz"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".BZ"), 0) >= 0)
	return bunzip ("bzip", nam, dest);

    if (access (strcat (strcpy (nam, name), ".bz2"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".BZ2"), 0) >= 0)
	return bunzip ("bzip2", nam, dest);

    if (access (strcat (strcpy (nam, name), ".lha"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".LHA"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".lzh"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".LZH"), 0) >= 0)
	return lha (nam, dest);

    if (access (strcat (strcpy (nam, name),".zip"),0) >= 0
	|| access (strcat (strcpy (nam, name),".ZIP"),0) >= 0
	|| access (strcat (strcpy (nam, name),".Zip"),0) >= 0
	|| access (strcat (strcpy (nam, name),".rp9"),0) >= 0
	|| access (strcat (strcpy (nam, name),".RP9"),0) >= 0)      
       return unzip (nam, dest);

    return 0;
}

/*
 * fopen () for a compressed file
 */
struct zfile *zfile_open (const char *name, const char *mode)
{
    struct zfile *l = (struct zfile *)malloc (sizeof *l);
    int fd = 0;

    if (! l)
	return NULL;

    strcpy (l->name, "");

    if (! uncompress (name, NULL))
	l->f = fopen (name, mode);
    else {
//	tmpnam (l->name);
//	fd = creat (l->name, S_IRUSR | S_IWUSR);
#ifdef ANDROIDSDL
  strncpy(l->name, "./uaetmp-XXXXXX", L_tmpnam);
#else
  strncpy(l->name, "/tmp/uaetmp-XXXXXX", L_tmpnam);
#endif
  fd = mkstemp(l->name);
  
	if (fd < 0)
	    return NULL;

	if (! uncompress (name, l->name)) {
	    close (fd);
	    unlink (l->name);
	    free (l);
	    return NULL;
	}
	l->f = fopen (l->name, mode);
	close (fd);

    }
    if (l->f == NULL) {
	if (strlen (l->name) > 0)
	    unlink (l->name);
	free (l);
	return NULL;
    }

    l->pprev = &zlist;
    l->next = zlist;
    if (l->next)
	l->next->pprev = &l->next;
    zlist = l;

    return l;
}


#ifdef USE_ZFILE
#define mi_z_type gzFile
#define mi_z_open(NAME,P) gzopen(NAME,P)
#define mi_z_seek(F,O,P) gzseek(F,O,P)
#define mi_z_tell(F) gztell(F)
#define mi_z_read(F,M,S) gzread(F,M,S)
#define mi_z_close(F) gzclose(F)
#else
#define mi_z_type FILE *
#define mi_z_open(NAME,P) fopen(NAME,P)
#define mi_z_seek(F,O,P) fseek(F,O,P)
#define mi_z_tell(F) ftell(F)
#define mi_z_read(F,M,S) fread(M,1,S,F)
#define mi_z_close(F) fclose(F)
#endif

static int  try_to_read_disk(int i,const char *name)
{
    mi_z_type f=mi_z_open(name,"rb");
    if (f)
    {
	    int readed=mi_z_read(f,uae4all_disk_memory[i],MAX_DISK_LEN);
	    mi_z_close(f);
	    if (readed>0)
 	    {
	    	uae4all_disk_len[i]=readed;
	    	return readed;
	    }
    }
#ifdef USE_ZFILE
    FILE * f2=fopen(name,"rb");
    if (f2)
    {
	    fseek(f2,0,SEEK_END);
	    uae4all_disk_len[i]=ftell(f2);
	    fseek(f2,0,SEEK_SET);
	    if (uae4all_disk_len[i]>MAX_DISK_LEN)
		uae4all_disk_len[i]=MAX_DISK_LEN;
	    uae4all_disk_len[i]=fread(uae4all_disk_memory[i],1,uae4all_disk_len[i],f2);
	    fclose(f2);
	    return (uae4all_disk_len[i]);
    }
#endif
    return 0;
}

static char __uae4all_write_namefile[256];

static char *get_namefile(unsigned num)
{
	unsigned crc=uae4all_disk_crc[num];

	if (!launchDir)
	{
		getcwd(launchDir, 250);
	}
	snprintf((char *)&__uae4all_write_namefile[0], 256, "%s/saves/%.8X.ads",launchDir, crc);

	return (char *)&__uae4all_write_namefile[0];
}

static void uae4all_disk_real_write(int num)
{
	unsigned new_crc=savedisk_get_checksum(uae4all_disk_memory[num],MAX_DISK_LEN);
	if (new_crc!=uae4all_disk_actual_crc[num])
	{
		void *buff=uae4all_disk_memory[num];
		void *buff_patch=uae4all_extra_buffer;
		memset(buff_patch,0,MAX_DISK_LEN);
		unsigned changed=savedisk_get_changes(buff,MAX_DISK_LEN,buff_patch,uae4all_disk_orig[num]);
		if ((changed)&&(changed<MAX_DISK_LEN))
		{
			char *namefile=get_namefile(num);
			void *bc=calloc(1,MAX_COMP_SIZE);
			unsigned long sizecompressed=MAX_COMP_SIZE;
			//int compress2(Bytef * dest, uLongf * destLen, const Bytef * source, uLong sourceLen, int level);
			int retc=compress2((Bytef *)bc,&sizecompressed,(const Bytef *)uae4all_extra_buffer,changed,Z_BEST_COMPRESSION);
			if (retc>=0)
			{
				unsigned usado=0;
				{
					FILE *f=fopen(namefile,"rb");
					if (f)
					{
						fseek(f,0,SEEK_END);
						usado=ftell(f);
						fclose(f);
						usado/=512;
					}
				}
				if ( ((getFreeBlocks()+usado)*512) >=(sizecompressed+VMUFILE_PAD))
				{
					eliminate_file(namefile);
					FILE *f=fopen(namefile,"wb");
					if (f)
					{
						rebuild_paquete(prefs_df[num], sizecompressed, (unsigned char*) bc, f);
						fwrite((void *)&sizecompressed,1,4,f);
						fwrite(bc,1,sizecompressed,f);
						fclose(f);
					}
				}
			}
			free(bc);
			uae4all_disk_actual_crc[num]=new_crc;
// FIXME - error: 'sync' was not declared in this scope
#ifndef WIN32
			sync();
#endif
		}
	}
}



static void uae4all_initsave(unsigned num)
{
	if (!uae4all_disk_orig[num])
		uae4all_disk_orig[num]=malloc(MAX_DISK_LEN);
	memcpy(uae4all_disk_orig[num],uae4all_disk_memory[num],MAX_DISK_LEN);
	uae4all_disk_crc[num]=savedisk_get_checksum(uae4all_disk_orig[num],MAX_DISK_LEN);
	if ((!mainMenu_autosave)||(!maple_first_vmu()))
		return;
	FILE *f=fopen(get_namefile(num),"rb");
	if (f)
	{
		void *bc=calloc(1,MAX_COMP_SIZE);
		unsigned long n;
		set_vmu_pad(f);
		fread((void *)&n,1,4,f);
		if (fread(bc,1,n,f)>=n)
		{
			unsigned long sizeuncompressed=MAX_DISK_LEN;
			int retc=uncompress((Bytef *)uae4all_extra_buffer,&sizeuncompressed,(const Bytef *)bc,n);
			if (retc>=0)
			{
				savedisk_apply_changes(uae4all_disk_memory[num],uae4all_extra_buffer,sizeuncompressed);
			}
			else
			{
				fclose(f);
				f=NULL;
				eliminate_file(get_namefile(num));
			}
		}
		free(bc);
		if (f)
			fclose(f);
	}
	uae4all_disk_actual_crc[num]=savedisk_get_checksum(uae4all_disk_memory[num],MAX_DISK_LEN);
}



size_t uae4all_fread( void *ptr, size_t tam, size_t nmiemb, FILE *flujo)
{
	int i;
	for(i=0;i<mainMenu_drives;i++)
		if (flujo==uae4all_disk_memory[i])
			break;
	if (i>=mainMenu_drives)
		return 0;
	if (uae4all_disk_pos[i]>=uae4all_disk_len[i])
		return 0;
	memcpy(ptr,(void *)(((unsigned)uae4all_disk_memory[i])+((unsigned)uae4all_disk_pos[i])),tam*nmiemb);
	uae4all_disk_pos[i]+=tam*nmiemb;
	return nmiemb;
}

size_t uae4all_fwrite( void *ptr, size_t tam, size_t nmiemb, FILE *flujo)
{
	int i;
	for(i=0;i<mainMenu_drives;i++)
		if (flujo==uae4all_disk_memory[i])
			break;
	if (i>=mainMenu_drives)
		return 0;
	if (uae4all_disk_pos[i]>=uae4all_disk_len[i])
		return 0;
	memcpy((void *)(((unsigned)uae4all_disk_memory[i])+((unsigned)uae4all_disk_pos[i])),ptr,tam*nmiemb);
	uae4all_disk_pos[i]+=tam*nmiemb;
	uae4all_disk_writed[i]=1;
	return nmiemb;
}

int uae4all_fseek( FILE *flujo, long desplto, int origen)
{
	int i;
	for(i=0;i<mainMenu_drives;i++)
		if (flujo==uae4all_disk_memory[i])
			break;
	if (i>=mainMenu_drives)
		return -1;
	switch(origen)
	{
		case SEEK_SET:
			uae4all_disk_pos[i]=desplto;
			break;
		case SEEK_CUR:
			uae4all_disk_pos[i]+=desplto;
			break;
		default:
			uae4all_disk_pos[i]=uae4all_disk_len[i];
	}
	if (uae4all_disk_pos[i]<=uae4all_disk_len[i])
		return 0;
	uae4all_disk_pos[i]=uae4all_disk_len[i];
	return -1;
}

long uae4all_ftell( FILE *flujo)
{
	int i;
	for(i=0;i<mainMenu_drives;i++)
		if (flujo==uae4all_disk_memory[i])
			break;
	if (i>=mainMenu_drives)
		return 0;
	return uae4all_disk_pos[i];
}

int uae4all_init_rom(const char *name)
{
	prepare_save();
	FILE *f=fopen(name,"rb");
	if (f)
	{
		printf("Open kick %s\n",name);
		if(uae4all_rom_memory == NULL)
		  uae4all_rom_memory=calloc(1,MAX_ROM_LEN);
		fseek(f,0,SEEK_END);
		uae4all_rom_len=ftell(f);
		fseek(f,0,SEEK_SET);
		if (uae4all_rom_len>MAX_ROM_LEN)
		    uae4all_rom_len=MAX_ROM_LEN;
		fread(uae4all_rom_memory,uae4all_rom_len,1,f);
		uae4all_rom_pos=0;
		fclose(f);
		return 0;
	}
	printf("Kick %s not open\n",name);
	return -1;
}

void uae4all_rom_reinit(void)
{
	prepare_save();
	if(uae4all_rom_memory != NULL)
	  free(uae4all_rom_memory);
	uae4all_rom_memory=NULL;
}

FILE *uae4all_rom_fopen(const char *name, const char *mode)
{
	prepare_save();
	if (!uae4all_rom_memory)
		uae4all_init_rom(name);
	return ((FILE *)uae4all_rom_memory);
}

int uae4all_rom_fclose(FILE *flujo)
{
	if(flujo!=((FILE *)uae4all_rom_memory))
	  return 1;
	if(uae4all_rom_memory != NULL)
	  free(uae4all_rom_memory);
	uae4all_rom_memory=NULL;
	return 0;
}

size_t uae4all_rom_fread(void *ptr, size_t tam, size_t nmiemb, FILE *flujo)
{
	if ((!flujo)||(!uae4all_rom_memory))
		return 0;
	if (uae4all_rom_pos>=uae4all_rom_len)
		return 0;
	memcpy(ptr,(void *)(((unsigned)uae4all_rom_memory)+((unsigned)uae4all_rom_pos)),tam*nmiemb);
	uae4all_rom_pos+=tam*nmiemb;
	return (uae4all_rom_len == 262155 || uae4all_rom_len == 524299) /* cloanto */ ? uae4all_rom_len - 11 : uae4all_rom_len;
}


int uae4all_rom_fseek( FILE *flujo, long desplto, int origen)
{
	if ((!flujo)||(!uae4all_rom_memory))
		return 0;
	switch(origen)
	{
		case SEEK_SET:
			uae4all_rom_pos=desplto;
			break;
		case SEEK_CUR:
			uae4all_rom_pos+=desplto;
			break;
		default:
			uae4all_rom_pos=uae4all_rom_len;
	}
	if (uae4all_rom_pos<=uae4all_rom_len)
		return 0;
	uae4all_rom_pos=uae4all_rom_len;
	return -1;
}

void uae4all_flush_disk(int n)
{
	if ((uae4all_disk_writed[n])&&(mainMenu_autosave))
	{
		if (maple_first_vmu())
		{
			if (uae4all_disk_writed_now[n]>6)
			{
				uae4all_disk_real_write(n);
				uae4all_disk_writed[n]=0;
				uae4all_disk_writed_now[n]=0;
			}
			else
				uae4all_disk_writed_now[n]++;
		}
	}
}

