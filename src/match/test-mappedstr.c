/*
  Copyright (c) 2007 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
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
#include <string.h>
#include <stdbool.h>
#include "core/arraydef.h"
#include "core/chardef.h"
#include "core/error.h"
#include "core/unused_api.h"
#include "encseq-def.h"
#include "intcode-def.h"
#include "sfx-nextchar.h"

#include "kmer2string.pr"
#include "sfx-mappedstr.pr"

static Codetype qgram2codefillspecial(unsigned int numofchars,
                                      unsigned int kmersize,
                                      const Encodedsequence *encseq,
                                      Readmode readmode,
                                      Seqpos startpos,
                                      Seqpos totallength)
{
  Codetype integercode;
  Seqpos pos;
  bool foundspecial;
  GtUchar cc;

  if (startpos >= totallength)
  {
    integercode = (Codetype) (numofchars - 1);
    foundspecial = true;
  } else
  {
    cc = getencodedchar(encseq,startpos,readmode); /* for testing */
    if (ISSPECIAL(cc))
    {
      integercode = (Codetype) (numofchars - 1);
      foundspecial = true;
    } else
    {
      integercode = (Codetype) cc;
      foundspecial = false;
    }
  }
  for (pos = startpos + 1; pos < startpos + kmersize; pos++)
  {
    if (foundspecial)
    {
      ADDNEXTCHAR(integercode,numofchars-1,numofchars);
    } else
    {
      if (pos >= totallength)
      {
        ADDNEXTCHAR(integercode,numofchars-1,numofchars);
        foundspecial = true;
      } else
      {
        cc = getencodedchar(encseq,pos,readmode); /* for testing */
        if (ISSPECIAL(cc))
        {
          ADDNEXTCHAR(integercode,numofchars-1,numofchars);
          foundspecial = true;
        } else
        {
          ADDNEXTCHAR(integercode,cc,numofchars);
        }
      }
    }
  }
  return integercode;
}

GT_DECLAREARRAYSTRUCT(Codetype);

static void outkmeroccurrence(void *processinfo,
                              Codetype code,
                              GT_UNUSED Seqpos position,
                              GT_UNUSED const Firstspecialpos
                                           *firstspecialposition)
{
  ArrayCodetype *codelist = (ArrayCodetype *) processinfo;

  GT_STOREINARRAY(codelist,Codetype,1024,code);
}

/*
   The function to collect the code from a stream of fasta files
   can only produce the sequence of code in forward mode.
   Hence we compute the corresponding sequence also in Forwardmode.
   Thus we restrict the call for verifymappedstr to the case where
   the suffix array is in readmode = Forwardmode.
*/

static void collectkmercode(ArrayCodetype *codelist,
                            const Encodedsequence *encseq,
                            unsigned int kmersize,
                            unsigned int numofchars,
                            Seqpos stringtotallength)
{
  Seqpos offset;
  Codetype code;

  for (offset=0; offset<=stringtotallength; offset++)
  {
    code = qgram2codefillspecial(numofchars,
                                 kmersize,
                                 encseq,
                                 Forwardmode,
                                 offset,
                                 stringtotallength);
    GT_STOREINARRAY(codelist,Codetype,1024,code);
  }
}

static int comparecodelists(const ArrayCodetype *codeliststream,
                            const ArrayCodetype *codeliststring,
                            unsigned int kmersize,
                            unsigned int numofchars,
                            const char *characters,
                            GtError *err)
{
  unsigned long i;
  char buffer1[64+1], buffer2[64+1];

  gt_error_check(err);
  if (codeliststream->nextfreeCodetype != codeliststring->nextfreeCodetype)
  {
    gt_error_set(err,"length codeliststream= %lu != %lu =length codeliststring",
                  (unsigned long) codeliststream->nextfreeCodetype,
                  (unsigned long) codeliststring->nextfreeCodetype);
    return -1;
  }
  for (i=0; i<codeliststream->nextfreeCodetype; i++)
  {
    if (codeliststream->spaceCodetype[i] != codeliststring->spaceCodetype[i])
    {
      fromkmercode2string(buffer1,
                          codeliststream->spaceCodetype[i],
                          numofchars,
                          kmersize,
                          characters);
      fromkmercode2string(buffer2,
                          codeliststring->spaceCodetype[i],
                          numofchars,
                          kmersize,
                          characters);
      gt_error_set(err,"codeliststream[%lu] = " FormatCodetype " != "
                    FormatCodetype " = codeliststring[%lu]\n%s != %s",
                    i,
                    codeliststream->spaceCodetype[i],
                    codeliststring->spaceCodetype[i],
                    i,
                    buffer1,
                    buffer2);
      return -1;
    }
  }
  return 0;
}

static int verifycodelists(const Encodedsequence *encseq,
                           unsigned int kmersize,
                           unsigned int numofchars,
                           const ArrayCodetype *codeliststream,
                           GtError *err)
{
  bool haserr = false;
  ArrayCodetype codeliststring;
  const GtUchar *characters;
  Seqpos stringtotallength;

  gt_error_check(err);
  stringtotallength = getencseqtotallength(encseq);
  characters = getencseqAlphabetcharacters(encseq);
  GT_INITARRAY(&codeliststring,Codetype);
  collectkmercode(&codeliststring,
                  encseq,
                  kmersize,
                  numofchars,
                  stringtotallength);
  if (comparecodelists(codeliststream,
                       &codeliststring,
                       kmersize,
                       numofchars,
                       (const char *) characters,
                       err) != 0)
  {
    haserr = true;
  }
  GT_FREEARRAY(&codeliststring,Codetype);
  return haserr ? -1 : 0;
}

int verifymappedstr(const Encodedsequence *encseq,unsigned int prefixlength,
                    GtError *err)
{
  unsigned int numofchars;
  ArrayCodetype codeliststream;
  bool haserr = false;

  gt_error_check(err);
  numofchars = getencseqAlphabetnumofchars(encseq);
  GT_INITARRAY(&codeliststream,Codetype);
  if (getfastastreamkmers(getencseqfilenametab(encseq),
                          outkmeroccurrence,
                          &codeliststream,
                          numofchars,
                          prefixlength,
                          getencseqAlphabetsymbolmap(encseq),
                          false,
                          err) != 0)
  {
    haserr = true;
  }
  if (!haserr)
  {
    if (verifycodelists(encseq,
                        prefixlength,
                        numofchars,
                        &codeliststream,
                        err) != 0)
    {
      haserr = true;
    }
  }
  GT_FREEARRAY(&codeliststream,Codetype);
  return haserr ? -1 : 0;
}
