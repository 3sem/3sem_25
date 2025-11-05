/*
 Write a 'shell-wrapper' program which allow users to:

    - run programs via command line cyclically getting commands from STDIN and running 	    it somewhere, e.g. in child process.
    - get exit codes of terminated programs
    - use pipelining of program sequences, for ex.: «env | grep HOSTNAME | wc»
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE_LENGTH 1048576
#define MAX_TOKENS      1048576
#define MAX_COMMANDS    1048576


int main() {

	return 0;

}
