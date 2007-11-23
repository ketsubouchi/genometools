/*
  Copyright (c) 2007 David Ellinghaus <dellinghaus@zbh.uni-hamburg.de>
  Copyright (c) 2007 Center for Bioinformatics, University of Hamburg

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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "libgtcore/env.h"
#include "libgtcore/str.h"
#include "libgtmatch/sarr-def.h"
#include "libgtmatch/spacedef.h"
#include "libgtmatch/esa-seqread.h"
#include "libgtmatch/intcode-def.h"

#include "ltrharvest-opt.h"
#include "repeats.h"
#include "searchforLTRs.h"
#include "duplicates.h"
#include "outputstd.h"
#include "outputfasta.h"
#include "outputgff3.h"

#include "libgtmatch/esa-maxpairs.pr"
#include "libgtmatch/pos2seqnum.pr"

static int runltrharvest(LTRharvestoptions *lo, Env *env)
{
  Sequentialsuffixarrayreader *ssar; /* suffix array */
  Seqpos *markpos = NULL;
  unsigned long numofdbsequences;
  bool had_err = false;

  env_error_check(env);

  ssar = newSequentialsuffixarrayreaderfromfile(lo->str_indexname,
                                  SARR_LCPTAB | SARR_SUFTAB |
                                  SARR_ESQTAB | SARR_DESTAB,
                                  SEQ_mappedboth,
                                  env);
  if (ssar == NULL)
  {
    return -1;
  }

  /* test if motif is valid and encode motif */
  if (testmotifandencodemotif (&lo->motif,
                             alphabetSequentialsuffixarrayreader(ssar),
                             env) != 0)
  {
    had_err = true;
  }

  /* show defined option and values */
  if (!had_err && lo->verbosemode)
  {
    showuserdefinedoptionsandvalues(lo);
  }

  numofdbsequences = numofdbsequencesSequentialsuffixarrayreader(ssar);
  /* calculate markpos array for sequences offset */
  if (!had_err && numofdbsequences > 1UL)
  {
    markpos = encseq2markpositions(
        encseqSequentialsuffixarrayreader(ssar),
        numofdbsequencesSequentialsuffixarrayreader(ssar),
        env);
    lo->markpos = markpos;
    if (markpos == NULL)
    {
      had_err = true;
    }
  }

  /* init array for maximal repeats */
  INITARRAY (&lo->repeatinfo.repeats, Repeat);
  lo->repeatinfo.ssarptr = ssar;

  /* search for maximal repeats */
  if (!had_err && enumeratemaxpairs(ssar,
                       getnumofcharsAlphabet(
                         alphabetSequentialsuffixarrayreader(ssar)),
                       encseqSequentialsuffixarrayreader(ssar),
                       readmodeSequentialsuffixarrayreader(ssar),
                       (unsigned int)lo->minseedlength,
                       (void*)simpleexactselfmatchstore,
                       lo,
                       NULL,
                       env) != 0)
  {
    had_err = true;
  }

  /* init array for candidate pairs */
  INITARRAY(&lo->arrayLTRboundaries, LTRboundaries);

  /* apply the filter algorithms */
  if (!had_err && searchforLTRs (ssar, lo, markpos, env) != 0)
  {
    had_err = true;
  }

  /* free array for maximal repeats */
  FREEARRAY(&lo->repeatinfo.repeats, Repeat);

  /* remove exact duplicates */
  if (!had_err)
  {
    removeduplicates(&lo->arrayLTRboundaries);
  }

  /* remove overlapping predictions if desired */
  if (!had_err && (lo->nooverlapallowed || lo->bestofoverlap))
  {
    removeoverlapswithlowersimilarity(&lo->arrayLTRboundaries,
                                      lo->nooverlapallowed);
  }

  /* print multiple FASTA file of predictions */
  if (!had_err && lo->fastaoutput)
  {
    if (showpredictionsmultiplefasta(lo,
          markpos,
          false,
          60U,
          ssar,
          true,
          env) != 0)
    {
      had_err = true;
    }
  }

  /* print inner region multiple FASTA file of predictions */
  if (!had_err && lo->fastaoutputinnerregion)
  {
    if (showpredictionsmultiplefasta(lo,
          markpos,
          true,
          60U,
          ssar,
          true,
          env) != 0)
    {
      had_err = true;
    }
  }

  /* print GFF3 format file of predictions */
  if (!had_err && lo->gff3output)
  {
    printgff3format(lo, ssar, markpos, env);
  }

  if (!had_err && numofdbsequences > 1UL)
  {
    FREESPACE(markpos);
  }

  /* print predictions to stdout */
  if (!had_err)
  {
    if (showinfoiffoundfullLTRs(lo, ssar, env) != 0)
    {
      had_err = true;
    }
  }

  /* free prediction array */
  FREEARRAY(&lo->arrayLTRboundaries, LTRboundaries);
  /* free suffixarray */
  freeSequentialsuffixarrayreader(&ssar, env);

  return had_err ? -1 : 0;
}

int parseargsandcallltrharvest(int argc,const char *argv[],Env *env)
{
  LTRharvestoptions lo;
  bool haserr = false;

  if(ltrharvestoptions(&lo,argc,argv,env) != 0)
  {
    haserr = true;
  } else
  {
    printargsline(argv,argc);
    haserr = runltrharvest(&lo,env);
  }
  wrapltrharvestoptions(&lo,env);
  return haserr ? -1 : 0;
}
