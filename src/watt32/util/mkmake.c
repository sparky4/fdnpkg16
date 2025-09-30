/*
 * This program originally by John E. Davis for the SLang library
 *
 * Modified for Waterloo tcp/ip by G.Vanem 1998
 * Requires SLang and djgpp 2.01+
 */

#include <stdio.h>
#include <slang.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>
#include <dir.h>
#include <dos.h>
#include <sys/stat.h>

int   recursive    = 0;
int   quiet        = 0;
int   time_depend  = 0;
char *in_makefile  = NULL;
char *out_makefile = NULL;
char  line_cont_ch = '\\'; /* if this is not '\\' and a line ends in '\\' */
                           /* then change it to this. (Watcom needs '&') */
#if (SLANG_VERSION < 20000)
  SLPreprocess_Type _pt, *pt;
#else
  SLprep_Type *pt;
#endif

void Usage (void)
{
  if (!quiet)
     fprintf (stderr,
       "Usage: mkmake [-rqt] [-ofile] [-ddir] makefile.all [DEF1 [DEF2 ...]]\n"
       "options:\n"
       "     -r:  recurse all subdirectories (needs `-o' option)\n"
       "     -q:  quiet, don't print any messages\n"
       "     -t:  don't generate file/stdout if `makefile.all' is older\n"
       "     -o:  write parsed `makefile.all' to file (default is stdout)\n"
       "     -d:  creates subdirectory (or subdirectories)\n"
       "     DEF1 DEF2 ... are preprocessor defines in `makefile.all'\n");
  exit (1);
}

void process_makefile (const char *infname, const char *outfname)
{
  char  buf[1024];
  FILE *out = stdout;
  FILE *in;

#if 0
  fprintf (stderr, "in = `%s', out = `%s'\n", infname, outfname);
  return;
#endif

  in = fopen (infname, "rt");
  if (!in)
  {
    if (!quiet)
       fprintf (stderr, "Cannot open `%s'\n", infname);
    Usage();
  }
  if (outfname)
  {
    out = fopen (outfname, "wt");
    if (!out)
    {
      if (!quiet)
         fprintf (stderr, "Cannot open `%s'\n", outfname);
      Usage();
    }
  }

  while (fgets(buf,sizeof(buf)-1,in))
  {
    if (SLprep_line_ok(buf,pt))
    {
      if (line_cont_ch != '\\')
      {  
        unsigned int len = strlen (buf);

        while (len > 0 && isspace(buf[len-1]))
           len--;
        if (len > 0 && buf[len-1] == '\\')
           buf[len-1] = line_cont_ch;
      }
      fputs (buf, out);
    }
  }
  if (out != stdout)
     fclose (out);
}

time_t file_time (const char *file)
{
  struct ffblk ff;
  struct tm    time;
  unsigned     attr = _A_NORMAL|_A_ARCH|_A_RDONLY|_A_HIDDEN;

  if (findfirst(file,&ff,attr))
     return (0);

  time.tm_hour  = (ff.ff_ftime >> 11) & 31;
  time.tm_min   = (ff.ff_ftime >> 5) & 63;
  time.tm_sec   = (ff.ff_ftime & 31) << 1;

  time.tm_year  = (ff.ff_fdate >> 9) + 1980 - 1900;
  time.tm_mday  = (ff.ff_fdate & 31);
  time.tm_mon   = (ff.ff_fdate >> 5) & 15;
  time.tm_mon--;
  time.tm_wday  = 0;
  time.tm_yday  = 0;
  time.tm_isdst = 0;

  if (time.tm_year < 80)
     time.tm_year += 2000;
  return mktime (&time);
}

int check_filestamp (const char *ref_file, const char *new_file)
{
  time_t ref_time, new_time;

  if (!new_file || !time_depend)
     return (0);

  if ((ref_time = file_time(ref_file)) == 0)
     return (0);

  if ((new_time = file_time(new_file)) == 0)
     return (0);

  return (ref_time <= new_time);
}

int dir_tree_walk (const char *path, const struct ffblk *ff)
{
  if ((ff->ff_attrib & _A_SYSTEM) ||
      (ff->ff_attrib & _A_SUBDIR) ||
      (ff->ff_attrib & _A_VOLID))
     return (0);

  if (!stricmp(ff->ff_name,in_makefile))
  {
    char outpath[PATH_MAX];
    char fdrv   [MAXDRIVE];
    char fdir   [MAXDIR];

    fnsplit (path, fdrv, fdir, NULL, NULL);
    sprintf (outpath, "%s%s%s", fdrv, fdir, out_makefile);
  
    if (!check_filestamp(path,outpath))
         process_makefile (path, outpath);
    else if (!quiet)
         fprintf (stderr, "%s is up to date\n", outpath);
  }
  return (0);
}

void check_outfile (void)
{
  if (!recursive && !time_depend)
     return;

  if (!out_makefile)
  {
    fprintf (stderr, "`-r' or `-t' option requires `-o' option\n");
    Usage();
  }

  if (strcspn(out_makefile,"?*[]/\\"))
  {
    fprintf (stderr, "outfile cannot contain path-chars or wildcards\n");
    Usage();
  }
}

void make_dirs (char *dir)
{
  char *slash = strchr (dir, '\\');

  if (slash && slash < dir+strlen(dir))
  {
    *slash = '\0';
    mkdir (dir, 666);
    *slash = '\\';
  }
  mkdir (dir, 666);
}

int main (int argc, char **argv)
{
  int i, ch;

  while ((ch = getopt(argc,argv,"?rqto:d:")) != EOF)
     switch (ch)
     {
       case 'r': recursive = 1;
                 break;
       case 'q': quiet = 1;
                 break;
       case 't': time_depend = 1;
                 break;
       case 'o': out_makefile = optarg;
                 break;
       case 'd': make_dirs (optarg);
                 break;
       case '?':
       default:  Usage();
     }

  argc -= optind;
  argv += optind;
  if (argc <= 1)
     Usage();

  in_makefile = *argv;
  argv++;
  argc--;

  check_outfile();

#if (SLANG_VERSION < 20000)
  SLprep_open_prep (&_pt);
  pt = &_pt;
  pt->preprocess_char = '@';
  pt->comment_char    = '#';
  pt->flags           = SLPREP_BLANK_LINES_OK | SLPREP_COMMENT_LINES_OK;
#else
  pt = SLprep_new();
  assert (pt != NULL);
  SLprep_set_prefix (pt, "@");
  SLprep_set_comment (pt, "#", "#");
  SLprep_set_flags (pt, SLPREP_BLANK_LINES_OK | SLPREP_COMMENT_LINES_OK);
#endif

  for (i = 0; i < argc; i++)
  {
    char *arg = strupr (argv[i]);

    if (!strcmp(arg,"WATCOM"))
       line_cont_ch = '&';
    SLdefine_for_ifdef (arg);
  }

  if (recursive)
  {
    char base [PATH_MAX];
    __file_tree_walk (getcwd(base,sizeof(base)), dir_tree_walk);
  }
  else
  {
    if (!check_filestamp(in_makefile,out_makefile))
         process_makefile (in_makefile, out_makefile);
    else if (!quiet)
         fprintf (stderr, "%s is up to date\n", out_makefile);
  }
  return (0);
}
