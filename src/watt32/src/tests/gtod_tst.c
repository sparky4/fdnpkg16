#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <sys/wtime.h>
#include <tcp.h>

#ifdef WIN32
#include <windows.h>
#define usleep(us)  Sleep((us)/1000UL)
#endif

int main (int argc, char **argv)
{
  if (argc > 1 && !strcmp(argv[1],"-t"))
     init_timer_isr();

  while (!kbhit())
  {
    struct timeval tv;
    const char *tstr;

    gettimeofday2 (&tv, NULL);
    tstr = ctime (&tv.tv_sec);
    printf ("%10u.%06lu, %s",
            tv.tv_sec, tv.tv_usec, tstr ? tstr : "illegal\n");
    usleep (500000);
  }
  return (0);
}
