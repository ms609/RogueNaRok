/*  RogueNaRok is an algorithm for the identification of rogue taxa in a set of phylogenetic trees.
 *
 *  Moreover, the program collection comes with efficient implementations of
 *   * the unrooted leaf stability by Thorley and Wilkinson
 *   * the taxonomic instability index by Maddinson and Maddison
 *   * a maximum agreement subtree implementation (MAST) for unrooted trees
 *   * a tool for pruning taxa from a tree collection.
 *
 *  Copyright October 2011 by Andre J. Aberer
 *
 *  Tree I/O and parallel framework are derived from RAxML by Alexandros Stamatakis.
 *
 *  This program is free software; you may redistribute it and/or
 *  modify its under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  For any other inquiries send an Email to Andre J. Aberer
 *  andre.aberer at googlemail.com
 *
 *  When publishing work that is based on the results from RogueNaRok, please cite:
 *  Andre J. Aberer, Denis Krompaß, Alexandros Stamatakis. RogueNaRok: an Efficient and Exact Algorithm for Rogue Taxon Identification. (unpublished) 2011.
 *
 */


#include "Dropset.h"

uint32_t *randForTaxa = NULL;
extern int mxtips,
  maxDropsetSize,
  bitVectorLength;
extern BitVector *droppedTaxa,
  *neglectThose,
  *paddingBits;


void initializeRandForTaxa(int mxtips)
{
  int i;
  randForTaxa = CALLOC(mxtips, sizeof(uint32_t));
  FOR_0_LIMIT(i,mxtips)
    randForTaxa[i] = UINT32_MAX * unif_rand();
}


/* is ONLY done for adding OWN elements */
void addEventToDropsetPrime(Dropset *dropset, int a, int b)
{
  List
    *lIter;

  lIter = dropset->ownPrimeE;
  while(lIter)
    {
      List *next = lIter->next;
      MergingEvent *me = lIter->value;

      assert(NOT me->isComplex);

      if( me->mergingBipartitions.pair[0] == a
	  || me->mergingBipartitions.pair[1] == b
	  || me->mergingBipartitions.pair[1] == a
	  || me->mergingBipartitions.pair[0] == b)
	{
	  assert( (me->mergingBipartitions.pair[0] == a)
	  	  + (me->mergingBipartitions.pair[1] == b)
	  	  + (me->mergingBipartitions.pair[0] == b)
	  	  + (me->mergingBipartitions.pair[1] == a)
	  	  == 2);
	  return;
	}
      lIter = next;
    }

  MergingEvent
    *result = CALLOC(1,sizeof(MergingEvent));
  result->mergingBipartitions.pair[0] = b;
  result->mergingBipartitions.pair[1] = a;

  APPEND(result, dropset->ownPrimeE);
}


List *addEventToDropsetCombining(List *complexEvents, MergingBipartitions primeEvent)
{
  int
    a = primeEvent.pair[0],
    b = primeEvent.pair[1];

  List
    *firstElem = NULL,
    *secondElem = NULL;

  /* find list elems that already contain merging events  */
  List *iter ;

  iter = complexEvents;
  FOR_LIST(iter)
  {
    int res = isInIndexListSpecial(a,b,((MergingEvent*)iter->value)->mergingBipartitions.many);
    if(res)
      {
	if(NOT firstElem)
	  firstElem = iter;
	else if(NOT secondElem)
	  secondElem = iter;
	else break;
      }
  }


  if(firstElem && secondElem)
    {
      /* merge bips into first elem */
      IndexList
	*il = ((MergingEvent*)secondElem->value)->mergingBipartitions.many,
	*persistentIndexList = ((MergingEvent*)firstElem->value)->mergingBipartitions.many;
      FOR_LIST(il)
	persistentIndexList = appendToIndexListIfNotThere(il->index, persistentIndexList);
      ((MergingEvent*)firstElem->value)->mergingBipartitions.many = persistentIndexList;
      freeIndexList(((MergingEvent*)secondElem->value)->mergingBipartitions.many);
      free((MergingEvent*)secondElem->value);

      /* remove second element */
      if(complexEvents == secondElem)
	complexEvents = secondElem->next;
      else
	{
	  iter = complexEvents;
	  FOR_LIST(iter)
	  {
	    if(iter->next == secondElem)
	      {
		iter->next = iter->next->next;
		break;
	      }
	  }
	}
      free(secondElem);
    }
  else if(firstElem)
    {
      assert( NOT secondElem);
      MergingEvent *me = firstElem->value;
      me->mergingBipartitions.many = appendToIndexListIfNotThere(a,me->mergingBipartitions.many);
      me->mergingBipartitions.many = appendToIndexListIfNotThere(b,me->mergingBipartitions.many);
    }
  else
    {
      MergingEvent
	*me = CALLOC(1,sizeof(MergingEvent));
      me->isComplex = TRUE;

      me->mergingBipartitions.many = NULL;
      APPEND_INT(a,me->mergingBipartitions.many);
      APPEND_INT(b,me->mergingBipartitions.many);
      APPEND(me, complexEvents);
    }

  return complexEvents;
}


void freeDropsetDeepInHash(void *value)
{
  freeDropsetDeep(value, TRUE);
}


void freeDropsetDeep(void *value, boolean extended)
{
  Dropset
    *dropset = (Dropset*) value;
  List
    *iter = NULL;

  if(dropset->taxaToDrop)
    freeIndexList(dropset->taxaToDrop);

  /* free combined events */
  if(extended && dropset->complexEvents)
    {
      iter = dropset->complexEvents;
      while(iter)
	{
	  List *next = iter->next;
	  MergingEvent *me = iter->value;
	  freeIndexList(me->mergingBipartitions.many);
	  free(me);
	  iter = next;
	}
      freeListFlat(dropset->complexEvents);
    }

  if(extended && dropset->acquiredPrimeE)
    freeListFlat(dropset->acquiredPrimeE);

  /* ownPrimeE */
  iter = dropset->ownPrimeE;
  while(iter)
    {
      List *next = iter->next;
      free((MergingEvent*)iter->value);
      iter = next;
    }
  freeListFlat(dropset->ownPrimeE);

  free(dropset);
}


void freeDropsetDeepInEnd(void *value)
{
  Dropset
    *dropset = (Dropset*) value;
  List
    *iter = NULL;

  if(dropset->taxaToDrop)
    freeIndexList(dropset->taxaToDrop);

  /* free combined events */
  if(dropset->complexEvents)
    freeListFlat(dropset->complexEvents);

  if( dropset->acquiredPrimeE)
    freeListFlat(dropset->acquiredPrimeE);

  /* ownPrimeE */
  iter = dropset->ownPrimeE;
  while(iter)
    {
      List *next = iter->next;
      free((MergingEvent*)iter->value);
      iter = next;
    }
  freeListFlat(dropset->ownPrimeE);

  free(dropset);
}


#ifdef PRINT_VERY_VERBOSE
void printMergingEventStruct(MergingEvent **mergingEvents, int mxtips)
{
  int i;
  MergingEvent
    *iter_merge;
  IndexList *iter_index;
  PR("MERGING EVENT STRUCT:\n");

  FOR_0_LIMIT(i,mxtips)
    {
      PR("%d:\t",i);
      iter_merge = mergingEvents[i];
      FOR_LIST(iter_merge)
      {
	PR("{");
	iter_index = iter_merge->mergingBipartitions;
	FOR_LIST(iter_index)
	{
	  PR("%d,", iter_index->index);
	}
	PR(" : %d}  ", (iter_merge->supportGained - iter_merge->supportLost));
      }
      PR( "\n");
    }
  PR("\n");
}
#endif

uint32_t dropsetHashValue(HashTable *hashTable, void *value)
{
  uint32_t
    result = 0;

  Dropset *dropset = (Dropset*)value;
  IndexList *iter = dropset->taxaToDrop;

  FOR_LIST(iter)
  {
    assert(iter->index < mxtips);
    result ^= randForTaxa[ iter->index ];
  }

  return result;
}


boolean dropsetEqual(HashTable *hashtable, void *entryA, void *entryB)
{
  IndexList *aList = ((Dropset*)entryA)->taxaToDrop,
    *bList = ((Dropset*)entryB)->taxaToDrop;

  return indexListEqual(aList, bList);
}


IndexList *convertBitVectorToIndexList(BitVector *bv)
{
  int i;
  IndexList
    *result = NULL;

  FOR_0_LIMIT(i,mxtips)
    if(NTH_BIT_IS_SET(bv, i))
      APPEND_INT(i, result);

  return result;
}


boolean elementsEqual(ProfileElem *elemA,ProfileElem *elemB, int a, int b)
{
  return (elemA->id == a && elemB->id == b  )
    || (elemB->id == a && elemA->id == b) ;
}


IndexList *getDropset(ProfileElem *elemA, ProfileElem *elemB, boolean complement, BitVector *neglectThose)
{
  int i,j,
    numBit = 0,
    localBitCount,
    differenceByte;

  IndexList
    *result = NULL;

  if(elemA == elemB)
    return NULL;

  FOR_0_LIMIT(i,bitVectorLength)
    {
      if( complement)
	differenceByte = ~ ((elemA->bitVector[i] ^ elemB->bitVector[i]) |  ( droppedTaxa[i] | paddingBits[i] ));
      else
	differenceByte = elemA->bitVector[i] ^ elemB->bitVector[i];

      localBitCount = BIT_COUNT(differenceByte);

      if( (numBit += localBitCount) > maxDropsetSize)
	{
	  freeIndexList(result);
	  return NULL;
	}

      if( NOT localBitCount)
	continue;

      FOR_0_LIMIT(j,MASK_LENGTH)
	{
	  if(NOT localBitCount)
	    break;

	  if(NTH_BIT_IS_SET_IN_INT(differenceByte,j))
	    {
	      APPEND_INT((( i * MASK_LENGTH) + j),result);
	      localBitCount--;

	      if(NOT NTH_BIT_IS_SET(neglectThose, (i*MASK_LENGTH + j)))
		{
		  freeIndexList(result);
		  return NULL;
		}
	    }
	}
    }

  assert(numBit);

  return result;
}

