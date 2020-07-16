#ifndef _ERRNO_H
#define _ERRNO_H

#ifndef _ERRNO_H_JUST_CODES // Prevent linking errors when using error codes in kernel code

extern int errno; // Error number (gets assigned by some libc methods if an error occures)

// Hidden array of error strings for use in perror and strerror
// Only visible inside libc. Not when a program uses libc.
__attribute__((visibility ("hidden"))) extern const char *__err_str[];

#endif

//------------------------------------------------------------------------------------------
//				Error numbers
//------------------------------------------------------------------------------------------

// NOTES: - ISO Standard defines error numbers to be strictly positive integer macro constants.
//        - ISO Standard only requires EDOM EILSEQ and ERANGE but those are just used in math functions
//          which OwOS doesn't implement. POSIX extends this list by many more useful error codes.
//        - This implementation only declares a small selection as OwOS is definitely not POSIX compliant.

// Errors referring to files (use stdio and dirent headers)
#define EBADF        1 // Bad file descriptor
#define EEXIST       2 // Existing file
#define ENOSPC       3 // No space left on device
#define EISDIR       4 // Is a directory
#define ENAMETOOLONG 5 // Filename too long
#define ENOENT       6 // No such file or directory
#define ENOTDIR      7 // Not a directory or a symbolic link to a directory
#define ENOTEMPTY    8 // Directory not empty

// General errors
#define EOVERFLOW 9  // Value too large to be stored in data type.
#define ENOMEM    10 // Not enough space
#define EINVAL    11 // Invalid argument

#endif //_ERRNO_H
