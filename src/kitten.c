/* Functions that emulate UNIX catgets */

/* Kitten version 2021 by Tom Ehlert, */

/*
  This software is free software; free to use,
  modify, pass to others, whatever

  use it at your own risk
*/

#ifndef NO_KITTEN

#include <stdio.h>		/* sprintf */
#include <stdlib.h>		/* getenv */
#include <io.h>		/* sprintf */
#include <fcntl.h>		/* sprintf */
#include <string.h>		/* sprintf */

#include "kitten.h"


/* Globals */


/* Functions */
/**
 * On success, catgets() returns a pointer to an internal
 * buffer area containing the null-terminated message string.
 * On failure, catgets() returns the value 'message'.
 */

char*global_nls_buffer;

char *
_kittengets (int msgID, char *message)
{
	struct message_header *message_header; // if this is != 0 then init was done


	if (global_nls_buffer == NULL)
		return message;

	message_header = (struct message_header *)global_nls_buffer;

	for ( ; message_header->len != 0; message_header = (struct message_header *)((char*)message_header + message_header->len))
		{
		if (message_header->id== msgID)
			{
			return (char*)message_header + sizeof(struct message_header);
			}

		}

	return message;
}


/**
 * Initialize kitten for program (name).
 */

#define _toupper(c) (c & ~0x20)

nl_catd
kittenopen (char *name, char *nls_buffer, int buffersize)
{
	int fd;
	struct message_end message_end;
	struct content     content[100];
	char *language;
	int i;
	long len;
	char exename[_MAX_PATH];
	long fileendnow, seekoffset;

	printf("kittenopen %s\n", name);

	if ((fd = open(name, _O_RDONLY | _O_BINARY)) < 0)
		{
		sprintf(exename, "%s.exe", name);
		if ((fd = open(exename, _O_RDONLY | _O_BINARY)) < 0)
			{
			printf("can't open %s\n", name);		// should never happen
			return 0;
			}
		}

 	fileendnow = lseek(fd, 0, SEEK_END);

 	lseek(fd, - (int)sizeof(struct message_end), SEEK_END);

	read(fd, &message_end, sizeof(struct message_end));

	if (memcmp(message_end.ID, "KITTENC", 8) != 0)
		{
		printf("no KITTENC record found\n");
		return 0;
		}

	if (message_end.resource_count > 100)
		{
		printf("resource > 100\n");
		return 0;
		}

	seekoffset = fileendnow - message_end.fileend_orig;

	if (lseek(fd, message_end.filepos+seekoffset, SEEK_SET) != message_end.filepos+seekoffset)
		{
		printf("can't seek 1\n");
		return 0;
		}


	read(fd, &content, sizeof(content[0]) * message_end.resource_count);

	language = getenv("LANG");

	// printf("using language %s\n", language);

	if (message_end.resource_count == 1)
		{
		if (language &&
			_toupper(language[0]) == 'E' &&
			_toupper(language[0]) == 'N')
			{
									// set LANG=EN switches to internal strings
			close(fd);
			return 0;
			}
									// otherwise use the only compiled language
		i = 0;

		goto found_my_language;
		}

	if (language == NULL)			// language not found
		{							// use internal strings
		close(fd);
		return 0;
		}

									// see if we find our language in the existing resources
	for (i = 0; i < message_end.resource_count; i++)
		{
		if(_toupper(content[i].language[0]) == _toupper(language[0]) &&
		   _toupper(content[i].language[1]) == _toupper(language[1]))
			{
			goto found_my_language;
			}
		}

	// language not found
	close(fd);
	return 0;




found_my_language:
	// found our language
	// printf(" language %s found between %lx and %lx\n", language, content[i].filepos_start, content[i].filepos_end);

	// printf("seek %lu --> %lu\n", content[i].filepos_start, content[i].filepos_start+seekoffset);

	lseek(fd, content[i].filepos_start+seekoffset, SEEK_SET);
	len = content[i].filepos_end - content[i].filepos_start;

	if (len > buffersize)
		{
		printf("nls_buffer too small(%d). we need at least %l\n",buffersize, len);
		close(fd);
		return 1;
		}


	read(fd, nls_buffer, (int)len);
	global_nls_buffer = nls_buffer;				// export the messagebuffer globally

	close(fd);
	return 0;
}



#endif /*NO_KITTEN */
