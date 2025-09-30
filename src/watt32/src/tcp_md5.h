/*!\file tcp_md5.h
 */
#ifndef _w32_TCP_MD5_H
#define _w32_TCP_MD5_H

#define make_md5_signature   NAMESPACE (make_md5_signature)
#define check_md5_signature  NAMESPACE (check_md5_signature)

/* At the momement, this is the only public interface.
 */
extern const char *tcp_md5_secret (void *s, const char *secret);

extern void make_md5_signature (const in_Header  *ip,
                                const tcp_Header *tcp,
                                WORD datalen, const char *secret,
                                BYTE *buf);

extern BOOL check_md5_signature (const _tcp_Socket *s,
                                 const in_Header *ip);

#endif
