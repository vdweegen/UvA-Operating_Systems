/*
	names: 			Cas van der Weegen & Johan Josua Storm
	Studentnumber: 	6055338 & 6283934
	Assignment 1, april 15 2013
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "scanner.h"
#include "func.h"

#define MAX_ARGS	(1024)
#define MAX_LINE	(2 * MAX_ARGS)

int do_exit(char *command){
        command = NULL;
        fprintf(stderr,"Exiting now...\n");
        return 0;
}

int do_cd(char *command){
     
    char slash[2] = "/\0";
    char * args;
    char temp_PATH[MAX_LINE];
    
    args = strtok(command, " \t\n");
    if ((args = strtok(NULL, " \t\n")) == NULL) {
        fprintf(stderr, "Usage: cd <Directory>\n");
        return 1;
    }

    if (args[0]=='/') {
        strcpy(temp_PATH, args);
    } else {
        if (strlen(current_PATH)+strlen(args)+1 >= MAX_LINE) {
            fprintf(stderr, "Path is too long...\n");
            return 1;
        }
        strcat(strcat(strcpy(temp_PATH, current_PATH), slash), args); 
    }
   
    /*Does the directory excist?*/    
    struct stat st;
    if(stat(temp_PATH ,&st) == 0) {
        char *temp= &temp_PATH[0];
        while (temp[1]=='/') temp = &temp[1];
 
        if ( chdir(temp) == -1 )
        	fprintf(stderr, "There is a problem with chdir...\n");
 		if ( getcwd(current_PATH, MAX_LINE)==NULL)
        	fprintf(stderr, "\nPath is too long...\n");
        return 1;
    } else {
       fprintf(stderr, "Given directory doesn't exist...\n");
       return 1; 
    }
}

int do_source(char *command){
        int commandnr = 0;
        int r;
        char slashes[2] = "/\0";
        char *args;
        char tmp_PATH[MAX_LINE];
        struct stat st;
        
        FILE * fp;
        
        args = strtok(command, " \t\n");
        if ((args = strtok(NULL, " \t\n")) == NULL ){
                fprintf(stderr, "Usage: source <file>\n");
                return 1;
        }
        if (args[0] == '/'){
                strcpy(tmp_PATH, args);
        }
        else{
                if (strlen(current_PATH)+strlen(args)+1 >= MAX_LINE){
                        fprintf(stderr, "Path is too long...\n");
                        return 1;
                }
                strcat(strcat(strcpy(tmp_PATH, current_PATH), slashes), args);
        }
        
        /*Does the file exist?*/
        if(stat(tmp_PATH , &st)){
                fprintf(stderr, "File does not exist!\n");
                return 1;
        }
        fp = fopen(tmp_PATH, "r");
        fprintf(stderr, "\n");
        while(1){
                r = scanLine(fp, &commandnr);
                if(INTERRUPT){
                        return 1;
                }
                if ( r == -1){
                        fprintf(stderr, "\n");
                        return 1;
                }
                if (r == 0){
                        return 0;
                }
                
        }
}

void handleInterrupt(int signum) {
    signum = signum;
    INTERRUPT = 1;

      
}

