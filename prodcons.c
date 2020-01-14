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
static ITEM item = 0; 
static ITEM pjob; //job to be done
static ITEM cjob;
static ITEM pposition = 0;
static ITEM cposition = BUFFER_SIZE;
static pthread_mutex_t      pmutex          = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t       pcondition      = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t      cmutex          = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t       ccondition      = PTHREAD_COND_INITIALIZER;



/* producer thread */
static void * 
producer (void * arg)
{
    while ( item < NROF_ITEMS /* TODO: not all items produced */)
    { 

	
 
        // TODO: 
        // * get the new item
	pjob = get_next_item();
	printf(" job  %d\n", pjob);
		
      	rsleep (100);	// simulating all kind of activities...
		
	// TODO:
	// * put the item into buffer[]	
        // follow this pseudocode (according to the ConditionSynchronization lecture):
        //      mutex-lock;
  	pthread_mutex_lock (&pmutex);
  	printf ("                    locking producer\n");

        //      while not condition-for-this-producer
	while ( pposition >= BUFFER_SIZE ){
		
	// wait-cv;
        //      critical-section;

        //      possible-cv-signals;
	pthread_cond_wait (&pcondition, &pmutex);
	
	}
	if (pposition < BUFFER_SIZE){
		buffer[pposition] = pjob;
		pposition++;
		cposition++;
	}
        
        //      mutex-unlock;

	pthread_mutex_unlock (&pmutex);
  	  printf ("unlocking producer\n");
	pthread_cond_signal (&ccondition);
        //
        // (see condition_test() in condition_basics.c how to use condition variables)

	item = item+1; 
	//printf(" item %d\n", item);
    }
	return (NULL);
}

/* consumer thread */
static void * 
consumer (void * arg)
{
    while (  true /* TODO: not all items retrieved from buffer[] */)
    {
        // TODO: 
		// * get the next item from buffer[]
	
		// * print the number to stdout
	printf(" got job number: %d\n" , cjob);
        //
        // follow this pseudocode (according to the ConditionSynchronization lecture):
        //      mutex-lock;

	pthread_mutex_lock (&cmutex);
  	printf ("                    locking consumer\n");
        //      while not condition-for-this-consumer
	while(buffer[0] == -1){
		 //          wait-cv;
		pthread_cond_wait (&ccondition, &cmutex);

	} //while end
       
        //      critical-section;
	
	if (cposition >  0){
		cjob = buffer[cposition];	
		buffer[cposition]= -1;
		cposition--;
		pposition--;
	}
        //      possible-cv-signals;
        //      mutex-unlock;

	pthread_mutex_unlock (&cmutex);
  	  printf ("unlocking consumer\n");
	pthread_cond_signal (&pcondition);
		
        rsleep (100);		// simulating all kind of activities...
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


