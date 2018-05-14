/*
 * Daniel Hernandez
 * Prof. Carroll
 * CS570
 * Due: 4/25/18
*/

/*
 * This program is a simple linux shell that processes commands from stdin. Implemented a better MV command, 
 * allow for reading in script files, and vertical piping up to 10 pipes. 
*/

#include "p2.h"

int char_count;				// Number of characters returned by getword()
int pipe_pos[MAXPIPEARGS];		// Index of the address of the word after a pipe (|)
int pipe_err[MAXPIPEARGS];		// Index of the left of (|&)
char char_arr[MAXSTORAGE];		// Array of the characters entered (to stdin) by user
char *words[MAXITEM];			// Addresses of the start of each word from char_arr
int input_fd, output_fd, devnull_fd, argv_fd;	// I/O file descriptors
char *IN_FILE, *OUT_FILE;			// I/O file names
int BG_FLAG, IN_FLAG, OUT_FLAG, P_FLAG, PA_FLAG, H_FLAG, S_FLAG = 0 ;		// Flags for metacharacters (&,<,>,|, |&,#)
										// S_FLAG indicates a script file
int main(int argc, char *argv[]) {
	int word_count = 0;			// Number of words entered by user
	pid_t pid, kidpid;		
	
	
	/* Set group PID for p2.c. Catch SIGTERM and send it to hhandler function */
	setpgid(0,0);
	(void)signal(SIGTERM, sighandler);
	
	for(;;) { 
		/* Check for script */
		if(argc > 2) {
			fprintf(stderr, "Too many command line arguments\n");
			exit(EXIT_FAILURE);
		}
		
		else if (argv[2] != NULL && (strcmp(argv[2], "LANG=C") != 0)) {
			fprintf(stderr, "Invalid command\n");
			exit(EXIT_FAILURE);
		}
		
		/* Redirect STDIN to the script file to be read */
		else if(argc == 2 && S_FLAG == 0) {
			if((argv_fd = open(argv[1], O_RDONLY)) == -1) {
				perror(argv[1]);
				exit(EXIT_FAILURE);
			}
			if((dup2(argv_fd, STDIN_FILENO)) == -1) {
				perror("Bad file descriptor");
				exit(EXIT_FAILURE);
			}
			S_FLAG++;
		}
		
		/* Don't output prompt if script is found */
		if(S_FLAG == 0)
			printf("p2: ");
			
		word_count = parse();
		/* Check for intial EOF. If so terminate program */
		if(char_count == 0 && word_count == 0) {
			break;
		}
			
		/* Check for no words on the line. Reprompt */
		if(word_count == 0) {
			continue;
		}
		
		/* Check for cd command */
		if(strcmp(words[0],"cd") == 0) {
			char *home;		// Home environment variable
			
			if(word_count > 2) {
				fprintf(stderr, "Too many arugments \n");
			}
			
			/* If input was only cd with no args. Get home evironment variable. */
			else if (word_count == 1) {
				if((home = getenv("HOME")) == NULL) 
					fprintf(stderr, "\'HOME\' environment variable doesn't exist\n");
				else
					chdir(home);
			}
			
			else {
				if((chdir(words[1])) == -1)
					perror(words[1]);
			}
			continue;
		}
		
		/* Check for MV command */
		if(strcmp(words[0], "MV") == 0) {
			int i;			// Index of user words
			int args = 0;		// Number of args after MV
			int error = 0;		// Checks if first arg isn't a file
			int errorCheck = 0;	// Checks if second arg is a path with a new file name
			int argIndex[2];	// Place within words array were arguments are located
			int fileType[2];	// File type: file = 0, dir = 1	
			char *flag =  "empty";	// If flags are seen         
			struct stat sb;		// Structure to search for file types
		
			/* MV with no arguments */
			if(word_count == 1) {
				fprintf(stderr, "MV: missing file operand\n");
				continue;
			}
			
			/* Iterate through command */
			for(i = 1; i < word_count; i++) {
				/* Check for flags */
				if(words[i][0] == '-') {
					if((strcmp(words[i], "-n") == 0) || (strcmp(words[i], "-f") == 0)) {
						flag = words[i];
					}
					else {
						flag = words[i];
						break;
					}
				}
				else {
					/* If stat fails check to see if it's a path with a new file name */
					if(stat(words[i], &sb) == -1) {
						errorCheck = i;
						args++;
						break;
					}
					
					/* File types */
					switch(sb.st_mode & S_IFMT) {
						/* Directory file type */
						case S_IFDIR: 
							/* Exit if directory is first MV arg */
							if(args == 0) {
								argIndex[args] = i;
								args++;
								error = 1;
								break;
							}
							fileType[args] = 1;
							argIndex[args] = i;
							args++;
							break;
						/* Regular file type */
						case S_IFREG:
							fileType[args] = 0;
							argIndex[args] = i;
							args++;
							break;
					}
				}
			}
			
			/* Error checking */
			if(error == 1) {
				fprintf(stderr, "MV: invalid file type\n");
			}
			
			if((strcmp(flag, "-n") != 0) && (strcmp(flag, "-f") != 0) && (strcmp(flag, "empty") != 0)) {
				fprintf(stderr, "MV: invalid option -- \'%s\'\n", flag);
			}
			
			if(args == 0) {
				fprintf(stderr, "MV: missing file operand\n");
			}
			
			if(args == 1) {
				if(errorCheck != 0) {
					fprintf(stderr, "MV: missing destination file operand after \'%s\'\n", words[errorCheck]);
				}
				else {
					fprintf(stderr, "MV: missing destination file operand after \'%s\'\n", words[argIndex[0]]);
				}
				continue;
			}
			
			if(args > 2) {
				fprintf(stderr, "MV: too many agruments\n");
				continue;
			}
			
			/* errorCheck indicates either location with a new file name or invalid file type */
			if(errorCheck != 0) {
				if(args == 2)  {
					/* Non-clobber link if -n or no flag found */
					if((strcmp(flag, "-n") == 0) || (strcmp(flag, "empty") == 0)) {
						if((link(words[argIndex[0]], words[errorCheck])) == -1) {
							perror(words[errorCheck]);
							continue;
						}
						if((unlink(words[argIndex[0]])) == -1) {
							perror("Could not unlink");
							continue;
						}
					}
					
					/* Force MV indicated by -f */
					else {
						unlink(words[argIndex[errorCheck]]);
						if((link(words[argIndex[0]], words[errorCheck])) == -1) {
							perror("Invalid MV");
							continue;
						}
						if((unlink(words[argIndex[0]])) == -1) {
							perror("Could not unlink2");
							continue;
						}
					}
				}
				
				/* Invalid file type indicated by errorCheck */
				else {
					fprintf(stderr, "Invalid file or directory: \'%s\'\n", words[errorCheck]);
					continue;
				}
			}
			
			/* Move file to new path */
			else {
				char *filePath;
				/* File to path name arguments */
				if(fileType[1] == 1) {
					/* If file is at the end of a path name */
					if(strstr(words[argIndex[0]], "/") != NULL) {
							filePath = strrchr(words[argIndex[0]], '/');
							/* Concatenate to end of file path with same name */
							strcat(words[argIndex[1]], filePath);
					}
					
					/* Concatenate file to end of path if just file name */
					else {
							strcat(words[argIndex[1]], "/");
							strcat(words[argIndex[1]], words[argIndex[0]]); 
					}
				}
					
				/* -n or no flag */
				if((strcmp(flag, "-n") == 0) || (strcmp(flag, "empty") == 0)) {
					if((link(words[argIndex[0]], words[argIndex[1]])) == -1) {
						perror(words[argIndex[1]]);
						continue;
					}
				
					if((unlink(words[argIndex[0]])) == -1) {
						perror("Could not unlink");
						continue;
					}
				}
					
				/* -f flag */
				else {
					unlink(words[argIndex[1]]);
					if((link(words[argIndex[0]], words[argIndex[1]])) == -1) {
						perror("Invalid MV");
						continue;
					}
					if((unlink(words[argIndex[0]])) == -1) {
						perror("Could not unlink");
						continue;
					}
				}					
			}
			continue;
		}
		
		/* '|' pipe command seen */
		if(P_FLAG != 0) {
			int i;
			int error = 0;
			if(P_FLAG > MAXPIPES) {
				fprintf(stderr, "Too many pipe characters. Max 10\n");
				continue;
			}
			for(i = 0; i < P_FLAG; i++) {
				if(words[pipe_pos[i]] == NULL) {
					fprintf(stderr, "Invalid null command\n");
					error = 1;
				}
			}
			if(error == 1)
				continue;
			pipecmd();
			continue;
		}
		
		/* '<' input redirection command found */
		if(IN_FLAG != 0) {
			if(IN_FILE == NULL) {
				fprintf(stderr, "Missing name for redirect\n");
				continue;
			}
			
			if(IN_FLAG > 2) {
				fprintf(stderr, "Ambiguous input redirect\n");
				continue;
			}
			
			/* Open file outside of fork if fails. Read-only */
			if((input_fd = open(IN_FILE, O_RDONLY)) == -1) {
				perror(IN_FILE);
				continue;
			}
		}
		
		/* '>' output redirection command found */
		if(OUT_FLAG != 0) {
			/* O_RDWR: Request opening the file read/write
			   O_APPEND: Write to end of the file
			   O_CREAT: If file doesn't exist create new file
			   O_EXCL: Ensures file is created, if file exitst error is sent */
			int flags = O_RDWR | O_APPEND | O_CREAT | O_EXCL;
			
			if(OUT_FILE == NULL) {
				fprintf(stderr, "Missing name for redirect\n");
				continue;
			}
			
			if(OUT_FLAG > 2) {
				fprintf(stderr, "Ambiguous output redirect\n");
				continue;
			}
			
			/* Open file outside of fork if fails. Allow for read/write permission */
			if((output_fd = open(OUT_FILE, flags, S_IRUSR | S_IWUSR)) == -1) {
				perror(OUT_FILE);
				continue;
			}
		}
				/* '|' Pipe command found */	
		
		/* '&' background character command found */
		if(BG_FLAG != 0 && IN_FLAG == 0) {
			/* Open /dev/null so background job doesn't read from stdin */
			if((devnull_fd = open("/dev/null", O_RDONLY)) == -1) {
				perror("/dev/null");
				continue;
			}
		}
		
		/* Clear buffer from left-over input */
		fflush(stdout);
		fflush(stderr);
		
		/* Forking process to create a child */
		/* Fork failed */
		if((kidpid = fork()) == -1) {
			perror("Fork failed");
			exit(1);
		}
		
		/* Child process */
		else if(kidpid == 0) {
		
			/* Redirect input file to STDIN. Close input file descriptor. */
			if(IN_FLAG != 0) {
				if((dup2(input_fd,STDIN_FILENO)) == -1) {
					perror("Failed to redirect file descriptor to STDIN");
					exit(1);
				}
				if((close(input_fd)) == -1) {
					perror("Unable to close file descriptor\n");
					exit(1);
				}
			}

			/* Redirect output file to STDOUT. Close output file descriptor. */
			if(OUT_FLAG != 0) {
				if((dup2(output_fd,STDOUT_FILENO)) == -1) {
					perror("Failed to redirect file descriptor to STDOUT");
					exit(1);
				}
				if((close(output_fd)) == -1) {
					perror("Unable to close file descriptor\n");
					exit(1);
				}
			}
			
			/* Redirect /dev/null to STDIN */
			if(BG_FLAG != 0 && IN_FLAG == 0) {
				if((dup2(devnull_fd,STDIN_FILENO)) == -1) {
					perror("Failed to redirect file descriptor to STDIN");
					exit(1);
				}
				if((close(devnull_fd)) == -1) {
					perror("Unable to close file descriptor\n");
					exit(1);
				}
			}
			
			/* Execute command */
			if((execvp(words[0],words)) == -1) {
				perror("Command not found");
				exit(9);
			}
		}
		
		/* Parent process */
		else {
			/* No background process found */
			if(BG_FLAG == 0) {
				/* Waits for child process to finish */
				for(;;) {
					pid = wait(NULL);
					if(pid == kidpid)
						break;
				}
			}
			
			/* Print out background process */
			else {
				printf("%s [%d]\n", words[0], kidpid);
				continue;
			}
			
		}
	}
	killpg(getpgrp(), SIGTERM);
	printf("p2 terminated.\n");
	exit(0);
}

int parse() {
	int i = 0;		// Number of words
	int loc_count = 0;	// Location of start of words in char_arr
	IN_FILE = NULL, OUT_FILE = NULL;					// Setting I/O files
	BG_FLAG = 0, IN_FLAG = 0, OUT_FLAG = 0, P_FLAG = 0, PA_FLAG = 0, H_FLAG = 0;	// Setting flags
	
	for(;;) {
		
		char_count = getword(char_arr + loc_count);
		
		/* Setting metacharacter flags */
		if(char_count == 0) {
			break;
		}
		
		else if(char_count == -10) {
			break;
		}
		
		else if(H_FLAG != 0) {
			loc_count += abs(char_count) + 1;
		}
		
		else if(*(char_arr + loc_count) == '&' && char_count == -1) {
			BG_FLAG++;
			break;
		}
		
		else if((*(char_arr + loc_count) == '<' && char_count == -1) || IN_FLAG == 1) {
			if(IN_FLAG == 1)
				IN_FILE = char_arr + loc_count;		// Save file after <
			IN_FLAG++;
		}
		
		else if((*(char_arr + loc_count) == '>' && char_count == -1) || OUT_FLAG == 1) {
			if(OUT_FLAG == 1)
				OUT_FILE = char_arr + loc_count;	// Save file after >
			OUT_FLAG++;
		}
		
		else if(*(char_arr + loc_count) == '|' && (char_count == -1 || char_count == -2)) {
			if(char_count == -2) {		// Keep track of location of |&
				pipe_err[P_FLAG] = 1;
			}
				
			P_FLAG++;
			words[i] = NULL;	// Set null when pipe is seen to use execvp
			i++;
			pipe_pos[P_FLAG-1] = i;		// Save position of word after pipe
		}

		else if((*(char_arr + loc_count) == '#' && char_count == -1)) {
			if(S_FLAG != 0) {
				H_FLAG++;
			}
			else {
				words[i] = char_arr + loc_count;
				i++;
			}
		}
		
		/* Process regular words on the command line */
		else {
			words[i] = char_arr + loc_count;
			i++;
		}
		loc_count += abs(char_count) + 1;
	}
	words[i] = NULL;
	return i;
}

void pipecmd() {
	int fildes[P_FLAG*2];		// To hold file descriptors stdin and stdout of pipe
	pid_t middle, last;		// PID for child and middle children
	
	/* '<' input redirection command found */
	if(IN_FLAG != 0) {
		if(IN_FILE == NULL) {
			fprintf(stderr, "Missing name for redirect\n");
			return;
		}
		
		if(IN_FLAG > 2) {
			fprintf(stderr, "Ambiguous input redirect\n");
			return;
		}
			
		/* Open file outside of fork if fails. Read-only */
		if((input_fd = open(IN_FILE, O_RDONLY)) == -1) {
			perror(IN_FILE);
			return;
		}
	}
	
	/* '>' output redirection command found */
	if(OUT_FLAG != 0) {
		/* O_RDWR: Request opening the file read/write
		   O_APPEND: Write to end of the file
		   O_CREAT: If file doesn't exist create new file
		   O_EXCL: Ensures file is created, if file exitst error is sent */
		int flags = O_RDWR | O_APPEND | O_CREAT | O_EXCL;
		
		if(OUT_FILE == NULL) {
			fprintf(stderr, "Missing name for redirect\n");
			return;
		}
		
		if(OUT_FLAG > 2) {
			fprintf(stderr, "Ambiguous output redirect\n");
			return;
		}
		
		/* Open file outside of fork if fails. Allow for read/write permission */
		if((output_fd = open(OUT_FILE, flags, S_IRUSR | S_IWUSR)) == -1) {
			perror(OUT_FILE);
			return;
		}
	}		
	
	fflush(stdout);
	fflush(stderr);
	
	/* Fork to create child */
	CHK(last = fork());
	if(last == 0) {
		/* Variables for 'for' loops to close fildes */
		int i;
		int p;
		int n;
		
		/* pipe within child so only child and grandchildren know about the file des */
		for(p = 0; p < P_FLAG; p++) {
			CHK(pipe(&fildes[2*p]));
		}
		
		/* Redirect input file to STDIN */
		if(IN_FLAG != 0) {
			CHK(dup2(input_fd, STDIN_FILENO));
			CHK(close(input_fd));
		}
		
		/* Redirect output file to STDOUT */
		if(OUT_FLAG != 0) {
			CHK(dup2(output_fd, STDOUT_FILENO));
			CHK(close(output_fd));
		}
		
		/* Fork middle children */
		for(i = 0; i < P_FLAG; i++) {
			int j;			// For 'for' loop to close fildes
			fflush(stdout);
			fflush(stderr);
			CHK(middle = fork());
			if(middle == 0) {
				/* If only 1 pipe */
				//printf("child %d\n", i);
				if(P_FLAG == 1) {
					if(pipe_err[(P_FLAG-1)-i] == 1)
						CHK(dup2(fildes[(i*2)+1], STDERR_FILENO));
					CHK(dup2(fildes[1], STDOUT_FILENO));
					CHK(close(fildes[0]));
					CHK(close(fildes[1]));
					CHK(execvp(words[0], words));
				}
			}
			else {
				//printf("parent %d\n", i);
				if(P_FLAG != 1) {
					/* Redirect to pipe read end to STDIN of process if not greatest grandchild*/
					if(i != P_FLAG-1) {
						CHK(dup2(fildes[(i*2)+2], STDIN_FILENO));
					}
				
					/* Check for |& metacharacter and redirect to STDERR */
					if(pipe_err[(P_FLAG-1)-i] == 1) {
						CHK(dup2(fildes[(i*2)+1], STDERR_FILENO));
					}
				
					/* Redirect write end to STDOUT of process */
					CHK(dup2(fildes[(i*2)+1], STDOUT_FILENO));
					/* Close all file descriptors */
					for(j = 0; j < P_FLAG * 2; j++) {
						CHK(close(fildes[j]));
					}
				
					/* If greatest grandchild execute first command */
					if(i == P_FLAG-1) {
						CHK(execvp(words[0], words));
					}
					else {
						CHK(execvp(words[pipe_pos[(P_FLAG-2)-i]], words + pipe_pos[(P_FLAG-2)-i]));
					}
				}
			}
		}
		
		/* child */		
		/* Redirect STDIN to read end of pipe. Last command of pipes */ 
		CHK(dup2(fildes[0], STDIN_FILENO));
		for(n = 0; n < P_FLAG * 2; n++) {
			CHK(close(fildes[n]));
		}
		CHK(execvp(words[pipe_pos[P_FLAG-1]], words + (pipe_pos[P_FLAG-1])));
	}
	
	/* Wait for child process to finish */
	for(;;) {
		pid_t pid;
		CHK(pid = wait(NULL));
		if(pid == last)
			break;
	}
}

void sighandler() {

}
