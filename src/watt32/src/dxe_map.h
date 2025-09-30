/*
 * dxe_map.h - a map (associative array) implementation
 * 
 * written by 2002 dbjh
 * 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef DXE_MAP_H
#define DXE_MAP_H

/*
 * map_create()  create a new map (associative array)
 * map_copy()    copy map src to map dest
 *               dest must be a larger map than src
 *               Note that this function also copies the member cmp_key.
 * map_put()     put object in map under key
 *               Callers should always reset the passed map pointer with
 *               the one this function returns. This is necessary in case
 *               the map had to be resized.
 * map_get()     get object from map stored under key
 *               returns NULL if there is no object with key in map
 * map_del()     remove the object stored under key from map
 * map_dump()    display the current contents of map
 * 
 * The value MAP_FREE_KEY is reserved as a special key value. Don't use that
 * value.
 */
#define MAP_FREE_KEY 0

typedef struct st_map_element {
        void  *key;
        void  *object;
      } st_map_element_t;

typedef struct st_map {
        st_map_element_t *data;
        int    size;
        int  (*cmp_key) (const void *key1, const void *key2);
      } st_map_t;

extern st_map_t *map_create (int n_elements);
extern st_map_t *map_put (st_map_t *map, const void *key, const void *object);
extern void      map_copy (st_map_t *dest, const st_map_t *src);
extern void     *map_get (st_map_t *map, const void *key);
extern void      map_del (st_map_t *map, const void *key);
extern void      map_dump (const st_map_t *map);

#endif  /* DXE_MAP_H */

