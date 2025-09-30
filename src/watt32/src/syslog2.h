/*!\file syslog2.h
 */
#ifndef _w32_SYSLOG2_H
#define _w32_SYSLOG2_H

extern char syslog_fileName [MAX_NAMELEN];
extern char syslog_hostName [MAX_HOSTLEN];
extern WORD syslog_port;
extern int  syslog_mask;

extern void syslog_init (void);

#endif
