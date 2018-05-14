#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include "CHK.h"
#include "getword.h"


#define MAXITEM 100 		/* max number of words per line */
#define MAXSTORAGE (STORAGE*MAXITEM)	/* max amount of characters per command (STORAGE * MAXITEM) */
#define MAXPIPES 10		/* Max amount of pipes one can use 10 */
#define MAXPIPEARGS 11		/* Max amount of arguments with use of 10 pipes */

int parse();
void pipecmd();
void sighandler();
