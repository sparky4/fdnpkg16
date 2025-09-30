/*!\file pcrecv.h
 */            
#ifndef _w32_PCRECV_H
#define _w32_PCRECV_H

/*!\struct recv_data
 */
typedef struct recv_data {
        DWORD  recv_sig;
        BYTE  *recv_bufs;
        WORD   recv_bufnum;
      } recv_data;

/*!\struct recv_buf
 */
typedef struct recv_buf {
        DWORD       buf_sig;
        DWORD       buf_hisip;
        long        buf_seqnum;
#if defined(USE_IPV6)
        ip6_address buf_hisip6;
#endif
        WORD        buf_hisport;
        short       buf_len;
        BYTE        buf_data [ETH_MAX]; /* sock_packet_peek() needs 1514 */
      } recv_buf;

extern int sock_recv_init (sock_type *s, void *space, unsigned len);
extern int sock_recv_used (const sock_type *s);
extern int sock_recv      (sock_type *s, void *buffer, unsigned len);
extern int sock_recv_from (sock_type *s, void *hisip, WORD *hisport,
                           void *buffer, unsigned len, int peek);
#endif
