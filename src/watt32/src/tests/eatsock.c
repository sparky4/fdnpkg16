#include <assert.h>

#include "socket.h"
#include "pcdbug.h"

WORD  test_port = 1234;
int   max_socks = 0;
int  *socks = NULL;

void setup (void)
{
  dbug_init();
  sock_init();

#if defined(USE_PROFILER)
  if (!profile_enable)
  {
    profile_enable = 1;
    if (!profile_init())
       exit (-1);
  }
#endif
}

/*--------------------------------------------------------------------------*/

int main (int argc, char **argv)
{
  struct sockaddr_in sa;
  int    rc, i;

  if (argc != 2)
  {
    printf ("Usage: %s num-sockets", argv[0]);
    return (0);
  }
  max_socks = atoi (argv[1]);
  if (max_socks == 0)
  {
    printf ("Usage: %s num-sockets", argv[0]);
    return (0);
  }

  setup();
  socks = calloc (max_socks, sizeof(int));
  assert (socks != NULL);

  for (i = 0; i < max_socks; i++)
  {
    PROFILE_START ("socket");
    socks[i] = socket (AF_INET, SOCK_STREAM, 0);
    PROFILE_STOP();
    if (socks[i] < 0)
    {
      perror ("socket");
      max_socks = i;
      break;
    }
  }

  for (i = 0; i < max_socks; i++)
  {
    sa.sin_family      = AF_INET;
    sa.sin_port        = htons (test_port);
    sa.sin_addr.s_addr = INADDR_ANY;
    PROFILE_START ("bind");
    rc = bind (socks[i], (struct sockaddr*)&sa, sizeof(sa));
    PROFILE_STOP();
    if (rc < 0)
    {
      perror ("bind");
      break;
    }
  }

  for (i = 0; i < max_socks; i++)
  {
    PROFILE_START ("close_s");
    close_s (socks[i]);
    PROFILE_STOP();
  }
  free (socks);
  return (0);
}

