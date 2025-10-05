/* Functions that emulate UNIX catgets, some small DOS file functions */

/* Copyright (C) 1999,2000 Jim Hall <jhall@freedos.org> */
/* Kitten version by Tom Ehlert, heavily modified by Eric Auer 2003 */

/*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef _CATGETS_H
#define _CATGETS_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef NO_KITTEN

#define kittengets(x,y,z) (z)
#define kittenclose()
#define kittenopen(a)

#else

/* Data types */

#define nl_catd int

/* Functions */

#define catgets(catalog, set,message_number,message) _kittengets((catalog << 8) + set,default_message)
#define catopen(name,flag) kittenopen(argv[0], nls_buffer, sizeof(nls_buffer))
#define catclose(catalog) 

#define kittengets(catalog, messageid,message) _kittengets((catalog << 8) + messageid,message)


  char *_kittengets (int messageid, char *message);
  nl_catd kittenopen (char *exename, char *buffer, int buffersize);


//internal stuff between kitten.c and kittenc.c
	struct message_header
		{
		short len;
		short id;
		// char message[];
		};

	struct content
		{
		char language[4];
		long filepos_start;	// for this language
		long filepos_end;
		long reserved;		// align on 16 byte to look better in hex editor
		};

	struct message_end
		{
		long filepos;		// points to content
		long resource_count;// number of entries
		long fileend_orig;  // file end NOW because UPX
		char ID[8];			// "KITTENC"
		};



#endif				/*NO_KITTEN */
#ifdef __cplusplus
}
#endif

#endif				/* _CATGETS_H */
