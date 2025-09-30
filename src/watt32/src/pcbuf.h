/*!\file pcbuf.h
 */
#ifndef _w32_PCBUF_H
#define _w32_PCBUF_H

#define VALID_UDP  1
#define VALID_TCP  2
#define VALID_IP4  3
#define VALID_IP6  4

/* public API
 */
W32_FUNC size_t sock_rbsize  (const sock_type *s);
W32_FUNC size_t sock_rbused  (const sock_type *s);
W32_FUNC size_t sock_rbleft  (const sock_type *s);
W32_FUNC size_t sock_tbsize  (const sock_type *s);
W32_FUNC size_t sock_tbused  (const sock_type *s);
W32_FUNC size_t sock_tbleft  (const sock_type *s);

W32_FUNC size_t sock_setbuf  (sock_type *s, BYTE *buf, size_t len);
W32_FUNC size_t sock_preread (const sock_type *s, BYTE *buf, size_t len);

W32_FUNC const char *sockerr      (const sock_type *s);
W32_FUNC void        sockerr_clear(sock_type *s);        /* GvB */

W32_FUNC const char *sockstate     (const sock_type *s);
W32_FUNC int        _chk_socket    (const sock_type *s);
W32_FUNC void       sock_debugdump (const sock_type *s);


/* Reduce internal states to "user-easy" states, GvB 2002-09
 */
enum TCP_SIMPLE_STATE {
     TCP_CLOSED,
     TCP_LISTENING,
     TCP_OPENING,
     TCP_OPEN,
     TCP_CLOSING
   };

W32_FUNC enum TCP_SIMPLE_STATE tcp_simple_state (const _tcp_Socket *s);

/* privates
 */
extern void _sock_check_tcp_buffers (const _tcp_Socket *tcp);
extern void _sock_check_udp_buffers (const _udp_Socket *udp);

const _raw_Socket  *find_oldest_raw  (const _raw_Socket *raw);
const _raw6_Socket *find_oldest_raw6 (const _raw6_Socket *raw);

#endif

