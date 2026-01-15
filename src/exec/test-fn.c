/*
 * test app for the FDNPKG16 local package parsing.
 * Made By Google Gemini... an A.I.
 * Strangely this works... let me test it on a real system
 * edited by sparky4 2026
 * works fine
 */
/******************************************************************************
 * Function: get_filename
 * Purpose:  Extracts the filename and extension from a full DOS path.
 *
 * Acknowledgment: This implementation was generated with assistance from
 * Google Gemini (January 2026) based on C89 string manipulation standards.
 *****************************************************************************/

#include <dos.h>      // For INT 21h functions
#include <strings.h> // For strlen, strrchr
#include <string.h> // For strlen, strrchr
#include <stdio.h> // For strlen, strrchr

char *get_filename(char *path) {
    char *filename;
    // Find the last path separator ('\' or '/')
    filename = strrchr(path, '\\');
    if (filename == NULL) {
        filename = strrchr(path, '/');
    }
    if (filename != NULL) {
        return filename + 1; // Return pointer to char after separator
    } else {
        return path; // No separator, whole path is filename
    }
}

void main(int argc, char **argv) {
    char full_path[512];
    char *fname;

    strcpy(full_path, argv[1]);
    fname = get_filename(full_path);

    // Print the extracted filename (example: COMMAND.COM)
    printf("Full Path: %s\n", full_path);
    printf("Filename: %s\n", fname);
    printf("%d\n", argc);

    // For just the name without extension (more complex, often manual parsing)
    // Find the last '.' and handle it...
    // This is basic; older DOS had 8.3 limits, so parsing extensions was simpler.
}
