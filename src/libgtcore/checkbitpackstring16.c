/*
** autogenerated content - DO NOT EDIT
*/
/*
** Copyright (C) 2007 Thomas Jahns <Thomas.Jahns@gmx.net>
**
** See LICENSE file or http://genometools.org/license.html for license details.
**
*/
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <time.h>
#include <sys/time.h>

#include "libgtcore/bitpackstring.h"
#include "libgtcore/env.h"
#include "libgtcore/ensure.h"

enum {
/*   MAX_RND_NUMS = 10, */
  MAX_RND_NUMS = 100000,
};

static inline int
icmp(uint16_t a, uint16_t b)
{
  if (a > b)
    return 1;
  else if (a < b)
    return -1;
  else /* if (a == b) */
    return 0;
}

#define freeResourcesAndReturn(retval) \
  do {                                 \
    env_ma_free(randSrc, env);         \
    env_ma_free(randCmp, env);         \
    env_ma_free(bitStore, env);        \
    env_ma_free(bitStoreCopy, env);    \
    return retval;                     \
  } while(0)


int
bitPackStringInt16_unit_test(Env *env)
{
  BitString bitStore = NULL;
  BitString bitStoreCopy = NULL;
  uint16_t *randSrc = NULL; /*< create random ints here for input as bit
                                *  store */
  uint16_t *randCmp = NULL; /*< used for random ints read back */
  size_t i, numRnd;
  BitOffset offsetStart, offset;
  unsigned long seedval;
  int had_err = 0;
  {
    struct timeval seed;
    gettimeofday(&seed, NULL);
    seedval = seed.tv_sec + seed.tv_usec;
    srandom(seedval);
  }
  offset = offsetStart = random()%(sizeof (uint16_t) * CHAR_BIT);
  numRnd = random() % (MAX_RND_NUMS + 1);
  env_log_log(env, "seedval = %lu, offset=%lu, numRnd=%lu\n", seedval,
              (long unsigned)offsetStart, (long unsigned)numRnd);
  {
    BitOffset numBits = sizeof (uint16_t) * CHAR_BIT * numRnd + offsetStart;
    randSrc = env_ma_malloc(env, sizeof (uint16_t)*numRnd);
    bitStore = env_ma_malloc(env, bitElemsAllocSize(numBits)
                             * sizeof (BitElem));
    bitStoreCopy = env_ma_calloc(env, bitElemsAllocSize(numBits),
                                 sizeof (BitElem));
    randCmp = env_ma_malloc(env, sizeof (uint16_t)*numRnd);
  }
  /* first test unsigned types */
  env_log_log(env, "bsStoreUInt16/bsGetUInt16: ");
  for (i = 0; i < numRnd; ++i)
  {
#if 16 > 32 && LONG_BIT < 16
    uint16_t v = randSrc[i] = (uint16_t)random() << 32 | random();
#else /* 16 > 32 && LONG_BIT < 16 */
    uint16_t v = randSrc[i] = random();
#endif /* 16 > 32 && LONG_BIT < 16 */
    int bits = requiredUInt16Bits(v);
    bsStoreUInt16(bitStore, offset, bits, v);
    offset += bits;
  }
  offset = offsetStart;
  for (i = 0; i < numRnd; ++i)
  {
    uint16_t v = randSrc[i];
    int bits = requiredUInt16Bits(v);
    uint16_t r = bsGetUInt16(bitStore, offset, bits);
    ensure(had_err, r == v);
    if (had_err)
    {
      env_log_log(env, "Expected %"PRIu16", got %"PRIu16
                  ", seed = %lu, i = %lu\n",
                  v, r, seedval, (unsigned long)i);
      freeResourcesAndReturn(had_err);
    }
    offset += bits;
  }
  env_log_log(env, "passed\n");
  if(numRnd > 1)
  {
    env_log_log(env, "bsCompare: ");
    {
      uint16_t v0 = randSrc[0];
      int bits0 = requiredUInt16Bits(v0);
      uint16_t r0;
      offset = offsetStart;
      r0 = bsGetUInt16(bitStore, offset, bits0);
      for (i = 1; i < numRnd; ++i)
      {
        uint16_t v1 = randSrc[i];
        int bits1 = requiredUInt16Bits(v1);
        uint16_t r1 = bsGetUInt16(bitStore, offset + bits0, bits1);
        int result = -2;   /*< -2 is not a return value of bsCompare, thus
                            *   if it is displayed, there was an earlier
                            *   error. */
        ensure(had_err, r0 == v0 && r1 == v1);
        ensure(had_err, icmp(v0, v1) ==
               (result = bsCompare(bitStore, offset, bits0,
                                   bitStore, offset + bits0, bits1)));
        if (had_err)
        {
          env_log_log(env, "Expected v0 %s v1, got v0 %s v1,\n for v0=%"
                      PRIu16" and v1=%"PRIu16",\n"
                      "seed = %lu, i = %lu, bits0=%u, bits1=%u\n",
                      (v0 > v1?">":(v0 < v1?"<":"==")),
                      (result > 0?">":(result < 0?"<":"==")), v0, v1,
                      seedval, (unsigned long)i, bits0, bits1);
          freeResourcesAndReturn(had_err);
        }
        offset += bits0;
        bits0 = bits1;
        v0 = v1;
        r0 = r1;
      }
    }
    env_log_log(env, "passed\n");
  }
  env_log_log(env, "bsStoreUniformUInt16Array/bsGetUInt16: ");
  {
    unsigned numBits = random()%(sizeof (uint16_t)*CHAR_BIT) + 1;
    uint16_t mask = ~(uint16_t)0;
    if (numBits < sizeof (uint16_t)*CHAR_BIT)
      mask = ~(mask << numBits);
    offset = offsetStart;
    bsStoreUniformUInt16Array(bitStore, offset, numBits, numRnd, randSrc);
    for (i = 0; i < numRnd; ++i)
    {
      uint16_t v = randSrc[i] & mask;
      uint16_t r = bsGetUInt16(bitStore, offset, numBits);
      ensure(had_err, r == v);
      if (had_err)
      {
        env_log_log(env, "Expected %"PRIu16", got %"PRIu16",\n"
                    "seed = %lu, i = %lu, bits=%u\n",
                    v, r, seedval, (unsigned long)i, numBits);
        freeResourcesAndReturn(had_err);
      }
      offset += numBits;
    }
    env_log_log(env, "passed\n");
    env_log_log(env,
                "bsStoreUniformUInt16Array/bsGetUniformUInt16Array: ");
    bsGetUniformUInt16Array(bitStore, offset = offsetStart,
                               numBits, numRnd, randCmp);
    for (i = 0; i < numRnd; ++i)
    {
      uint16_t v = randSrc[i] & mask;
      uint16_t r = randCmp[i];
      ensure(had_err, r == v);
      if (had_err)
      {
        env_log_log(env,
                    "Expected %"PRIu16", got %"PRIu16",\n seed = %lu,"
                    " i = %lu, bits=%u\n",
                    v, r, seedval, (unsigned long)i, numBits);
        freeResourcesAndReturn(had_err);
      }
    }
    if (numRnd > 1)
    {
      uint16_t v = randSrc[0] & mask;
      uint16_t r;
      bsGetUniformUInt16Array(bitStore, offsetStart,
                                 numBits, 1, &r);
      if (r != v)
      {
        env_log_log(env,
                    "Expected %"PRIu16", got %"PRIu16", seed = %lu,"
                    " one value extraction\n",
                    v, r, seedval);
        freeResourcesAndReturn(had_err);
      }
    }
    env_log_log(env, " passed\n");
  }
  /* int types */
  env_log_log(env, "bsStoreInt16/bsGetInt16: ");
  for (i = 0; i < numRnd; ++i)
  {
    int16_t v = (int16_t)randSrc[i];
    unsigned bits = requiredInt16Bits(v);
    bsStoreInt16(bitStore, offset, bits, v);
    offset += bits;
  }
  offset = offsetStart;
  for (i = 0; i < numRnd; ++i)
  {
    int16_t v = randSrc[i];
    unsigned bits = requiredInt16Bits(v);
    int16_t r = bsGetInt16(bitStore, offset, bits);
    ensure(had_err, r == v);
    if (had_err)
    {
      env_log_log(env, "Expected %"PRId16", got %"PRId16",\n"
                  "seed = %lu, i = %lu, bits=%u\n",
                  v, r, seedval, (unsigned long)i, bits);
      freeResourcesAndReturn(had_err);
    }
    offset += bits;
  }
  env_log_log(env, "passed\n");
  env_log_log(env, "bsStoreUniformInt16Array/bsGetInt16: ");
  {
    unsigned numBits = random()%(sizeof (int16_t)*CHAR_BIT) + 1;
    int16_t mask = ~(int16_t)0;
    if (numBits < sizeof (int16_t)*CHAR_BIT)
      mask = ~(mask << numBits);
    offset = offsetStart;
    bsStoreUniformInt16Array(bitStore, offset, numBits, numRnd,
                                (int16_t *)randSrc);
    for (i = 0; i < numRnd; ++i)
    {
      int16_t m = (int16_t)1 << (numBits - 1);
      int16_t v = (int16_t)((randSrc[i] & mask) ^ m) - m;
      int16_t r = bsGetInt16(bitStore, offset, numBits);
      ensure(had_err, r == v);
      if (had_err)
      {
        env_log_log(env, "Expected %"PRId16", got %"PRId16",\n"
                    "seed = %lu, i = %lu, numBits=%u\n",
                    v, r, seedval, (unsigned long)i, numBits);
        freeResourcesAndReturn(had_err);
      }
      offset += numBits;
    }
    env_log_log(env, "passed\n");
    env_log_log(env, "bsStoreUniformInt16Array/bsGetUniformInt16Array: ");
    bsGetUniformInt16Array(bitStore, offset = offsetStart,
                              numBits, numRnd, (int16_t *)randCmp);
    for (i = 0; i < numRnd; ++i)
    {
      int16_t m = (int16_t)1 << (numBits - 1);
      int16_t v = (int16_t)((randSrc[i] & mask) ^ m) - m;
      int16_t r = randCmp[i];
      ensure(had_err, r == v);
      if (had_err)
      {
        env_log_log(env, "Expected %"PRId16", got %"PRId16
                    ", seed = %lu, i = %lu\n",
                    v, r, seedval, (unsigned long)i);
        freeResourcesAndReturn(had_err);
      }
    }
    if(numRnd > 0)
    {
      int16_t m = (int16_t)1 << (numBits - 1);
      int16_t v = (int16_t)((randSrc[0] & mask) ^ m) - m;
      int16_t r;
      bsGetUniformInt16Array(bitStore, offsetStart,
                                numBits, 1, &r);
      ensure(had_err, r == v);
      if (had_err)
      {
        env_log_log(env, "Expected %"PRId16", got %"PRId16
                    ", seed = %lu, one value extraction\n",
                    v, r, seedval);
        freeResourcesAndReturn(had_err);
      }
    }
    env_log_log(env, "passed\n");
  }
  if(numRnd > 0)
  {
    env_log_log(env, "bsCopy: ");
    {
      /* first decide how many of the values to use and at which to start */
      size_t numValueCopies, copyStart;
      BitOffset numCopyBits = 0, destOffset;
      unsigned numBits = random()%(sizeof (uint16_t)*CHAR_BIT) + 1;
      uint16_t mask = ~(uint16_t)0;
      if (numBits < 16)
        mask = ~(mask << numBits);
      if (random()&1)
      {
        numValueCopies = random()%(numRnd + 1);
        copyStart = random()%(numRnd - numValueCopies + 1);
      }
      else
      {
        copyStart = random() % numRnd;
        numValueCopies = random()%(numRnd - copyStart) + 1;
      }
      assert(copyStart + numValueCopies <= numRnd);
      offset = offsetStart + (BitOffset)copyStart * numBits;
      destOffset = random()%
        (offsetStart
         + (sizeof (uint16_t)*CHAR_BIT)
         * (BitOffset)(numRnd - numValueCopies) + 1);
      numCopyBits = (BitOffset)numBits * numValueCopies;
      /* the following bsCopy should be equivalent to:
       * bsStoreUniformUInt16Array(bitStoreCopy, destOffset,
       *                              numBits, numValueCopies, randSrc); */
      bsCopy(bitStore, offset, bitStoreCopy, destOffset, numCopyBits);
      ensure(had_err,
             bsCompare(bitStore, offset, numCopyBits,
                       bitStoreCopy, destOffset, numCopyBits) == 0);
      if (had_err)
      {
        env_log_log(env, "Expected equality on bitstrings\n"
                    "seed = %lu, offset = %llu, destOffset = %llu,"
                    " numCopyBits=%llu\n",
                    seedval, (unsigned long long)offset,
                    (unsigned long long)destOffset, (unsigned long long)numCopyBits);
        /* FIXME: implement bitstring output function */
        freeResourcesAndReturn(had_err);
      }
      env_log_log(env, "passed\n");
    }
  }
  freeResourcesAndReturn(had_err);
}
