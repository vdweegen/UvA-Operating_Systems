// Sample code by
// Dick van Albada
// November 2009
// (c) University of Amsterdam
//
// In this program we consider several ways to solve the dining
// philosophers problem in an environment using pthreads, mutexes and
// semaphores.
// We'll try to program various philosopher behaviours in such a way that
// different mixes of philosophers can be made to dine together.
// Each philosopher must:
// - find his position at the table, identify his left and right chopsticks
// - try to obtain chopsticks
// - wait (eat) for some time
// - put down the chopsticks
// - repeat this  until some condition is met
// - keep a log of successful/unsuccessful attempts
// In addition to the philosophers there must be a host task that
// - initialises the philosophers
// - try to identify deadlock if it occurs
// - signals the end of the simulation
// - coordinates reporting

// We can model the chopsticks by using mutexes - that will suffice
// We can use the lock, try and unlock calls.
// A sequence of locks can lead to a deadlock situation

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include "philosophers.h"

#define lock pthread_mutex_lock
#define unlock pthread_mutex_unlock
#define trylock pthread_mutex_trylock
#define LEFT 0
#define RIGHT 1
#define AL 1
#define AR 2
#define OP 3
#define SH 4
#define IN 5

// It is conventient to have some global variables that are shared by all
// threads

int Ndiners = 0;             // How many diners do we expect
int type = 0;
int rnd = 0;
static int Nregistered = 0;         // How many have arrived at the table
static int *occupiedSeats;			// Keeps track of the occupied seats
static int *theChopsticks;			// Keeps track of who hold which chopstick

// We'll have an array of mutexes to represent the chopsticks
static pthread_mutex_t * chopsticks;

// Three mutexes to shield various critical sections
static pthread_mutex_t beSeated = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t pay = PTHREAD_MUTEX_INITIALIZER;

/* Declare worker_tid */
pthread_t * worker_tid;

// Three arrays to represent some interesting aspects of the state of each
// philosopher
static int * holdsLeft;
static int * holdsRight;
static int * holdAndWait;

static int initChopsticks()
{
    int i;
    chopsticks = malloc(Ndiners * sizeof(chopsticks));
    holdsLeft = malloc(Ndiners * sizeof(int));
    holdsRight = malloc(Ndiners * sizeof(int));
    holdAndWait = malloc(Ndiners * sizeof(int));
    
    for (i = 0; i < Ndiners; i++)
    {
	holdsRight[i] = holdsLeft[i] = holdAndWait[i] = 0;
	if ((pthread_mutex_init(chopsticks + i, NULL)))
	{
            perror("Could not initialise all chopsticks");
            printf("i = %d\n", i);
            return -1;
        }
    }
    return 0;
}

static int getSeat()
{
    int mySeat = -1;
    lock(&beSeated);
    do
    {
	mySeat = (rand() % Ndiners) + 1;
	if(!occupiedSeats[mySeat-1])
	{
	    occupiedSeats[mySeat-1] = mySeat;
	    Nregistered++;
	    break;
	    printf("Another seat has been filled! [seat: %d]", Nregistered);
	}
    } while(1);
    
    unlock(&beSeated);
    return mySeat;
}

// If sched_yield does not exist, consider a usleep() or something similar
// BTW see http://kerneltrap.org/Linux/Using_sched_yield_Improperly
//
int yield()
{
    return sched_yield();
}

// Each diner thread will execute a diner function... 
static void * diner(void * args)
{
    //dinerArgs myArgs =  * (dinerArgs *) args;
    int mySeat = getSeat();
    while (!payNow)
    {
	rnd = 0;
	if(Nregistered == Ndiners)
	{
	    switch(type)
	    {
		case AR:
		aggressiveRight(theChopsticks[LEFT], theChopsticks[RIGHT], mySeat);
		break;
		
		case AL:
		aggressiveLeft(theChopsticks[LEFT], theChopsticks[RIGHT], mySeat);
		break;
		
		case OP:
		oppertunistic(theChopsticks[LEFT], theChopsticks[RIGHT], mySeat);
		break;
		
		case SH:
		shy(theChopsticks[LEFT], theChopsticks[RIGHT], mySeat);
		break;
	    
		case IN:
		/* Pick Random Behavure */
		srand(time(NULL));
		rnd = (rand() % 4) + 1;
		    switch(rnd)
		    {
			case 1:
			aggressiveRight(theChopsticks[LEFT], theChopsticks[RIGHT], mySeat);
			break;
			
			case 2:
			aggressiveLeft(theChopsticks[LEFT], theChopsticks[RIGHT], mySeat);
			break;
			
			case 3:
			oppertunistic(theChopsticks[LEFT], theChopsticks[RIGHT], mySeat);
			break;
			
			case 4:
			shy(theChopsticks[LEFT], theChopsticks[RIGHT], mySeat);
			break;
		    }
		break;
	    
		default:
		printf("OOOPPSSSS");
		exit(1);
		break;
	    }
	}
    }
    
    printf("Exiting....");
    
    /* Execute Cleanup */
    my_finale();
    
    pthread_exit(NULL);
    return NULL;
}

static void * my_finale()
{    
    for (int i = 0; i < Ndiners; i++) {
        if (pthread_join(worker_tid[i], NULL));
    }
    return NULL;
}
    
static int pickup_chopstick(int mySeat, int side)
{
    if(side == LEFT)
    {
	theChopsticks[LEFT] = mySeat;
	holdsLeft[mySeat] = 1;
    	printf("Philosopher at seat %d: I am Holding a Lefthanded Chopstick\n", mySeat);
    }
    else
    if(side == RIGHT)
    {
	theChopsticks[RIGHT] = mySeat;
	holdsRight[mySeat] = 1;
	printf("Philosopher at seat %d: I am Holding a Righthanded Chopstick\n", mySeat);
    }
    return 1;
}

static int drop_chopsticks(int mySeat)
{
    holdAndWait[mySeat] = 1;
    holdsLeft[mySeat] = 0;
    holdsRight[mySeat] = 0;
    theChopsticks[LEFT] = -1;
    theChopsticks[RIGHT] = -1;
    printf("Philosopher at seat %d: Noms finished\n", mySeat);
    return 1;
}

// Define several strategyFuns
static int aggressiveRight(int left, int right, int mySeat)
{
    if(!holdsRight[mySeat] && right < 0)
    {
    	lock(&chopsticks[RIGHT]);
    	if(!pickup_chopstick(mySeat, RIGHT))
    	{
    		/* Try/Catch basicly */
    		printf("Couldn't pickup chopstick... It is too heavy\n");
		return 0;
    	}
    	unlock(&chopsticks[RIGHT]);
    	yield();
    	return 1;
    }
    else
    if(!holdsLeft[mySeat] && holdsRight[mySeat] && left < 0)
    {
    	lock(&chopsticks[LEFT]);
    	if(!pickup_chopstick(mySeat, LEFT))
    	{
    		/* Try/Catch basicly */
    		printf("Couldn't pickup chopstick... It is too heavy\n");
		return 0;
    	}
    	unlock(&chopsticks[LEFT]);
	if(holdsRight[mySeat] && holdsLeft[mySeat])
	{
	    printf("Philosopher at seat %d: Got both chopsticks, nom nom nom....\n", mySeat);
	    drop_chopsticks(mySeat);
	    yield();
	}
	return 1;
    }
    return 1;
}

static int aggressiveLeft(int left, int right, int mySeat)
{
    if(!holdsLeft[mySeat] && left < 0)
    {
    	lock(&chopsticks[LEFT]);
    	if(!pickup_chopstick(mySeat, LEFT))
    	{
    		/* Try/Catch basicly */
    		printf("Couldn't pickup chopstick... It is too heavy\n");
		return 0;
    	}
    	unlock(&chopsticks[LEFT]);
	yield();
	return 1;
    }
    else
    if(!holdsRight[mySeat] && holdsLeft[mySeat] && right < 0)
    {
    	lock(&chopsticks[RIGHT]);
    	if(!pickup_chopstick(mySeat, RIGHT))
    	{
    		/* Try/Catch basicly */
    		printf("Couldn't pickup chopstick... It is too heavy\n");
		return 0;
    	}
    	unlock(&chopsticks[RIGHT]);
	
    if(holdsLeft[mySeat] && holdsRight[mySeat])
	{
	    printf("Philosopher at seat %d: Got both chopsticks, nom nom nom....\n", mySeat);
	    drop_chopsticks(mySeat);
	    yield();
	}
	return 1;
    }
    return 1;	
}

static int oppertunistic(int left, int right, int mySeat)
{
    /* right hand empty, and a righthanded chopstick is available */
    if(!holdsRight[mySeat] && right < 0)
    {
	/* trylock the right chopstick */
	if(trylock(&chopsticks[RIGHT]))
	{
	    printf("Philosopher at seat %d: I just locked a Righthanded Chopstick\n", mySeat);
	    if(!pickup_chopstick(mySeat, RIGHT))
	    {
    		/* Try/Catch basicly */
    		printf("Couldn't pickup chopstick... It is too heavy\n");
		return 0;
	    }
	    yield();
	    return 1;
	}
    }
    else
    /* left hand empty, and a lefthanded chopstick is available */
    if(!holdsLeft[mySeat] && left < 0)
    {
	/* trylock the left chopstick */
	if(trylock(&chopsticks[LEFT]))
	{
	    printf("Philosopher at seat %d: I just locked a Lefthanded Chopstick\n", mySeat);
	    if(!pickup_chopstick(mySeat, LEFT))
	    {
		/* Try/Catch basicly */
		printf("Couldn't pickup chopstick... It is too heavy\n");
		return 0;
	    }
	    yield();
	    return 1;
	}
    }
    else
    /* Got both, NOM NOM NOM */
    if(holdsRight[mySeat] && holdsLeft[mySeat])
    {
	printf("Philosopher at seat %d: Got both chopsticks, nom nom nom....\n", mySeat);
	drop_chopsticks(mySeat);
	/* Unlock the chopsticks (if locks are present) */
	unlock(&chopsticks[RIGHT]);
	unlock(&chopsticks[LEFT]);
	yield();
	return 1;
    }
    yield();
    return 1;
}

static int shy(int left, int right, int mySeat)
{
    /* Try pickup right chopstick */
    if(!holdsRight[mySeat] && right < 0)
    {
	/* trylock the right chopstick */
	if(trylock(&chopsticks[RIGHT]))
	{
	    printf("Philosopher at seat %d: I just locked a Righthanded Chopstick\n", mySeat);
	    if(!pickup_chopstick(mySeat, RIGHT))
	    {
    		/* Try/Catch basicly */
    		printf("Couldn't pickup chopstick... It is too heavy\n");
		yield();
		return 0;
	    }
	    else
	    {
		/* Try pickup left chopstick */
		if(!holdsLeft[mySeat] && left < 0)
		{
		    if(trylock(&chopsticks[LEFT]))
		    {
			printf("Philosopher at seat %d: I just locked the Lefthanded Chopstick also\n", mySeat);
			if(!pickup_chopstick(mySeat, RIGHT))
			{
			    /* Try/Catch basicly */
			    printf("Couldn't pickup chopstick... It is too heavy\n");
			    return 0;
			}
			printf("Philosopher at seat %d: Got both chopsticks, nom nom nom....\n", mySeat);
			drop_chopsticks(mySeat);
			/* Unlock the chopsticks (if locks are present) */
			unlock(&chopsticks[RIGHT]);
			unlock(&chopsticks[LEFT]);
			yield();
		    }
		    else
		    {
			printf("Philosopher at seat %d: Couldn't lock LEFT... Putting Right back also\n", mySeat);
			theChopsticks[RIGHT] = -1;
			holdsRight[mySeat] = 0;
			yield();
		    }
		    yield();
		}
		yield();
	    }
	    yield();
	}
	yield();
    }
    return 1;
}

static dinerArgs * UseArgs = NULL;	// Create an array with Ndiner entries

static pthread_attr_t	my_tattr;

int main(int argc, char ** argv)
{
    int i, j, deadlock, option = 0;
    
char input[2];          //Used to read user input.
    
    /* Initialize Chopsticks */
    initChopsticks();
    
    /* Allocate the Seats */
    occupiedSeats = malloc(Ndiners * sizeof(int));

    // Read various control parameters for the simulation...
    if(argc > 1)
    {
	if(!strcmp(argv[1],"--help"))
	{
	    printf("Welcome to the Philophers Executable\n\n");
	    printf("This program can simulate the behavure of certain types of philophers\n\n");
	    printf("There are 5 Types of Philosophers:\nL -> Aggressive Left\nR -> Aggressive Right\nO -> Oppertunistic\nS -> Shy\nI -> Indecisive\n\n");
	    printf("You can define different Philosophers as Arguments for the program\n\n");
	    printf("Example: ./philosophers 8\nThis would give you eight Philosophers\n\n");
	    printf("Have Fun!\n");
	    exit(1);
	}
	else
	{
	    Ndiners = atoi(argv[1]);
	    printf("\nI Should Make %d Philosophers\n\n", Ndiners);
	    while((option == 0) || (option > 5))
	    {
		fputs("What kind of Philosophers would you like?:\n", stdout);
		printf("1. Aggressive Left\n2. Aggressive Right\n3. Oppertunistic\n4. Shy\n5. Indecisive\n\n6. Exit\n");
		fflush(stdout);
		fgets(input, sizeof input, stdin);
		
		 //Convert input string to an integer value to compare in switch statment.
		sscanf(input, "%d", &option);
	    
		//Determine how data will be sorted based on option entered.
		switch(option)
		{
		    case 1:
		    type = AL;
		    break;
	    
		    case 2:
		    type = AR;
		    break;
	    
		    case 3:
		    type = OP;
		    break;
	    
		    case 4:
		    type = SH;
		    break;
	    
		    case 5:
		    type = IN;
		    break;
		
		    case 6:
		    exit(1);
		    break;
		    
		    default:
		    printf("Error! Invalid option selected!\n");
		    break;
		}   
	    }
	}
    }
    else
    {
	printf("philosophers: missing file operant\nTry 'philosophers --help' for more information\n");
	exit(1);
    }
    
    // Create the UseArgs array
    UseArgs = (dinerArgs *) calloc(Ndiners, sizeof(dinerArgs));
    worker_tid = (pthread_t *) calloc(Ndiners, sizeof(pthread_t));
    pthread_attr_init(&my_tattr);
    
    // Some more initiations
    occupiedSeats = malloc(Ndiners * sizeof(int));
    theChopsticks = malloc(Ndiners * sizeof(int));
    
    for(j = 0; j < sizeof(theChopsticks); j++)
    {
	/* Initialize all variables */
       	theChopsticks[j] = -1;
    	occupiedSeats[j] = 0;
    }
		
    // Now create our dining philosopher friends
    for (i = 0; i < Ndiners; i++)
    {
        UseArgs[i].strategy = NULL;
        if (pthread_create(worker_tid + i, &my_tattr, &diner, (void *)(UseArgs + i)))
        {
            fprintf(stderr, "Could not create thread %i: %s\n",
                    i, strerror(errno));
        }
    }

    // Monitor progress and deadlock regularly
    for (i = 0; i < 10000; i++)
    {
    	sleep(3);
	deadlock++;
	printf("Checking for deadlock [%d]....\n", deadlock);
       	if(deadlock >= 5)
	{
	    printf("Deadlock....\n");
	    printf("\tFixing Deadlock...\n");
	    printf("\t\tForcing all Philosophers to drop chopsticks\n");
	    for(j = 0; j < sizeof(theChopsticks); j++)
	    {
		theChopsticks[j] = -1;
	    }
	    return 0;
    	}
    }

    // Let the philosophers pay (by submitting their report) one at the time
    printf("Freeing Used Space...");
    free(UseArgs);
    UseArgs = NULL;
    free(worker_tid);
    worker_tid = NULL;
    payNow = 1;
    unlock(&pay);
}
