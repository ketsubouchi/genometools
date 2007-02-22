/*
  Copyright (c) 2003-2007 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2003-2007 Center for Bioinformatics, University of Hamburg
  See LICENSE file or http://genometools.org/license.html for license details.
*/

#include <gt.h>
#include "gtr.h"

/* The GenomeTools (gt) genome analysis system */
int main(int argc, char *argv[])
{
  Env *env;
  GTR *gtr;
  int rval;
  env = env_new();
  gtr = gtr_new(env);
  gtr_register_components(gtr, env);
  switch (gtr_parse(gtr, &rval, argc, argv, env)) {
    case OPTIONPARSER_OK:
      argc -= rval;
      argv += rval;
      rval = gtr_run(gtr, argc, argv, env);
      break;
    case OPTIONPARSER_ERROR:
      rval = EXIT_FAILURE;
      break;
    case OPTIONPARSER_REQUESTS_EXIT:
      rval = EXIT_SUCCESS;
  }
  if (env_error_is_set(env)) {
    fprintf(stderr, "error: %s\n", env_error_get(env));
    assert(rval);
  }
  gtr_delete(gtr, env);
  env_delete(env);
  return rval;
}
