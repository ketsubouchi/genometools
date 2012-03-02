/*
  Copyright (c) 2011-2012 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
  Copyright (c) 2011-2012 Center for Bioinformatics, University of Hamburg

  Permission to use, copy, modify, and distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <limits.h>
#include "core/ma.h"
#include "esa-seqread.h"
/* no include for seqnumrelpos.h */

static void initBUinfo_errfind(GtBUinfo_errfind *,GtBUstate_errfind *);

static void freeBUinfo_errfind(GtBUinfo_errfind *,GtBUstate_errfind *);

static int processleafedge_errfind(bool,
    unsigned long,
    GtBUinfo_errfind *,
    unsigned long, /* position */
    GtBUstate_errfind *,
    GtError *err);

static int processbranchingedge_errfind(bool firstsucc,
    unsigned long,
    GtBUinfo_errfind *,
    unsigned long,
    unsigned long,
    GtBUinfo_errfind *,
    GtBUstate_errfind *,
    GtError *);

static int processlcpinterval_errfind(unsigned long,
    GtBUinfo_errfind *,
    GtBUstate_errfind *,
    GtError *err);

#define TOP_ESA_BOTTOMUP_errfind\
        stack->spaceGtBUItvinfo[stack->nextfreeGtBUItvinfo-1]

#define POP_ESA_BOTTOMUP_errfind\
        (stack->spaceGtBUItvinfo + (--stack->nextfreeGtBUItvinfo))

#define PUSH_ESA_BOTTOMUP_errfind(LCP,LB)\
        if (stack->nextfreeGtBUItvinfo >= stack->allocatedGtBUItvinfo)\
        {\
          gt_assert(stack->nextfreeGtBUItvinfo ==\
                    stack->allocatedGtBUItvinfo);\
          stack->spaceGtBUItvinfo\
            = allocateBUstack_errfind(stack->spaceGtBUItvinfo,\
                              stack->allocatedGtBUItvinfo,\
                              stack->allocatedGtBUItvinfo+incrementstacksize,\
                              bustate);\
          stack->allocatedGtBUItvinfo += incrementstacksize;\
        }\
        gt_assert(stack->spaceGtBUItvinfo != NULL);\
        stack->spaceGtBUItvinfo[stack->nextfreeGtBUItvinfo].lcp = LCP;\
        stack->spaceGtBUItvinfo[stack->nextfreeGtBUItvinfo].lb = LB;\
        stack->spaceGtBUItvinfo[stack->nextfreeGtBUItvinfo++].rb = ULONG_MAX

typedef struct
{
  unsigned long lcp, lb, rb;
  GtBUinfo_errfind info;
} GtBUItvinfo_errfind;

typedef struct
{
  GtBUItvinfo_errfind *spaceGtBUItvinfo;
  unsigned long allocatedGtBUItvinfo,
                nextfreeGtBUItvinfo;
} GtArrayGtBUItvinfo_errfind;

GtArrayGtBUItvinfo_errfind *gt_GtArrayGtBUItvinfo_new_errfind(void)
{
  GtArrayGtBUItvinfo_errfind *stack = gt_malloc(sizeof (*stack));

  stack->spaceGtBUItvinfo = NULL;
  stack->allocatedGtBUItvinfo = stack->nextfreeGtBUItvinfo = 0;
  return stack;
}

void gt_GtArrayGtBUItvinfo_delete_errfind(GtArrayGtBUItvinfo_errfind *stack,
                                  GtBUstate_errfind *state)
{
  unsigned long idx;

  for (idx=0; idx<stack->allocatedGtBUItvinfo; idx++)
  {
    freeBUinfo_errfind(&stack->spaceGtBUItvinfo[idx].info,state);
  }
  gt_free(stack->spaceGtBUItvinfo);
  gt_free(stack);
}

static GtBUItvinfo_errfind *allocateBUstack_errfind(GtBUItvinfo_errfind *ptr,
                                   unsigned long currentallocated,
                                   unsigned long allocated,
                                   GtBUstate_errfind *state)
{
  unsigned long idx;
  GtBUItvinfo_errfind *itvinfo;

  itvinfo = gt_realloc(ptr,sizeof (*itvinfo) * allocated);
  gt_assert(allocated > currentallocated);
  for (idx=currentallocated; idx<allocated; idx++)
  {
    initBUinfo_errfind(&itvinfo[idx].info,state);
  }
  gt_assert(itvinfo != NULL);
  return itvinfo;
}

int gt_esa_bottomup_errfind(Sequentialsuffixarrayreader *ssar,
                    GtBUstate_errfind *bustate,
                    /* no parameter snrp */
                    GtError *err)
{
  const unsigned long incrementstacksize = 32UL;
  unsigned long lcpvalue,
                previoussuffix = 0,
                idx,
                numberofsuffixes,
                lastsuftabvalue = 0;
  GtBUItvinfo_errfind *lastinterval = NULL;
  bool haserr = false, firstedge, firstedgefromroot = true;
  GtArrayGtBUItvinfo_errfind *stack;

  stack = gt_GtArrayGtBUItvinfo_new_errfind();
  PUSH_ESA_BOTTOMUP_errfind(0,0);
  numberofsuffixes = gt_Sequentialsuffixarrayreader_nonspecials(ssar);
  for (idx = 0; !haserr && idx < numberofsuffixes; idx++)
  {
    NEXTSEQUENTIALLCPTABVALUEWITHLAST(lcpvalue,lastsuftabvalue,ssar);
    NEXTSEQUENTIALSUFTABVALUE(previoussuffix,ssar);
    gt_assert(stack->nextfreeGtBUItvinfo > 0);
    if (lcpvalue <= TOP_ESA_BOTTOMUP_errfind.lcp)
    {
      if (TOP_ESA_BOTTOMUP_errfind.lcp > 0 || !firstedgefromroot)
      {
        firstedge = false;
      } else
      {
        firstedge = true;
        firstedgefromroot = false;
      }
      if (processleafedge_errfind(firstedge,
                          TOP_ESA_BOTTOMUP_errfind.lcp,
                          &TOP_ESA_BOTTOMUP_errfind.info,
                          previoussuffix,
                          bustate,
                          err) != 0)
      {
        haserr = true;
      }
    }
    gt_assert(lastinterval == NULL);
    while (!haserr && lcpvalue < TOP_ESA_BOTTOMUP_errfind.lcp)
    {
      lastinterval = POP_ESA_BOTTOMUP_errfind;
      lastinterval->rb = idx;
      if (processlcpinterval_errfind(lastinterval->lcp,
                             &lastinterval->info,
                             bustate,
                             err) != 0)
      {
        haserr = true;
      }
      if (lcpvalue <= TOP_ESA_BOTTOMUP_errfind.lcp)
      {
        if (TOP_ESA_BOTTOMUP_errfind.lcp > 0 || !firstedgefromroot)
        {
          firstedge = false;
        } else
        {
          firstedge = true;
          firstedgefromroot = false;
        }
        if (processbranchingedge_errfind(firstedge,
                   TOP_ESA_BOTTOMUP_errfind.lcp,
                   &TOP_ESA_BOTTOMUP_errfind.info,
                   lastinterval->lcp,
                   lastinterval->rb - lastinterval->lb + 1,
                   &lastinterval->info,
                   bustate,
                   err) != 0)
        {
          haserr = true;
        }
        lastinterval = NULL;
      }
    }
    if (!haserr && lcpvalue > TOP_ESA_BOTTOMUP_errfind.lcp)
    {
      if (lastinterval != NULL)
      {
        unsigned long lastintervallb = lastinterval->lb;
        unsigned long lastintervallcp = lastinterval->lcp,
              lastintervalrb = lastinterval->rb;
        PUSH_ESA_BOTTOMUP_errfind(lcpvalue,lastintervallb);
        if (processbranchingedge_errfind(true,
                       TOP_ESA_BOTTOMUP_errfind.lcp,
                       &TOP_ESA_BOTTOMUP_errfind.info,
                       lastintervallcp,
                       lastintervalrb - lastintervallb + 1,
                       NULL,
                       bustate,
                       err) != 0)
        {
          haserr = true;
        }
        lastinterval = NULL;
      } else
      {
        PUSH_ESA_BOTTOMUP_errfind(lcpvalue,idx);
        if (processleafedge_errfind(true,
                            TOP_ESA_BOTTOMUP_errfind.lcp,
                            &TOP_ESA_BOTTOMUP_errfind.info,
                            previoussuffix,
                            bustate,
                            err) != 0)
        {
          haserr = true;
        }
      }
    }
  }
  gt_assert(stack->nextfreeGtBUItvinfo > 0);
  if (!haserr && TOP_ESA_BOTTOMUP_errfind.lcp > 0)
  {
    /* no assignment to lastsuftabvalue */
    if (processleafedge_errfind(false,
                        TOP_ESA_BOTTOMUP_errfind.lcp,
                        &TOP_ESA_BOTTOMUP_errfind.info,
                        lastsuftabvalue,
                        bustate,
                        err) != 0)
    {
      haserr = true;
    } else
    {
      TOP_ESA_BOTTOMUP_errfind.rb = idx;
      if (processlcpinterval_errfind(TOP_ESA_BOTTOMUP_errfind.lcp,
                             &TOP_ESA_BOTTOMUP_errfind.info,
                             bustate,
                             err) != 0)
      {
        haserr = true;
      }
    }
  }
  gt_GtArrayGtBUItvinfo_delete_errfind(stack,bustate);
  return haserr ? -1 : 0;
}