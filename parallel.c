#ifdef PARALLEL
#include <stdio.h>
#include <pthread.h>
#include "common.h"
#include "ProfileElem.h"
#include "Dropset.h"
#include "parallel.h"


void findCandidatesForBip(HashTable *mergingHash, ProfileElem *elemA, boolean firstMerge, Array *bipartitionsById, Array *bipartitionProfile, int* indexByNumberBits); 
void combineEventsForOneDropset(Array *allDropsets, Dropset *refDropset, Array *bipartitionsById);
int getSupportOfMRETree(Array *bipartitionsById,  Dropset *dropset);
extern int cumScore; 

#ifndef PORTABLE_PTHREADS
void pinToCore(int tid)
{
  cpu_set_t cpuset;
         
  CPU_ZERO(&cpuset);    
  CPU_SET(tid, &cpuset);

  if(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0)
    {
      printBothOpen("\n\nThere was a problem finding a physical core for thread number %d to run on.\n", tid);
      printBothOpen("Probably this happend because you are trying to run more threads than you have cores available,\n");
      printBothOpen("which is a thing you should never ever do again, good bye .... \n\n");
      assert(0);
    }
}
#endif


void execFunction(parallelArguments *pArgs, int tid, int n)
{
  int currentJob = threadJob >> 16;

  switch(currentJob)
    {
    case THREAD_GET_EVENTS: 
      {
	HashTable *mergingHash = pArgs->mergingHash; 
	BitVector *candidateBips = pArgs->candidateBips; 
	Array *bipartitionsById = pArgs->bipartitionsById; 
	Array *bipartitionProfile = pArgs->bipartitionProfile; 
	int *indexByNumberBits = pArgs->indexByNumberBits; 	
	boolean firstMerge = pArgs->firstMerge ; 
	

	boolean done = FALSE ;
	while (NOT done ) 
	  {
	    int jobId = bipartitionProfile->length; 
	    ProfileElem *jobElem = NULL ; 
	    pthread_mutex_lock(&mutex);
	    do
	      {	
		jobId = bipartitionProfile->length - numberOfJobs; 
		numberOfJobs--; 				
	      }
	    while(numberOfJobs > 0 && NOT NTH_BIT_IS_SET(candidateBips, jobId));
	    done = (numberOfJobs <= 0);
	    pthread_mutex_unlock(&mutex);
	    
	    if( bipartitionProfile->length > jobId &&  (jobElem = GET_PROFILE_ELEM(bipartitionsById, jobId)) ) 
	      {
		assert(jobElem);
		findCandidatesForBip(mergingHash, jobElem, firstMerge , bipartitionsById, bipartitionProfile, indexByNumberBits);
	      }
	  }	  
	break;
      }
    case THREAD_COMBINE_EVENTS:
      {
	Array *allDropsets = globalPArgs->allDropsets; 
	Array *bipartitionsById = globalPArgs->bipartitionsById; 
	int jobId = allDropsets->length; 

	boolean done = FALSE ;
	while (NOT done ) 
	  {
	    pthread_mutex_lock(&mutex);
	    jobId = allDropsets->length - numberOfJobs; 
	    numberOfJobs--; 
	    done = (numberOfJobs <= 0);
	    pthread_mutex_unlock(&mutex);

	    if( allDropsets->length > jobId )
	      combineEventsForOneDropset(allDropsets, GET_DROPSET_ELEM(allDropsets,jobId), bipartitionsById);
	  }	  
	break;
      }
    case THREAD_MRE:
      {
	Array *allDropsets = globalPArgs->allDropsets,
	  *bipartitionsById = globalPArgs->bipartitionsById ;
	int jobId = allDropsets->length; 	
	
	boolean done = FALSE ;
	while (NOT done ) 
	  {
	    pthread_mutex_lock(&mutex);
	    jobId = allDropsets->length - numberOfJobs; 
	    numberOfJobs--; 
	    done = (numberOfJobs <= 0);
	    pthread_mutex_unlock(&mutex);

	    if( allDropsets->length > jobId )	      
	      {
		Dropset *dropset = GET_DROPSET_ELEM(allDropsets,jobId);
		int newSup  = getSupportOfMRETree(bipartitionsById, dropset);
		dropset->improvement = newSup - cumScore;  
	      } 
	  }

	break;
      }
    default:
	printf("Job %d\n", currentJob);
	assert(0);
    }
}


void *workerThreadWait(void *tData)
{
  threadData *td = (threadData*)tData;
  parallelArguments
    *pArgs = td->pArgs;
  int
    myCycle = 0;

  const int 
    n = numberOfThreads,
    tid = td->threadNumber;

#ifndef PORTABLE_PTHREADS
  pinToCore(tid);
#endif
 
  printf("\nThis is worker thread number: %d\n", tid);

  while(1)
    {
      while (myCycle == threadJob) 
	;
      myCycle = threadJob;
      execFunction(pArgs, tid, n);   
      barrierBuffer[tid] = 1; 
      
    }

  return (void*)NULL;
}


void startThreads()
{

  pthread_t *threads;
  pthread_attr_t attr;
  int rc, t;
  threadData *tData;

  jobCycle        = 0;
  threadJob       = 0;

  printf("\nThis is the master thread\n");

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  pthread_mutex_init(&mutex , (pthread_mutexattr_t *)NULL);
  threads = (pthread_t *)CALLOC(numberOfThreads , sizeof(pthread_t));
  tData = (threadData *)CALLOC(numberOfThreads , sizeof(threadData));  
  barrierBuffer = (volatile char *)CALLOC(numberOfThreads, sizeof(volatile char));
  
  for(t = 0; t < numberOfThreads; t++)
    barrierBuffer[t] = 0;
 
  for(t = 1; t < numberOfThreads; t++)
    {
      tData[t].pArgs  = globalPArgs;
      tData[t].threadNumber = t;
      rc = pthread_create(&threads[t], &attr, workerThreadWait, (void *)(&tData[t]));
      if(rc)
	{
	  printf("ERROR; return code from pthread_create() is %d\n", rc);
	  exit(-1);
	}
    }  
}


void masterBarrier(int jobType, parallelArguments *pArgs)
{
  const int 
    n = numberOfThreads;
  
  int 
    i, 
    sum;

  jobCycle = NOT jobCycle;
  threadJob = (jobType << 16) + jobCycle;

  execFunction(pArgs, 0, n);
 
  do
    {
      for(i = 1, sum = 1; i < n; i++)
	sum += barrierBuffer[i];
    }
  while(sum < n);  

  for(i = 1; i < n; i++)
    barrierBuffer[i] = 0;
}
#else
#endif
