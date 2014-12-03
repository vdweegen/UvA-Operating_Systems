/*
	names: 			Cas van der Weegen & Johan Josua Storm
	Studentnumber: 	6055338 & 6283934
	Assignment 1, april 15 2013
*/
#define MAX_ARGS	(1024)
#define MAX_LINE	(2 * MAX_ARGS)

int INTERRUPT;

char current_PATH[MAX_LINE];

extern int scanLine(FILE *, int *);

void parseCommand(char *);

void executeCommand(char *);

void handleInterrupt(int);
