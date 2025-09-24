/*
 * This file is part of the FDNPKG16 project.
 * Copyright (C) 2025 sparky4
 */

#include <string.h>
#include <ctype.h>  // For tolower()
#include <stddef.h> // For size_t

/*#ifdef _STRINGS_H_INCLUDED
int strcasecmp(const char *s1, const char *s2) {
  while (*s1 && (tolower((unsigned char)*s1) == tolower((unsigned char)*s2))) {
    s1++;
    s2++;
  }
  return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
  if (n == 0) {
    return 0;
  }

  // Process strings up to n characters or until a null terminator is reached
  while (n > 0 && *s1 != '\0' && *s2 != '\0') {
    int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
    if (diff != 0) {
      return diff;
    }
    s1++;
    s2++;
    n--;
  }

  // Check if one string is a prefix of the other, comparing the final characters
  if (n > 0) {
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
  }

  // If n=0, the strings are equal over the compared length
  return 0;
}
*/
