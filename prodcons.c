/* 
 * Operating Systems {2INCO} Practical Assignment
 * Condition Variables Application
 *
 * Sonya Zarkova 1034611
 * Stefan van der Heijden 0910541
 *
 * Grading:
 * Students who hand in clean code that fully satisfies the minimum requirements will get an 8. 
 * Extra steps can lead to higher marks because we want students to take the initiative. 
 * Extra steps can be, for example, in the form of measurements added to your code, a formal 
 * analysis of deadlock freeness etc.
 */
 
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "prodcons.h"

static ITEM buffer[BUFFER_SIZE];

static void rsleep (int t);			// already implemented (see below)
static ITEM get_next_item (void);	// already implemented (see below)
static int item = 0; // number of items that have been made
// The position indicates where a new item will be added to the buffer,
// but can also be used to get the last item in the buffer and check
// whether the buffer is full.
static int position = 0; // The position
// This is the mutex that is used by all producers and the consumer
// Such that the buffer and current position are only chaged in one
// thread at a time.
static pthread_mutex_t      mutex           = PTHREAD_MUTEX_INITIALIZER;
// This is the condition that signals a item has been added to the buffer
static pthread_cond_t       pcondition      = PTHREAD_COND_INITIALIZER;
// This is the condition that signals a item has been removed from the buffer
static pthread_cond_t       ccondition      = PTHREAD_COND_INITIALIZER;


// * put the item into buffer[]	
// MAKE SURE MUTEX IS LOCKED BEFORE CALLING!!!
static void addToBuffer()
{
	static ITEM pjob;
	pjob = get_next_item();
	// increment number of items, so that we know when to stop
	// CHECK if items < NROF_ITEMS?
	item = item+1;
	// Add next job to the buffer
	buffer[position] = pjob;
	position++;
	// Then signal condition, that says buffer is not empty
	pthread_cond_signal (&ccondition);
	// mutex-unlock;
	pthread_mutex_unlock (&mutex);
}

/* producer thread */
static void * 
producer (void * arg)
{
	// Keep going untill no more items can be produced
    while ( item < NROF_ITEMS)
    { 
		// * get the new item
		rsleep (100);	// simulating all kind of activities...
			
		// * put the item into buffer[]	
		// follow this pseudocode (according to the ConditionSynchronization lecture):
		if (position < BUFFER_SIZE){
			// mutex-lock;
			pthread_mutex_lock (&mutex);
			addToBuffer();
		}else{// If not wait for buffer to become empty
			//possible-cv-signals; ---------------wait for buffer to empty
			pthread_cond_wait (&pcondition, &mutex);
			addToBuffer();
		}
    }
	return (NULL);
}

// Print all item in the buffer
// MAKE SURE MUTEX IS LOCKED BEFORE CALLING!!!
static void clearBuffer()
{
	// Get all items from the buffer
	while(position > 0) {
		rsleep (100); // simulating all kind of activities...
		printf("%d\n", buffer[position - 1]); // then print item
		position--;
	}
	// Then signal condition, that says buffer is empty
	pthread_cond_signal (&pcondition);
	// unlock mutex
	pthread_mutex_unlock (&mutex);
}

/* consumer thread */
static void * 
consumer (void * arg)
{
	// keep going untill
    while ( item < NROF_ITEMS)
    {
		// If there is an item in the buffer
		if(position > 0){
			// Wait for lock
			pthread_mutex_lock (&mutex);
			clearBuffer();
		// If there is no item in the buffer wait for cond. var
		}else{
			pthread_cond_wait (&ccondition, &mutex);
			clearBuffer();
		}
    }
    // Make sure all items are taken from the buffer
    if(position > 0) {
		// Wait for lock
		pthread_mutex_lock (&mutex);
		clearBuffer();
	}
	return (NULL);
}

int main (void)
{
    // TODO: 
    // * startup the producer threads and the consumer thread
	pthread_t prod[NROF_PRODUCERS];
	pthread_t cons;
	int i;

	for(i=0; i<NROF_PRODUCERS; i++) {
  	pthread_create( &prod[i], NULL , producer , NULL);
	}

	
	pthread_create( &cons, NULL , consumer , NULL);

	

    // * wait until all threads are finished  
	for(int j=0; j<NROF_PRODUCERS; j++) {
		pthread_join (prod[j], NULL);
	}
	
	pthread_join (cons, NULL);
    
    return (0);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------already implemented

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void 
rsleep (int t)
{
    static bool first_call = true;
    
    if (first_call == true)
    {
        srandom (time(NULL));
        first_call = false;
    }
    usleep (random () % t);
}


/* 
 * get_next_item()
 *
 * description:
 *		thread-safe function to get a next job to be executed
 *		subsequent calls of get_next_item() yields the values 0..NROF_ITEMS-1 
 *		in arbitrary order 
 *		return value NROF_ITEMS indicates that all jobs have already been given
 * 
 * parameters:
 *		none
 *
 * return value:
 *		0..NROF_ITEMS-1: job number to be executed
 *		NROF_ITEMS:		 ready
 */


static ITEM
get_next_item(void)
{
    static pthread_mutex_t	job_mutex	= PTHREAD_MUTEX_INITIALIZER;
	static bool 			jobs[NROF_ITEMS+1] = { false };	// keep track of issued jobs
	static int              counter = 0;    // seq.nr. of job to be handled
    ITEM 					found;          // item to be returned
	
	/* avoid deadlock: when all producers are busy but none has the next expected item for the consumer 
	 * so requirement for get_next_item: when giving the (i+n)'th item, make sure that item (i) is going to be handled (with n=nrof-producers)
	 */
	pthread_mutex_lock (&job_mutex);

    counter++;
	if (counter > NROF_ITEMS)
	{
	    // we're ready
	    found = NROF_ITEMS;
	}
	else
	{
	    if (counter < NROF_PRODUCERS)
	    {
	        // for the first n-1 items: any job can be given
	        // e.g. "random() % NROF_ITEMS", but here we bias the lower items
	        found = (random() % (2*NROF_PRODUCERS)) % NROF_ITEMS;
	    }
	    else
	    {
	        // deadlock-avoidance: item 'counter - NROF_PRODUCERS' must be given now
	        found = counter - NROF_PRODUCERS;
	        if (jobs[found] == true)
	        {
	            // already handled, find a random one, with a bias for lower items
	            found = (counter + (random() % NROF_PRODUCERS)) % NROF_ITEMS;
	        }    
	    }
	    
	    // check if 'found' is really an unhandled item; 
	    // if not: find another one
	    if (jobs[found] == true)
	    {
	        // already handled, do linear search for the oldest
	        found = 0;
	        while (jobs[found] == true)
            {
                found++;
            }
	    }
	}
    jobs[found] = true;
	pthread_mutex_unlock (&job_mutex);
	return (found);
}


