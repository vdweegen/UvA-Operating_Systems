/* This file contains a bare bones skeleton for the scheduler function
 *  for the second assignment for the OSN course of the 2005 fall 
 *  semester.
 *  Author:
 *
 *  G.D. van Albada
 *  Section Computational Science
 *  Universiteit van Amsterdam
 *  
 *  Cas van der Weegen [6055338]
 *  Student Computer Sciences
 *  University of Amsterdam
 *  
 *  Johan Josua Storm [6283934]
 *  Student Computer Sciences
 *  University of Amsterdam
 *
 *  Date:
 *  October 23, 2003
 *  Modified:
 *  May 01, 2013
 */
#include <stdio.h>
#include <stdlib.h>

#include "schedule.h"
#include "mem_alloc.h"

/* This variable will simulate the allocatable memory */
static long memory[MEM_SIZE];
static double timeslice = 0.0;

/* The actual CPU scheduler is implemented here */
static void CPU_scheduler()
{
   /* Check if timeslice has been set */
   if(timeslice == 0.0)
   {
      printf("Geef de gewenste Timeslice grootte op: (double) ");
      scanf("%lf", &timeslice);
      if((int)timeslice < 1)
      {
         timeslice = 1.000000;
      }
      printf("Gelezen waarde: %lf\n", timeslice);
   }
   
   /* Hier 'ook' *proc3 verwijderd  */
   pcb *proc1, *proc2;
   proc1 = ready_proc;
   proc2 = ready_proc;
   
   while(proc1)
   {
      proc1 = proc1->next;   
   }
   
   /*
    * Return als proc == ready_proc,
    * dit zou namelijk betekenen dat proc
    * als door de loop gegaan is
    */
   if (proc2 == ready_proc)
   {
      return;
   }
   
   if (ready_proc->next == proc2)
   {
      /* Move along */
      ready_proc->next = proc2->next;
      ready_proc->prev = proc2;
      
      /* Proces is ready */
      proc2->next = ready_proc;
      proc2->prev = NULL;
      
      /* Er zijn geen processen meer */
      if (proc2->next != NULL)
      {
         proc2->next->prev = ready_proc;
      }
   }
   else
   {
      /* Move along */
      ready_proc->prev = proc2->prev;
      proc2->prev = ready_proc->next;
      ready_proc->next = proc2->next;
      
      /* Reset */
      proc2->next = proc2->prev;
      proc2->prev = NULL;
      
      /* Proces is ready */
      ready_proc->prev->next = ready_proc;
      
      /* Er zijn geen processen meer */
      if (ready_proc->next != NULL)
      {
         ready_proc->next->prev = ready_proc;   
      }
   }
   ready_proc = proc2;
}

static void rr()
{   
   pcb *proc = ready_proc;
   /* Breek af als Proc NULL is of al is afgerond */
   if (proc == NULL || proc == ready_proc)
   {
      return;
   }
   
   /* Kijk of er een proc klaarstaan */
   while(proc->next != NULL)
   {
      proc = proc->next;
   }
   
   /* Huidige proc is aan de beurt */
   proc->next = ready_proc;
   /* Proc die klaar stond word de eerstvolgende */
   ready_proc->prev = proc;
   /* Proc is klaar en word op ready gezet */
   ready_proc = ready_proc->next;
   
   /* Alles op NULL zetten voor de volgende PASS */
   proc->next->next = NULL;
   ready_proc->prev = NULL;   
}

static void GiveMemory()
{
   int index;
   /* Note: *proc3 verwijderd */
   pcb *proc1, *proc2;
   proc2 = new_proc;

   while(proc2)
   {
      /*
       * Search for a new process that should be given memory.
       * Insert search code and criteria here.
       * Attempt to allocate as follows:
       */
      index = mem_get(proc2->MEM_need);
      if (index >= 0)
      {
         /* Allocation succeeded, now put in administration */
         proc2->MEM_base = index;

         if(new_proc == proc2)
         {
            new_proc = proc2->next;
            if(proc2->next != NULL)
            {
               proc2->next->prev = NULL;
            }
         }
         else
         {
            proc2->prev->next = proc2->next;
            if(proc2->next != NULL)
            {
               proc2->next->prev = proc2->prev;
            }
         }
         
         /* Add to ready queue */
         if(ready_proc == NULL)
         {
            ready_proc = proc2;
            proc2->next = NULL;
            proc2->prev = NULL;
         }
         else
         {
            proc1 = ready_proc;
            while(proc1->next != NULL)
            {
               proc1 = proc1->next;
            }
            proc1->next = proc2;
            proc2->prev = proc1;
            proc2->next = NULL;
         }
         proc2 = new_proc;
      }
      else
      {
         proc2 = proc2->next;
      }
   }
}

/* Here we reclaim the memory of a process after it has finished */
static void ReclaimMemory()
{
    pcb *proc;

    proc = defunct_proc;
    while (proc)
    {
        /* Free the simulated allocated memory */
        mem_free(proc->MEM_base);
        proc->MEM_base = -1;

        /* Call the function that cleans up the simulated process */
        rm_process(&proc);

        /* See if there are more processes to be removed */
        proc = defunct_proc;
    }
}

static void my_finale()
{
   double simtime = sim_time();
   simtime = (simtime / 1000.000000);
   printf("Simulation Time: %lf Seconden\n",simtime);
}

/* The main scheduling routine */
void schedule(event_type event)
{
   static int first = 1;

   if(first == 1)
   {
      mem_init(memory);
      finale = my_finale;
      first = 0;
   }
   
   /* I did this differently */
   if(event == NewProcess_event)
   {
      GiveMemory();
   }
   else if(event == Time_event)
   {
      /* Here we should call RoundRobin */
      rr();
      CPU_scheduler();
   }
   else if(event == IO_event)
   {
      CPU_scheduler();
   }
   else if(event == Ready_event)
   {
      /* do nothing */
   }
   else if(event == Finish_event)
   {
      ReclaimMemory();
      GiveMemory();
      CPU_scheduler();
   }
   else
   {
      printf("I cannot handle event nr. %d\n", event);
   }
}

