/*
 *  Besturingssystemen Opgave3
 *      Memory Management
 *      
 *  Author: Cas van der Weegen
 *		Johan Josua Storm
 *      Student Informatica
 *      Universiteit van Amsterdam
 *
 *  Date: 14 Mei 2013
 *
 *  File: first-fit.c
 *  Version:    0.1
 *
 */
#include <stdlib.h>
#include "mem_alloc.h"

/* move memory into memory-manager */
long *thememory;

void mem_init(long mem[MEM_SIZE]) /* size 32768, mem_alloc.h */
{
    /* if mem is unset -> break */
    if(!mem)
    {
        return;
    }
    /* make local pointer to the memory */
    thememory = mem;
    
    /* initialize all words at 0 */
    mem[0] = -1 * MEM_SIZE;
}

long  mem_get(long request)
{
    int i = 0;
    /* if index is unset -> break */
    if(!request)
    {
        return -1;
    }
    
    /* Check if we're still within the domain */
    for(i = 0; i < MEM_SIZE; i = abs(thememory[i]))
    {
        if (thememory[i] < 0)
        {
            /* If Match, allocate */
            if (abs(thememory[i]) - i - 1 == request + 1)
            {
                thememory[abs(thememory[i]) - 1] = thememory[i];
                thememory[i] = abs(thememory[i]) - 1;
                i = i + 1;
                return i;
            }
            else if(abs(thememory[i]) - i - 1 >= request)
            {
                if ((abs(thememory[i]) - i - 1 != request) && (i + request + 1 != MEM_SIZE))
                {
                    thememory[i + request + 1] = thememory[i];
                }
                thememory[i] = i + request + 1;
                i = i + 1;
                return i;
            }
            else
            {
                return (long)-1;
            }
        }
    }
    return (long)-1;
}

void mem_free(long index)
{
    int i = 0;
    
    /* break if index is unset or < 1 */
    if(!index || index < 1)
    {
        return;
    }
    /* Catch out of bounds */
    if(index > MEM_SIZE)
    {
        return;
    }
    /*
     * If thememory[index] -1 (previous index) is negative
     * or smaller than 0 is it free, and we can return immediately
     */
    if(thememory[index - 1] < 0)
    {
        return;
    }
    
    /*
     * Walk through memory until free space is reached,
     * Then merge with newly freed space
     */
    if (thememory[index - 1] < MEM_SIZE - 2 && thememory[thememory[index - 1]] == thememory[index - 1] + 1)
    {
        thememory[index - 1] = (thememory[index - 1] + 1);
    }
    
    
    if (i == index - 1)
    {
        /*
         * if we need to free, make contents negative,
         * then return
         */    
        thememory[i] = -1 * abs(thememory[i]);
        if (thememory[abs(thememory[i])] < 0)
        {
            thememory[i] = thememory[abs(thememory[i])];
        }
        return;
    }
    
    for(i = 0; i < index; i = abs(thememory[i]))
    {
        if (abs(thememory[i]) == index - 1)
        {
            thememory[abs(thememory[i])] = -1 * abs(thememory[abs(thememory[i])]);
            
            if (thememory[abs(thememory[abs(thememory[i])])] < 0)
            {
                /*
                 * i -> index (current)
                 * abs(thememory[i]) -> index of next block
                 * abs(thememory[abs(thememory[i])) -> index of the block after that
                 */
                thememory[abs(thememory[i])] = thememory[abs(thememory[abs(thememory[i])])];
            }
        
        /* Catch if we're at the beginning */    
        if (thememory[i] < 0)
        {
            thememory[i] = thememory[index - 1];
        }
        return;
    }
  }
}

double mem_internal()
{
    long c = (long)0;
    long total = (long)0;
    int i = 0;
    
    for(i = 0; i < MEM_SIZE; i = abs(thememory[i]))
    {
        if (thememory[i] > 0 || abs(thememory[i]) == i + 1)
        {
            c++;
            total = total + (thememory[i] - i);
        }
    }
    
    /* if total is unset -> break */
    if(!total)
    {
        return (double)-1;
    }
    
    return (c / (total * (double)1));
}

void mem_available(long *empty, long *large, long *n_holes)
{
    int i = 0;
    /* if any of the arguments are unset -> break */
    if(!empty)
    {
        return;
    }
    if(!large)
    {
        return;
    }
    if(!n_holes)
    {
        return;
    }
    /* Initialize pointers */
    *large = 0;
    *n_holes = 0;
    *empty = 0;
    
    for(i = 0; i < MEM_SIZE; i = abs(thememory[i]))
    {
        if ((thememory[i] < 0) && (abs(thememory[i]) != i + 1))
        {
            (*n_holes)++;
            *empty += abs(thememory[i]) - i - 1;
            if(abs(thememory[i]) - i - 1 > *large)
            {
                *large = abs(thememory[i]) - i - 1;
            }
    }
  }
}

void mem_exit()
{
    /* set pointer to NULL */
    thememory = NULL;
}