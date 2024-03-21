#ifndef UNISTD_H_
#define UNISTD_H_

#include <io.h>
#include <process.h>

// Symbolic constants for file access modes
#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1

// Symbolic constants for file modes
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001

// File descriptor values
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

// Standard functions
#define access _access
#define close _close
#define dup _dup
#define dup2 _dup2
#define execv _execv
#define execve _execve
#define execvp _execvp
#define ftruncate _chsize
#define isatty _isatty
#define lseek _lseek
#define read _read
#define unlink _unlink
#define write _write

#endif /* UNISTD_H_ */
