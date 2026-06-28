#include <stdio.h>

void main() {
#if defined(__WATCOMC__)
#if (__WATCOMC__ >= 1200)
  printf ("FDNPKG%s is Compiled with OpenWatcom %d.%d\n", "16",
                  (__WATCOMC__/100) - 11, (__WATCOMC__ % 100) / 10);
  printf ("%d\n", __WATCOMC__);
  #endif
  #endif
}
