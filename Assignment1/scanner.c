/*
	names: 			Cas van der Weegen & Johan Josua Storm
	Studentnumber: 	6055338 & 6283934
	Assignment 1, april 15 2013
*/
/* This skeleton code is provided for the practical work for the "OS"
   course 2011 at the Universiteit van Amsterdam. It is untested code that
   shows how a commandline could possibly be parsed by a very simple
   shell.
   (C) Universiteit van Amsterdam, 1997 - 2011
   Author: G.D. van Albada
	   G.D.vanAlbada@uva.nl
   Date:   October 1, 1997

   At least one problem is known: a command string containg the (illegal)
   combination '||' will lead to severe problems - so add extra tests.
   */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "scanner.h"
#include "func.h"

#define MAX_ARGS	(1024)
#define MAX_LINE	(2 * MAX_ARGS)

static builtin eigen[] =
{
    {do_exit, "exit"},
    {do_cd, "cd"},
    {do_source, "source"},
    {do_source, "."},
    {NULL, ""}
};
	   
void executeCommand(char *commandStr)
{
    char * args[MAX_ARGS] = {NULL};
    int i = 1;
    char funpath[MAX_LINE] = "/bin/";

    args[0] = strtok(commandStr, " \t\n");
    while ((args[i] = strtok(NULL, " \t\n"))) i++;

    if (strchr(args[0], '/'))
    {
	fprintf(stderr, "Attempt to call function '%s' not in /bin\n", args[0]);
	exit(-1);
    }
    strcat(funpath, args[0]);

    if (execv(funpath, args) == -1)
    {
	fprintf(stderr, "Error executing file\n");
	fflush(stderr);
        exit(1);
    }
    exit(-2);
}

void parseCommand(char *commandStr)
{
    char *pipeChar;
    pid_t nPid, nPid2;
    int pipefd[2];
    int len;
    struct sigaction ignore, saveint, savequit;
	
    sigemptyset(&ignore.sa_mask);
    ignore.sa_handler = SIG_IGN;
    ignore.sa_flags = 0;
	
    sigaction(SIGINT, &ignore, &saveint);
    sigaction(SIGQUIT, &ignore, &savequit);	
	
    len = strlen(commandStr);
    if (commandStr[0] == '|' || commandStr[(len)] == '|') {
	
		printf("Geaccepteerde invoer is 'opdracht parameter | opdracht parameter' \n");
        exit(1);
    }
	
    if ((pipeChar = strchr(commandStr, '|')))
    {
	char commandStr1[MAX_LINE];
	char *cptr = commandStr;
	char *cptr1 = commandStr1;

	if (pipe(pipefd) == -1)
	{
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	
	nPid = fork();
	
	if (nPid < 0)
	{
		perror("fork");
		exit(EXIT_FAILURE);
	}
	else if (nPid == 0)
	{ 
	    sigaction(SIGINT, &saveint, (struct sigaction *) 0);
	    sigaction(SIGQUIT, &savequit, (struct sigaction *) 0);
	    dup2(pipefd[1], 1);
	    close(pipefd[0]);

	    while (cptr != pipeChar)
	    {
	    
		    *(cptr1++) = *(cptr++);
	    }
	    *cptr1 = 0;
	    executeCommand(commandStr1);    
	}
	else if (nPid > 0)
	{
		dup2(pipefd[0], 0);
		close(pipefd[1]);				
		parseCommand(pipeChar + 1);
	}     
    }
    else
    {
	nPid2 = fork();    
	if (nPid2 == 0)
	{
	    sigaction(SIGINT, &saveint, (struct sigaction *) 0);
	    sigaction(SIGQUIT, &savequit, (struct sigaction *) 0);
	    executeCommand(commandStr);
	}
	else
	{
	    int status;
	    waitpid(nPid2, &status, 0);				
	}
    }

	exit(0);
}

int scanLine(FILE * fd, int * commandNo)
{
    char commandStr[MAX_LINE];
    pid_t nPid;
    
    /* Is stderr the best choice? I'm not sure */
    if (fprintf(stderr, "%s@myShell[%d]:%s>", getenv ("USER"), *commandNo, current_PATH) < 0) exit(0);
	fflush(stderr);
	
    if (fgets(commandStr, MAX_LINE, fd) == NULL)
    {
    /* Handle EOF condition and return appropriate value */
    /* This also handles ^C while retrieving commands from prompt 
       as that specific condition will return to main */
        return -1;
    }
    if (commandStr[0]=='\n') {
    /* Handle empty strings because strcmp doesnt like them*/
        return 1;
    }
    
    (*commandNo)++;

    /* Check for a builtin */
    for (int i = 0; eigen[i].fun ; i++)
    {
	    int l = strlen(eigen[i].name);
	    if (l == 0) break;
	    if ((0 == strncmp(commandStr, eigen[i].name, l)) &&
			(isspace(commandStr[l])))
	    {
	    /* I think, we must have a builtin now, so call the
	       appropriate function and return.
	       This procedure will not work for the !n type
	       "builtins" */
	    return eigen[i].fun(commandStr);
	    }
    }
    /* I think this is where we fork. The code below should be executed by
       the child, while the parent waits */
    
   	nPid = fork();
    if (nPid == 0) 
    	parseCommand(commandStr); /*Child process */
    else if (nPid < 0) 
    {
       fprintf(stderr, "\nSome error occured in fork()\n");
      return 1;
    } 
    else 
    { 
	   /* Parent process */
       int status;
       
      //  fprintf(stderr, "Entering Wait PP\n");
	while(	wait(&status)!= -1);
    //    fprintf(stderr, "Exiting Wait PP\n");
    //  if (WIFEXITED(status)==0) return -1;
       return 1;
	
    }
  fprintf(stderr, "\nSome error occured in parseCommand, child returned\n");
  return 0;
}

int main (void) {   
    int commandNo = 0; 
    /* Specify Signal Handling */
    struct sigaction interrupt;
    interrupt.sa_handler = handleInterrupt;
    sigemptyset (&interrupt.sa_mask);
    interrupt.sa_flags = 0;
    sigaction (SIGINT, &interrupt, NULL); 
    
    /* Set 
    Path for use with source and cd */
    if (getcwd(current_PATH, MAX_LINE)==NULL) {
        fprintf(stderr, "\nPath to working directory is too long for poor simple shell\n");
        exit(0);
    }
    
    while (1) {
        if (INTERRUPT)
            fprintf(stderr, "\n^C detected, returning to parent prompt\n");
        INTERRUPT=0;
        int r = scanLine(stdin, &commandNo);
        if (r == -1) {
			printf("ERROR\n");
        }
        if (r == 0) {
            break;
        }
    }
}
