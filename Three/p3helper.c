/* p3helper.c
 * Program 3 assignment
 * Daniel Hernandez
 * CS570
 * Prof. Carroll
 * SDSU
 * Spring 2018
 *
 * This is the ONLY file you are allowed to change. (In fact, the other
 * files should be symbolic links to
 *   ~cs570/Three/p3main.c
 *   ~cs570/Three/p3robot.c
 *   ~cs570/Three/p3.h
 *   ~cs570/Three/makefile
 *   ~cs570/Three/CHK.h    )
 * The file ~cs570/Three/createlinks is a script that will create these for you.
 */
#include "p3.h"

/* You may put declarations/definitions here.
 * In particular, you will probably want access to information
 * about the job (for details see the assignment and the documentation
 * in p3robot.c):
 */
extern int nrRobots;
extern int seed;
extern int width;
extern int quota;

int fd;					//File descriptor for "countfile"
int count;				//Number of robots printing
sem_t *pmutx;				//Semaphore guarding against file
char semaphoreMutx[SEMNAMESIZE];	//Semaphore name


/* General documentation for the following functions is in p3.h
 * Here you supply the code, and internal documentation:
 */
void initStudentStuff(void) {
	/* Unique semaphore name */
	sprintf(semaphoreMutx,"/%s%ldmutx",COURSEID,(long)getuid());
	
	/* Initialize semaphore for read/write. Is created if doesn't exists. Fails if it does exist. */
	if((pmutx = sem_open(semaphoreMutx, O_RDWR | O_CREAT | O_EXCL, 
		S_IRUSR | S_IWUSR, 0)) != SEM_FAILED) {
		/* Critical region. Initialization of file */
		CHK(fd = open("countfile", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR));
		count = 0;
		CHK(lseek(fd, 0, SEEK_SET));
		assert(sizeof(count) == write(fd, &count, sizeof(count)));
		CHK(sem_post(pmutx));
		/* End of critical region */
	}
	
	/* Initialize non-starting semaphores for read/write only */
	else {
		CHK(SEM_FAILED != (pmutx = sem_open(semaphoreMutx, O_RDWR)));
	}
}

/* In this braindamaged version of placeWidget, the widget builders don't
 * coordinate at all, and merely print a random pattern. You should replace
 * this code with something that fully follows the p3 specification.
 */
void placeWidget(int n) {
	/* Enter critical region --- open file, read, write */
	CHK(sem_wait(pmutx));
	CHK(fd = open("countfile", O_RDWR));
	CHK(lseek(fd, 0, SEEK_SET));
	assert(sizeof(count) == read(fd, &count, sizeof(count)));
	count++;
	
	/* Checks if robot is last one to be printed */
	if((nrRobots * quota) == count) {
		printeger(n);
		printf("F\n");
		fflush(stdout);
		/* Close file descriptor, delete file, close/unlink semaphore */
		CHK(close(fd));
		CHK(unlink("countfile"));
		CHK(sem_close(pmutx));
		CHK(sem_unlink(semaphoreMutx));
	}
	
	/* Checks if robot is last in row */
	else {
		if((count % width) == 0) {
			printeger(n);
			printf("N\n");
  			fflush(stdout);
		}
		
		/* Prints other robots that aren't the last or end of row */
		else {
			printeger(n);
			fflush(stdout);
		}
		CHK(lseek(fd, 0, SEEK_SET));
		assert(sizeof(count) == write(fd, &count, sizeof(count)));
		CHK(sem_post(pmutx));
		/* End of critical region */
	}
}

/* If you feel the need to create any additional functions, please
 * write them below here, with appropriate documentation:
 */
