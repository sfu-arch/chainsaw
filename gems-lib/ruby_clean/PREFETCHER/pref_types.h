#ifndef INTERFACE_H
#define INTERFACE_H

#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ext/hash_map>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <cassert>

using namespace __gnu_cxx;
using namespace std;


#define UINT32      unsigned int
#define INT32       int
#define ADDRINT     unsigned long long
#define COUNTER     unsigned long long
#define CacheAddr_t unsigned long long



static const int BLK_WIDTH =  6;
static const int IDX_WIDTH =  8;
static const int TAG_WIDTH = 18;

static const int BLK_BITS = BLK_WIDTH;
static const int IDX_BITS = IDX_WIDTH + BLK_BITS;
static const int TAG_BITS = TAG_WIDTH + IDX_BITS;

static const CacheAddr_t BLK_SIZE = ((CacheAddr_t)1) << BLK_BITS;
static const CacheAddr_t IDX_SIZE = ((CacheAddr_t)1) << IDX_BITS;
static const CacheAddr_t TAG_SIZE = ((CacheAddr_t)1) << TAG_BITS;

static const CacheAddr_t BLK_MASK = (((CacheAddr_t)1) << BLK_WIDTH)-1;
static const CacheAddr_t IDX_MASK = (((CacheAddr_t)1) << IDX_WIDTH)-1;
static const CacheAddr_t TAG_MASK = (((CacheAddr_t)1) << TAG_WIDTH)-1;

static const int PREF_MSHR_SIZE = 32;


typedef struct
{
  COUNTER      LastRequestCycle;    // Cycle in which this request took place
  CacheAddr_t  LastRequestAddr;     // Program Counter of the instruction that made the request 
  COUNTER      SequenceNumber;      // Sequence number of the instruction that made the request
  INT32        AccessType;          // Request type (Load:1, Store:2)
  CacheAddr_t  DataAddr;            // Virtual address for the data being requested
  bool         hit;                 // Did the request hit in the cache?
  bool         LastRequestPrefetch; // Was this a prefetch request?
  UINT32       LRU;                 // Currently unimplemented: contains a bogus value, DO NOT use it 
} PrefetchData_AMPM_t;

#endif

