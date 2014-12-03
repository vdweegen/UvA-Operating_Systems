/*
	names: 			Cas van der Weegen & Johan Josua Storm
	Studentnumber: 	6055338 & 6283934
	Assignment 1, april 15 2013
*/
#define MAX_ARGS	(1024)
#define MAX_LINE	(2 * MAX_ARGS)

int INTERRUPT;

char current_PATH[MAX_LINE];

int scanLine(FILE *, int *);

int do_exit(char *command);

int do_cd(char *command);

int do_source(char *command);

typedef int builtinFun(char *command);

typedef struct builtin{
        builtinFun *fun;
        char name[32];
}builtin;

