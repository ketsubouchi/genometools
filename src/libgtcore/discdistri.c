/*
  Copyright (c) 2006-2007 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2006-2007 Center for Bioinformatics, University of Hamburg

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

#include <assert.h>
#include <stdio.h>
#include "libgtcore/array.h"
#include "libgtcore/discdistri.h"
#include "libgtcore/xansi.h"

struct DiscDistri {
  Array *values;
  unsigned long num_of_occurrences;
};

DiscDistri* discdistri_new(Env *env)
{
  return env_ma_calloc(env, 1, sizeof (DiscDistri));
}

void discdistri_add(DiscDistri *d, unsigned long value, Env *env)
{
  unsigned long *distri, zero = 0;
  assert(d);

  if (!d->values)
    d->values = array_new(sizeof (unsigned long), env);

  while (array_size(d->values) <= value)
    array_add(d->values, zero, env);

  distri = array_get_space(d->values);
  distri[value]++;
  d->num_of_occurrences++;
}

void discdistri_show(const DiscDistri *d)
{
  assert(d);
  discdistri_show_generic(d, NULL);
}

void discdistri_show_generic(const DiscDistri *d, GenFile *genfile)
{
  unsigned long value, occurrences;
  double probability, cumulative_probability = 0.0;
  assert(d);

  for (value = 0; value < array_size(d->values); value++) {
    occurrences = *(unsigned long*) array_get(d->values, value);
    probability = (double) occurrences / d->num_of_occurrences;
    cumulative_probability += probability;
    if (occurrences)
      genfile_xprintf(genfile, "%lu: %lu (prob=%.4f,cumulative=%.4f)\n", value,
                      occurrences, probability, cumulative_probability);
  }
}

void discdistri_delete(DiscDistri *d, Env *env)
{
  if (!d) return;
  array_delete(d->values, env);
  env_ma_free(d, env);
}
