//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

//
// TODO:Student Information
//
const char *studentName = "Mahima Rathore";
const char *studentID   = "A53297810";
const char *email       = "mrathore@eng.ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of cache_sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of cache_sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of cache_sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // cache_block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//program variables
uint32_t offset_size;
uint32_t offset_mask;

uint32_t icache_index_mask;
uint32_t dcache_index_mask;
uint32_t l2cache_index_mask;

uint32_t icache_index_size;
uint32_t dcache_index_size;
uint32_t l2cache_index_size;

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//

//
//TODO: Add your Cache data structures here
//

typedef struct cache_block
{
  struct cache_block *prev_blk, *next_blk; //to maintain a queue like structure for easier lru replacement implementation
  uint32_t tag;
  uint8_t valid;
  uint8_t dirty;
}cache_block;
//Cache set implementation with 2 cache blocks for the front and back pointers for blocks of cache queue
typedef struct cache_set
{
  uint32_t size;
  cache_block *front_ptr, *rear_ptr;
}cache_set;


cache_set *icache;
cache_set *dcache;
cache_set *l2cache;

//------------------------------------//
//          Cache Functions           //
//------------------------------------//
//on evry miss, cache block is added to the set
cache_block* create_cache_block(uint32_t tag, uint8_t valid, uint8_t dirty)
{
  cache_block *blk = (cache_block*)malloc(sizeof(cache_block));
  blk->tag = tag;
  blk->valid = valid;
  blk->dirty = dirty;
  blk->prev_blk = NULL;
  blk->next_blk = NULL;

  return blk;
}
//This functions pops the front blk fromm the cache set queue
void pop_blk_from_set(cache_set* set){
  if(!set->size)//cache set is empty, nothing to pop
    return;
  //get the front blk in blk_x
  cache_block *blk_x = set->front_ptr;//replace the front_ptr with the second
  set->front_ptr = blk_x->next_blk;
  //element of cache queue as first element(front blk) is being poped
  if(set->front_ptr)
    set->front_ptr->prev_blk = NULL;

  (set->size)--;
  free(blk_x);
}
//the new cache block is added to the back of the cache set queue
//set->rear_ptr will now point to this new block
void push_blk_into_set(cache_set* set,  cache_block *blk)
{
  if(set->size)//cache set is not empty if set->rear = A blk and new blk is B,following code makes B as the rear most element of cache queue and pushes A one element front
  {
    set->rear_ptr->next_blk = blk;//A->next = B
    blk->prev_blk = set->rear_ptr;//B->prev = A
    set->rear_ptr = blk;//B is the last blk of set queue
  }
  else//set empty hence B is the only blk in the cache set queue
  {
    set->front_ptr = blk;
    set->rear_ptr = blk;
  }
  (set->size)++;
}

// Initialize the Cache Hierarchy
//
void
init_cache()
{
  // Initialize cache stats
  icacheRefs        = 0;
  icacheMisses      = 0;
  icachePenalties   = 0;
  dcacheRefs        = 0;
  dcacheMisses      = 0;
  dcachePenalties   = 0;
  l2cacheRefs       = 0;
  l2cacheMisses     = 0;
  l2cachePenalties  = 0;

  icache = (cache_set*)malloc(sizeof(cache_set) * icacheSets);
  dcache = (cache_set*)malloc(sizeof(cache_set) * dcacheSets);
  l2cache = (cache_set*)malloc(sizeof(cache_set) * l2cacheSets);

  for(int i=0; i<icacheSets; i++)
  {
    icache[i].size = 0;
    icache[i].front_ptr = NULL;
    icache[i].rear_ptr = NULL;
  }

  for(int i=0; i<dcacheSets; i++)
  {
    dcache[i].size = 0;
    dcache[i].front_ptr = NULL;
    dcache[i].rear_ptr = NULL;
  }

  for(int i=0; i<l2cacheSets; i++)
  {
    l2cache[i].size = 0;
    l2cache[i].front_ptr = NULL;
    l2cache[i].rear_ptr = NULL;
  }
    offset_size = (uint32_t)log2(blocksize);
    offset_size += ((1<<offset_size)==blocksize)? 0 : 1;
    offset_mask = (1 << offset_size) - 1;


    icache_index_size = (uint32_t)log2(icacheSets);
    icache_index_size += ((1<<icache_index_size)==icacheSets)? 0 : 1;
    dcache_index_size = (uint32_t)log2(dcacheSets);
    dcache_index_size += ((1<<dcache_index_size)==dcacheSets)? 0 : 1;
    l2cache_index_size = (uint32_t)log2(l2cacheSets);
    l2cache_index_size += ((1<<l2cache_index_size)==l2cacheSets)? 0 : 1;
    icache_index_mask = ((1 << icache_index_size) - 1) << offset_size;
    dcache_index_mask = ((1 << dcache_index_size) - 1) << offset_size;
    l2cache_index_mask = ((1 << l2cache_index_size) - 1) << offset_size;
}

//sets the correct order in the cache set queue after popping the blk at given index
cache_block* pop_set_index(cache_set* set, int index){
  if(index > set->size || index<0)//invalid index, do nothing
    return NULL;

  cache_block *blk_x = set->front_ptr;//get the front blk of cache set queue in blk_x

  if(set->size == 1){// only this blk in the queue
    set->front_ptr = NULL;
    set->rear_ptr = NULL;
  }
  else if (index == 0)//popped the front element, change the front_ptr to the second element
  {
    set->front_ptr = blk_x->next_blk;
    set->front_ptr->prev_blk = NULL;//Nothing before this blk
  }
  else if (index == set->size - 1)// popped the last(rearmost)element, change the rear_ptr to second last element
  {
    blk_x = set->rear_ptr;//blk_x becomes the last blk
    set->rear_ptr = set->rear_ptr->prev_blk;//make second last blk as last
    set->rear_ptr->next_blk = NULL;//Nothing after this blk
  }
  else{//popping blk from in between the cache set queue
    for(int i=0; i<index; i++)
      blk_x = blk_x->next_blk;//push all blks one blk forward i.e. towards the front
    blk_x->prev_blk->next_blk = blk_x->next_blk;
    blk_x->next_blk->prev_blk = blk_x->prev_blk;
  }

  blk_x->next_blk = NULL;
  blk_x->prev_blk = NULL;

  (set->size)--;
  return blk_x;
}

//invalidation cache functions
void invalidate_icache(uint32_t addr){//addr for L2 cache miss
  uint32_t offset = addr & offset_mask;
  uint32_t index = (addr & icache_index_mask) >> offset_size;
  uint32_t tag = addr >> (icache_index_size + offset_size);

  cache_block *blk_x = icache[index].front_ptr;

  for(int i=0; i<icache[index].size; i++){
    if(blk_x->tag == tag){//if tags match, evict that blk
      cache_block *blk = pop_set_index(&icache[index], i);
      free(blk);
      return;
    }
    blk_x = blk_x->next_blk;
  }
}

void invalidate_dcache(uint32_t addr){//addr for L2 cache miss
  uint32_t offset = addr & offset_mask;
  uint32_t index = (addr & dcache_index_mask) >> offset_size;
  uint32_t tag = addr >> (dcache_index_size + offset_size);

  cache_block *blk_x = dcache[index].front_ptr;

  for(int i=0; i<dcache[index].size; i++){
    if(blk_x->tag == tag){//if tags match, evict that blk
      cache_block *blk = pop_set_index(&dcache[index], i);
      free(blk);
      return;
    }
    blk_x = blk_x->next_blk;
  }
}
// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t icache_access(uint32_t addr)
{
  if(icacheSets==0){
    return l2cache_access(addr);
  }
  icacheRefs += 1;

  uint32_t offset = addr & offset_mask;
  uint32_t index = (addr & icache_index_mask) >> offset_size;
  uint32_t tag = addr >> (icache_index_size + offset_size);

  cache_block *blk_x = icache[index].front_ptr;

  for(int i=0; i<icache[index].size; i++){
    if(blk_x->tag == tag){ // Hit
      cache_block *blk = pop_set_index(&icache[index], i);//pop this blk out of the cache queue,set the blks in order after popping
      push_blk_into_set(&icache[index],  blk);//push this blk to the rear end of the cache set queue-most recently used
      return icacheHitTime;
    }
    blk_x = blk_x->next_blk;
  }

  icacheMisses += 1;

  uint32_t penalty = l2cache_access(addr);
  icachePenalties += penalty;

  cache_block *blk = create_cache_block(tag,1,0);

  if(icache[index].size == icacheAssoc)
    pop_blk_from_set(&icache[index]);//pop the front blk-least recently used
  push_blk_into_set(&icache[index],  blk);//push the newly created blk to the end -most recently used

  return penalty + icacheHitTime;
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t dcache_access(uint32_t addr)
{
  if(dcacheSets==0){
    return l2cache_access(addr);
  }
  dcacheRefs += 1;

  uint32_t offset = addr & offset_mask;
  uint32_t index = (addr & dcache_index_mask) >> offset_size;
  uint32_t tag = addr >> (dcache_index_size + offset_size);

  cache_block *blk_x = dcache[index].front_ptr;

  for(int i=0; i<dcache[index].size; i++){
    if(blk_x->tag == tag){
      cache_block *blk = pop_set_index(&dcache[index], i);//pop this blk out of the cache queue,set the blks in order after popping
      push_blk_into_set(&dcache[index],  blk);//push this blk to the rear end of the cache set queue-most recently used
      return dcacheHitTime;
    }
    blk_x = blk_x->next_blk;
  }

  dcacheMisses += 1;


  uint32_t penalty = l2cache_access(addr);
  dcachePenalties += penalty;

  cache_block *blk = create_cache_block(tag,1,0);

  if(dcache[index].size == dcacheAssoc)
    pop_blk_from_set(&dcache[index]);//pop the front blk-least recently used
  push_blk_into_set(&dcache[index],  blk);//push this blk to the rear end of the cache set queue-most recently used

  return penalty + dcacheHitTime;
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr)
{
  if(l2cacheSets==0){
    return memspeed;
  }
  l2cacheRefs += 1;

  uint32_t offset = addr & offset_mask;
  uint32_t index = (addr & l2cache_index_mask) >> offset_size;
  uint32_t tag = addr >> (l2cache_index_size + offset_size);

  cache_block *blk_x = l2cache[index].front_ptr;

  for(int i=0; i<l2cache[index].size; i++){
    if(blk_x->tag == tag){
      cache_block *blk = pop_set_index(&l2cache[index], i);//pop this blk out of the cache queue,set the blks in order after popping
      push_blk_into_set(&l2cache[index],  blk);//push this blk to the rear end of the cache set queue-most recently used
      return l2cacheHitTime;
    }
    blk_x = blk_x->next_blk;
  }

  l2cacheMisses += 1;

  cache_block *blk = create_cache_block(tag,1,0);

  if(l2cache[index].size == l2cacheAssoc){
    if(inclusive){

      uint32_t remove_blk = (((l2cache[index].front_ptr->tag)<<l2cache_index_size) + index)<<offset_size;
      invalidate_icache(remove_blk);
      invalidate_dcache(remove_blk);
    }
    pop_blk_from_set(&l2cache[index]);//pop the front blk-least recently used

  }
  push_blk_into_set(&l2cache[index],  blk);//push this blk to the rear end of the cache set queue-most recently used

  l2cachePenalties += memspeed;
  return memspeed + l2cacheHitTime;
}
