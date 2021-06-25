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

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"


typedef struct hash_el
{
  uint32_t fullKey;
  void *value;
  struct hash_el *next;
} HashElem;

typedef struct hash_table
{
  uint32_t tableSize;
  uint32_t entryCount;
  void *commonAttributes;
  uint32_t (*hashFunction)(struct hash_table *h, void *value);
  boolean (*equalFunction)(struct hash_table *hashtable, void *entrA, void *entryB);
  HashElem **table;
#ifdef PARALLEL
  pthread_mutex_t **lockPerSlot;
  pthread_mutex_t *cntLock;
#endif
} HashTable;

typedef struct
{
  HashTable *hashTable;
  HashElem *hashElem;
  uint32_t index;
} HashTableIterator;

#define FOR_HASH(htIter, hashtable)  boolean hasNext = TRUE; for(htIter = createHashTableIterator(hashtable); htIter && hasNext ; hasNext = hashTableIteratorNext(htIter))
#define FOR_HASH_2(htIter, hashtable)  hasNext = TRUE; for(htIter = createHashTableIterator(hashtable); hasNext ; hasNext = hashTableIteratorNext(htIter))

HashTable *createHashTable(uint32_t size, void *commonAttr, uint32_t (*hashFunction)(HashTable *hash_table, void *value), boolean (*equalFunction)(HashTable *hash_table, void *entryA, void *entryB));
HashTableIterator *createHashTableIterator(HashTable *hashTable);
void *getCurrentValueFromHashTableIterator(HashTableIterator *hashTableIterator);
boolean hashTableIteratorNext(HashTableIterator *hashTableIterator);
void *searchHashTableWithInt(HashTable *hashtable, uint32_t hashValue);
void *searchHashTable(HashTable *hashtable, void *value, uint32_t hashValue);
void insertIntoHashTable(HashTable *hashTable, void *value, uint32_t index);
boolean removeElementFromHash(HashTable *hashtable, void *value);
void destroyHashTable(HashTable *hashTable, void (*freeValue)(void *value));

#endif
