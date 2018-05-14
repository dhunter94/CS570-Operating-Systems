/*
 * Daniel Hernandez
 * Prof. Carroll
 * CS570
 * Due: 1/25/18
*/

/*
 * This program is a lexical analyzer that gets a word from the input stream (stdin)
*/

#include "getword.h"

int getword(char *w) {
	int ch;
	int size = 0;
	
	/* 
	 * Initial check of newline, EOF, or # (special characters)
	*/
	
	ch = getchar();
	
	/* Skip leading white-space to get to a valid character*/
	while(ch == ' ')
		ch = getchar();
	
	/* Put null character in array to be able to print empty string. Return -10 */	
	if(ch == '\n') {
		*w = '\0';
		return -10;
	}
	
	/* Put null character in array to be able to print empty string. Return 0 */	
	if(ch == EOF) {
		*w = '\0';
		return 0;
	}
	
	/* Insert hashtag and null to array and return -1 */
	if(ch == '#') {
		*w = '#';
		w++;
		*w = '\0';
		return -1;
	}
	
	/*
	 * Process any other characters to be stored in array
	*/	
	
	while(1) {
		
		/* Check for EOF and space to return word size before it */
		if(ch == EOF || ch == ' ') {	
			*w = '\0';
			return size;
		}
		
		if(ch == '\n') {
			/* Go one character back in stdin in order to process on next call */
			ch = ungetc(ch, stdin);
			*w = '\0';
			return size;
		}
		
		*w = ch;
		w++;
		size++;
		ch = getchar();
	}
}
