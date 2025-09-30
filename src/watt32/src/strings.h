/*!\file strings.h
 */
#ifndef _w32_STRINGS_H
#define _w32_STRINGS_H

#define _printf      NAMESPACE (_printf)
#define _outch       NAMESPACE (_outch)
#define outs         NAMESPACE (outs)
#define outsnl       NAMESPACE (outsnl)
#define outsn        NAMESPACE (outsn)
#define outhexes     NAMESPACE (outhexes)
#define outhex       NAMESPACE (outhex)
#define rip          NAMESPACE (rip)
#define StrLcpy      NAMESPACE (StrLcpy)
#define strntrimcpy  NAMESPACE (strntrimcpy)
#define strrtrim     NAMESPACE (strrtrim)
#define strltrim     NAMESPACE (strltrim)
#define strreplace   NAMESPACE (strreplace)
#define atox         NAMESPACE (atox)

/* Points to current var-arg print routine.
 * Need gcc 3.x for printf-check on func-pointers.
 */
W32_DATA int (MS_CDECL *_printf) (const char *fmt, ...) ATTR_PRINTF(1,2);

/* Points to current character output handler.
 */
W32_DATA void (*_outch) (char c);

W32_FUNC void  outs    (const char *s);         /* print a ASCIIZ string to stdio  */
W32_FUNC void  outsnl  (const char *s);         /* print a ASCIIZ string w/newline */
W32_FUNC void  outsn   (const char *s, int n);  /* print a string with len max n   */
W32_FUNC void  outhexes(const char *s, int n);  /* print a hex-string with len n   */
W32_FUNC void  outhex  (char ch);               /* print a hex-char to stdio       */
W32_FUNC char *rip     (char *s);               /* strip off '\r' and '\n' from s  */

extern  char  *strreplace  (int ch1, int ch2, char *str);
extern  char  *StrLcpy     (char *dst, const char *src, size_t len);
extern  size_t strntrimcpy (char *dst, const char *src, size_t len);
extern  char  *strrtrim    (char *src);
extern  char  *strltrim    (const char *src);
extern  BYTE   atox        (const char *src);

#endif

