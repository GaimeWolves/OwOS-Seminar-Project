#include "include/errno.h"

// Implementation of the errno constant initialized with zero
int errno = 0;

const char *__err_str[12] =
{
	/* NOERROR      */ "No error",
	
	// File errors (stdio and dirent)
	/* EBADF        */ "Bad file descriptor",
	/* EEXISTS      */ "File already exists",
	/* ENOSPC       */ "No space left on device",
	/* EISDIR       */ "File is a directory",
	/* ENAMETOOLONG */ "Filename too long",
	/* ENOENT       */ "No such file or directory",
	/* ENOTDIR      */ "Not a directory or a symbolic link to a directory",
	/* ENOTEMPTY    */ "Directory is not empty",

	// General errors
	/* EOVERFLOW */ "Value too large",
	/* ENOMEM    */ "Out of memory",
	/* EINVAL    */ "Invalid argument"
};
