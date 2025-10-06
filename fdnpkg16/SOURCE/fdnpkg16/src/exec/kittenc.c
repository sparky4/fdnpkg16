/*
  KITTENC - compile kitten files and attach this to 
  executables in a way that the kitten library retrieves
  the language resources, in the same way that 
  catgets/kittengets ever did

*/
/*
  This software is free software; free to use,
  modify, pass to others, whatever

  use it at your own risk
*/



#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <io.h>

#include "kitten.h"

void usage()
	{
	printf(kittengets(1,0,"KITTENC - KITTEN compiler\n"));
	printf(kittengets(1,1,"usage\n"));
	printf(kittengets(1,2,"KITTENC program.exe ATTACH NLS\\program.??\n"));
	printf(kittengets(1,3,"KITTENC program.exe ATTACH NLS\\program.DE\n"));
	printf(kittengets(1,4,"KITTENC program.exe ATTACH NLS\\program.DE NLS\\program.fr\n"));
	printf(kittengets(1,5,"KITTENC program.exe INFO     : show info about language resources\n"));
	printf(kittengets(1,6,"KITTENC program.exe DUMP     : recreate language resources\n"));
	printf(kittengets(1,7,"KITTENC program.exe TRUNCATE : delete attached resources\n"));
	}



/**
 * Parse a string that represents an unsigned integer.
 * Returns -1 if an error is found. The first size
 * chars of the string are parsed.
 */

int
mystrtoul (char *src, int base, int size)
{
  int ret = 0;

  for (; size > 0; size--)
    {
      int digit;
      int ch = *src;

      src++;

      if (ch >= '0' && ch <= '9')
		digit = ch - '0';
      else if (ch >= 'A' && ch <= 'Z')
		digit = ch - 'A' + 10;
      else if (ch >= 'a' && ch <= 'z')
		digit = ch - 'a' + 10;
      else
		return -1;

      if (digit >= base)
		return -1;

      ret = ret * base + digit;
    }				/* for */

  return ret;
}




/**
 * Process strings, converting \n, \t, \v, \b, \r, \f, \\,
 * \ddd, \xdd and \x0dd to actual chars.
 * (Note: \x is an extension to support hexadecimal)
 * This is used to allow the messages to use C escape sequences.
 * Modifies the line in-place (always same size or shorter).
 * Returns a pointer to input string.
 */

char special[]   = "\\nrtvbafx";
char translated[]= "\\\n\r\t\v\b\a\fx";


char *
processEscChars (char *line)
{
  /* used when converting \xdd and \ddd (hex or octal) characters */
  char ch;
  char *src = line;
  char *dst = line;		/* possible as dst is shorter than src */
  char *s;
  int chx;


  if (line == NULL)
    return line;

  /* cycle through copying characters, except when a \ is encountered. */
	while (*src != '\0')
		{
		ch = *src;
		src++;

		if (ch != '\\')
			{
			*dst++ = ch;
			continue;
			}

		ch = *src;		/* what follows slash? */
	    src++;

		s = strchr(special, ch);		/* is it a special character?? */

		if (s == NULL)					/* no. store the character as is */
			{
			*dst++ = ch;
			continue;
			}
		ch = translated[s - special];	

		if (ch != 'x')					/* newline, formfeed etc. */
			{
			*dst++ = ch;
			continue;
			}

		chx = mystrtoul (src, 16, 2);	/* get value */
		if (chx >= 0)
			{		/* store character */
			*dst = chx;
			dst++;
			src += 2;
			}
		else		/* error so just store x (loose slash) */
		  {
		    *dst = *src;
		    dst++;
		  }
		}				/* while */

  /* ensure '\0' terminated */
  *dst = '\0';

  return line;
}


//
// process a single FIND.DE file
//

int process_language_file(FILE *fd, char *filename)
	{
	FILE *fdres;
	char linebuff[512];
	char msg[512];
	int id1, id2;
	struct message_header message_header;
	int len;
	int linecount = 0;
	
	if ((fdres = fopen(filename, "r")) == 0)
		{
		printf("can't open %s\n", filename);
		return 0;
		}

	while (fgets(linebuff, sizeof(linebuff), fdres))
		{
		char *s = linebuff;

		linecount++;

		for (len = strlen(s); len > 0; )			// delete trailing white space
			{										// from line
			len--;					  
			if (strchr(" \t\r\n", s[len]) == NULL)
				break;
			s[len] = '\0';
			}
		if (len == 0)
			continue;


		if (sscanf(linebuff, " %u , %u :%s", &id1, &id2, msg) != 3)
			{
			if (sscanf(linebuff, " %c", &id1) == 0 ||		// empty line 
					id1 == '#')								// # comment line
				continue;

			printf("%3u:%10.10s: missing 'num, num :' \n", linecount, linebuff);
			continue;
			}
		s = strchr(linebuff, ':');
		if (s == NULL) continue;

		s++;

		processEscChars(s);

		if (strlen(s) > 250)
			{
			printf("%10.10s: lines must not be longer then 250 characters, is %u\n", linebuff, strlen(s));
			s[250] = 0;
			}

									/* now here we have destilled id1, id2, message s */

		message_header.len =  strlen(s)		// the message
								+ 1			// the trailing \0
								+ sizeof(message_header);
		message_header.id = (id1 << 8) + id2;

		fwrite(&message_header, 1, sizeof(message_header), fd);
		fwrite(s, strlen(s)+1,1,fd);

		}


	message_header.len =  0;		// END message
	message_header.id  =  0;
	
	fwrite(&message_header, 1, sizeof(message_header), fd);

	return linecount;
	}


// this is the 'compiler' part of KITTENC
//
// attach("FIND.EXE", "NLS\\FIND.??", 1)  
// attach("FIND.EXE", "NLS\\FIND.DE", NLS\\FIND.ES", 2)  
//

int attach(char *progname, char *args[], int argc)	   
	{
	int i;
	FILE *fd;
	struct _finddata_t fileinfo;
	long search_handle;
	char *s;
	char *filename;
	char path_buffer[_MAX_PATH];
	struct content content[100];
	struct message_end message_end;
	int linecount;

	int resourcecount = 0;

	if ((fd = fopen(progname, "r+b")) == NULL)
		{
		printf(kittengets(2,1, "executable file <%s> can't be opened. because <%s>\n"), progname, strerror( errno ));
		return 1;
		}
	fseek(fd, 0, SEEK_END);

	for (i = 0; i < argc; i++)
		{
		filename = args[i];

		printf("%s\n", filename);

		if ((search_handle = _findfirst(args[i], &fileinfo)) == 0)
			{
			printf(kittengets(2,2,"%s: no such file\n"), args[i]);
			return 1;
			}

		do {									/* the name is name only, without the path */

			if ((s = strrchr(filename, '\\')) != NULL ||
			    (s = strrchr(filename, ':')) != NULL)
				{
				memcpy(path_buffer, filename, s-filename+1);
				strcpy(path_buffer + (s-filename) +1, fileinfo.name);
				}
			else
				strcpy(path_buffer, fileinfo.name);

			printf("%s:", path_buffer);


			if ((s = strrchr(path_buffer, '.')) == NULL ||
				strlen(path_buffer) - (s - path_buffer) != 3)
				{
				printf(kittengets(2,3,"file names MUST end with .EN\n"));
				continue;
				}

			if (resourcecount > 100)
				{
				printf(kittengets(2,4,"too many resourcefiles (max 100)\n"));
				continue;
				}

			content[resourcecount].filepos_start = ftell(fd);
			strcpy(content[resourcecount].language,s+1);


			linecount = process_language_file(fd, path_buffer);

			content[resourcecount].filepos_end = ftell(fd);
			content[resourcecount].reserved    = 0;

			printf("%u messages, %lu byte\n", linecount, content[resourcecount].filepos_end - content[resourcecount].filepos_start);

			resourcecount++;

			} while (_findnext(search_handle, &fileinfo) == 0);
		}

	message_end.filepos = ftell(fd);

				/* make all absolute filepositions relative to file end 
					to make UPX support easier */


	fwrite(&content, sizeof(content[0]),resourcecount,fd);

	message_end.resource_count = resourcecount;
	memcpy(message_end.ID, "KITTENC",8);

	message_end.fileend_orig = ftell(fd) + sizeof(message_end);

	fwrite(&message_end, sizeof(message_end),1,fd);
	fclose(fd);

	return 0;
	}

enum { KITTEN_TRUNCATE, KITTEN_INFO, KITTEN_DUMP };

do_kitten_stuff(char *progname, int mode)
	{
	int fd;
	struct message_end message_end;
	struct content     content[100];
	int i;
	long fileendnow, seekoffset;


	
	if ((fd = open(progname, _O_RDWR | _O_BINARY)) < 0)
		{
		printf("can't open %s because<%s>\n", progname, strerror(errno));
		return 0;
		}
 	fileendnow = lseek(fd, 0, SEEK_END);

 	lseek(fd, - (int)sizeof(struct message_end), SEEK_END);

	read(fd, &message_end, sizeof(struct message_end));

	if (memcmp(message_end.ID, "KITTENC", 8) != 0)
		{
		printf("no KITTENC record found\n");
		goto RETURN;
		}

	if (message_end.resource_count > 100)	
		{
		printf("resource > 100\n");
		goto RETURN;
		}

	seekoffset = fileendnow - message_end.fileend_orig;

	if (lseek(fd, message_end.filepos+seekoffset, SEEK_SET) != message_end.filepos+seekoffset)
		{
		printf("can't seek 1\n");
		goto RETURN;
		}

	read(fd, &content, sizeof(content[0]) * message_end.resource_count);
	

	if (mode == KITTEN_TRUNCATE)
		{
		_chsize(fd, content[0].filepos_start);		// truncate file
		goto RETURN;
		}
	if (mode == KITTEN_INFO)
		{
		printf("%s: %u languages supported\n", progname, message_end.resource_count);

		for (i = 0; i < message_end.resource_count; i++)
			{
			printf("%2.2s %lu byte\n", content[i].language, content[i].filepos_end - content[i].filepos_start);
			}

		goto RETURN;
		}

	if (mode == KITTEN_DUMP)
		{
		char resname[_MAX_PATH],*s;
		char outname[_MAX_PATH];
		FILE *fdout;
		int i;
		int linecount;

		for (i = 0; i < message_end.resource_count; i++)		// for each language
			{
			_splitpath(progname, NULL,NULL, resname, NULL);		// reconstruct KITTENC.DE

			sprintf(outname, "%s.%c%c", resname, content[i].language[0], content[i].language[1]);;

			if ((fdout = fopen(outname, "w")) == NULL)			// create KITTENC.DE
				{
				printf("can't write resourcefile %s\n", outname);
				goto RETURN;
				}
			lseek(fd, content[i].filepos_start+seekoffset, SEEK_SET);

			printf("%s : ", outname);

			linecount= 0;

			while (1)
				{
				short len, id; char c;

				read(fd, &len, sizeof(short));

				if (len == 0)
					break;

				read(fd,&id, sizeof(short));

				fprintf(fdout, "%u,%u:", id >> 8, id & 0xff);

				while (read(fd, &c, 1) == 1 &&
					c != 0)
					{
					if (c >= 0x20)
						fputc(c, fdout);
					else
						{
						if ((s = strchr(translated, c)) != NULL)		/* \t \n \f */
							{
							fprintf(fdout, "\\%c", special[s - translated]);	
							}
						else
							fprintf(fdout, "\\x%02x", c);	
						}
					} // next character

				fprintf(fdout, "\n");
				linecount++;
				}	// next line

			printf("%u messages\n", linecount);
			fclose(fdout);
			} // next language


		goto RETURN;
		}



RETURN:
	close(fd);
	return 0;

	}


dump(char*progname)
	{
	printf("not yet done\n");
	return 1;
	}
info(char*progname)
	{
	printf("not yet done\n");
	return 1;
	}



main(int argc, char *argv[])
{
	char *progname;
	char buffer[_MAX_PATH];

	{
	static char nls_buffer[4000];

	kittenopen(argv[0], nls_buffer, sizeof(nls_buffer));
	}

	printf("KITTENC 1.0\n");

	printf(kittengets(0,0,"hallo world\n"));

	if (argc < 3)
		{
		usage();
		exit(1);
		}

	 progname = argv[1];
	 strupr(progname);

	 if (strstr(progname, ".EXE") == NULL &&
		 strstr(progname, ".COM") == NULL)
		 {
		 sprintf(buffer,"%s.EXE", progname);
		 progname = buffer;
		 }

	 if (stricmp(argv[2], "ATTACH") == 0)
		{
		do_kitten_stuff(progname, KITTEN_TRUNCATE);
		return attach(progname, argv+3, argc-3);

		}
	 else if (stricmp(argv[2], "TRUNCATE") == 0)
		{
		return do_kitten_stuff(progname, KITTEN_TRUNCATE);


		}
	 else if (stricmp(argv[2], "INFO") == 0)
		{
		return do_kitten_stuff(progname, KITTEN_INFO);
		}
	 else if (stricmp(argv[2], "DUMP") == 0)
		{
		return do_kitten_stuff(progname, KITTEN_DUMP);
		}
	 else {
		 printf("'%s' is wrong %s %s %s\n", argv[2], argv[0], argv[1], argv[2]);
		 usage();
		 exit(1);						 
		 }
 
}


