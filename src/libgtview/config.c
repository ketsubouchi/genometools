/*
  Copyright (c) 2007 Sascha Steinbiss <ssteinbiss@stud.zbh.uni-hamburg.de>
  Copyright (c) 2005-2007 Center for Bioinformatics, University of Hamburg
  See LICENSE file or http://genometools.org/license.html for license details.
*/

#include <libgtview/config.h>
#include <assert.h>
#include <string.h>
#include <libgtcore/warning.h>
#include <libgtcore/ensure.h>
#include <libgtview/config.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

struct Config
{
  lua_State *L;
  Str *filename;
  bool verbose;
};

static void config_lua_new_table(lua_State *L, const char *key)
{
  lua_pushstring(L, key);
  lua_newtable(L);
  lua_settable(L, -3);
}

Config* config_new(Env *env, bool verbose)
{
  Config *cfg;
  env_error_check(env);
  cfg = env_ma_malloc(env, sizeof (Config));
  cfg->filename = NULL;
  cfg->verbose = verbose;
  cfg->L = luaL_newstate();
  if (!cfg->L)
  {
    env_error_set(env, "out of memory (cannot create new lua state)");
  } else luaL_openlibs(cfg->L);
  return cfg;
}

void config_delete(Config *cfg, Env *env)
{
  assert(cfg);
  if (cfg->L) lua_close(cfg->L);
  if (cfg->filename) str_delete(cfg->filename,env);
  env_ma_free(cfg, env);
}

void config_load_file(Config *cfg, Str *fn, Env* env)
{
  int had_err = 0;
  env_error_check(env);
  assert(cfg && cfg->L && fn);
  cfg->filename = str_ref(fn);
  if (config_get_verbose(cfg))
    fprintf(stderr, "Trying to load config file: %s...\n", str_get(fn));
  if (luaL_loadfile(cfg->L, str_get(fn)) ||
      lua_pcall(cfg->L, 0, 0, 0))
  {
    env_error_set(env, "cannot run configuration file: %s",
                  lua_tostring(cfg->L, -1));
    had_err = -1;
  }
  if (!had_err)
  {
    lua_getglobal(cfg->L, "config");
    if (lua_isnil(cfg->L, -1) || !lua_istable(cfg->L, -1))
    {
      env_error_set(env, "'config' is not defined or not a table in \"%s\"",
                    str_get(fn));
    }
    lua_pop(cfg->L, 1);
  }
}

/*!
Searches for a section inside the config table,
creating it if it does not exist and finally pushing
it on the Lua stack (at the top).
\param cfg Config object to search in.
\param section Name of the section to search for.
\param env Pointer to Environment object.
\return Total number of items pushed on the stack by this function.
*/
static int config_find_section_for_setting(Config* cfg,
                                           const char* section,
																					 Env* env)
{
  int depth = 0;
  assert(cfg && section);
  env_error_check(env);
  lua_getglobal(cfg->L, "config");
  if (lua_isnil(cfg->L, -1))
  {
	  lua_pop(cfg->L, 1);
    lua_newtable(cfg->L);
    lua_setglobal(cfg->L, "config");
    lua_getglobal(cfg->L, "config");
  }
  depth++;
  lua_getfield(cfg->L, -1, section);
  if (lua_isnil(cfg->L, -1))
  {
    lua_pop(cfg->L, 1);
    config_lua_new_table(cfg->L, section);
    lua_getfield(cfg->L, -1, section);
  }
  depth++;
  return depth;
}

/*!
Searches for a section inside the config table,
returning if it is not found.
\param cfg Config object to search in.
\param section Name of the section to search for.
\param env Pointer to Environment object.
\return -1 if not found, otherwise number of items
        pushed on the stack by this function.
*/
static int config_find_section_for_getting(Config* cfg,
                                           const char* section,
																					 Env* env)
{
  int depth = 0;
  assert(cfg && section);
  lua_getglobal(cfg->L, "config");
  if (lua_isnil(cfg->L, -1))
  {
    if (cfg->verbose) warning("'config' is not defined");
    lua_pop(cfg->L, 1);
    return -1;
  } else depth++;
  lua_getfield(cfg->L, -1, section);
  if (lua_isnil(cfg->L, -1) || !lua_istable(cfg->L, -1))
  {
    if (cfg->verbose) warning("section '%s' is not defined", section);
    lua_pop(cfg->L, 1);
    return -1;
  } else depth++;
  return depth;
}

void config_set_cstr(Config *cfg,
                     const char* section,
                     const char *key,
                     const char* str,
                     Env* env)
{
  int i = 0;
  assert(cfg && section && key);
  i = config_find_section_for_setting(cfg, section, env);
  lua_pushstring(cfg->L, key);
  lua_pushstring(cfg->L, str);
  lua_settable(cfg->L, -3);
  lua_pop(cfg->L, i);
}

void config_set_color(Config *cfg, const char *key, Color color, Env* env)
{
  int i = 0;
  assert(cfg && key);
  i = config_find_section_for_setting(cfg, "colors", env);
  lua_getfield(cfg->L, -1, key);
  i++;
  if (lua_isnil(cfg->L, -1))
  {
    lua_pop(cfg->L, 1);
    config_lua_new_table(cfg->L, key);
    lua_getfield(cfg->L, -1, key);
  }
  lua_pushstring(cfg->L, "red");
  lua_pushnumber(cfg->L, color.red);
  lua_settable(cfg->L, -3);
  lua_pushstring(cfg->L, "green");
  lua_pushnumber(cfg->L, color.green);
  lua_settable(cfg->L, -3);
  lua_pushstring(cfg->L, "blue");
  lua_pushnumber(cfg->L, color.blue);
  lua_settable(cfg->L, -3);
  lua_pop(cfg->L, i);
}

void config_set_num(Config *cfg,
                    const char* section,
                    const char *key,
                    double num,
                    Env* env)
{
  int i = 0;
  assert(cfg && section && key);
  i = config_find_section_for_setting(cfg, section, env);
  lua_pushstring(cfg->L, key);
  lua_pushnumber(cfg->L, num);
  lua_settable(cfg->L, -3);
  lua_pop(cfg->L, i);
}

double config_get_num(Config *cfg,
                      const char *section,
                      const char *key,
                      double deflt,
                      Env* env)
{
  assert(cfg && key && section);
  double num = deflt;
  int i = 0;
  env_error_check(env);
  /* get section */
  i = config_find_section_for_getting(cfg, section, env);
  /* could not get section, return default */
  if (i < 0) {
    lua_pop(cfg->L, i);
    return deflt;
  }
  /* lookup entry for given key */
  lua_getfield(cfg->L, -1, key);
  if (lua_isnil(cfg->L, -1) || !lua_isnumber(cfg->L, -1))
  {
    if (cfg->verbose) warning("no or non-numeric value found for key '%s'",
                              key);
    lua_pop(cfg->L, i+1);
    return deflt;
  } else i++;
  /* retrieve value */
  num = lua_tonumber(cfg->L, -1);
  /* reset stack to original state for subsequent calls */
  lua_pop(cfg->L, i);
  return num;
}

const char* config_get_cstr(Config *cfg,
                            const char *section,
                            const char *key,
                            const char *deflt,
                            Env* env)
{
  assert(cfg && key && section);
  const char* str = deflt;
  int i = 0;
  env_error_check(env);
  /* get section */
  i = config_find_section_for_getting(cfg, section, env);
  /* could not get section, return default */
  if (i < 0) {
    lua_pop(cfg->L, i);
    return deflt;
  }
  /* lookup entry for given key */
  lua_getfield(cfg->L, -1, key);
  if (lua_isnil(cfg->L, -1) || !lua_isstring(cfg->L, -1))
  {
    if (cfg->verbose) warning("no value is defined for key '%s'",
                               key);
    lua_pop(cfg->L, i+1);
    return deflt;
  } else i++;
  /* retrieve string */
  str = lua_tostring(cfg->L, -1);
  /* reset stack to original state for subsequent calls */
  lua_pop(cfg->L, i);
  return str;
}

Color config_get_color(Config *cfg, const char *key, Env* env)
{
  assert(cfg && key);
  Color color;
  int i = 0;
  env_error_check(env);
  /* set default colors */
  color.red=0.8; color.green = 0.8; color.blue=0.8;
  /* get section */
  i = config_find_section_for_getting(cfg, "colors", env);
  /* could not get section, return default */
  if (i < 0)
  {
    lua_pop(cfg->L, i);
    return color;
  }
  /* lookup color entry for given feature */
  lua_getfield(cfg->L, -1, key);
  if (lua_isnil(cfg->L, -1) || !lua_istable(cfg->L, -1))
  {
    if (cfg->verbose) warning("no colors are defined for type '%s', "
                               "will use defaults.",
                               key);
    lua_pop(cfg->L, 3);
    return color;
  } else i++;
  /* update color struct */
  lua_getfield(cfg->L, -1, "red");
  if (lua_isnil(cfg->L, -1) || !lua_isnumber(cfg->L, -1))
  {
    if (cfg->verbose) warning("%s  value for type '%s' is undefined or"
                               " not numeric, using default","red", key);
  }
  else
  {
    color.red = lua_tonumber(cfg->L,-1);
  }
  lua_pop(cfg->L, 1);
  lua_getfield(cfg->L, -1, "green");
  if (lua_isnil(cfg->L, -1) || !lua_isnumber(cfg->L, -1))
  {
    if (cfg->verbose) warning("%s  value for type '%s' is undefined or"
                               " not numeric, using default","green", key);
  }
  else
  {
    color.green = lua_tonumber(cfg->L,-1);
  }
  lua_pop(cfg->L, 1);
  lua_getfield(cfg->L, -1, "blue");
  if (lua_isnil(cfg->L, -1) || !lua_isnumber(cfg->L, -1))
  {
    if (cfg->verbose) warning("%s  value for type '%s' is undefined or"
                               " not numeric, using default","blue", key);
  }
  else
  {
    color.blue = lua_tonumber(cfg->L,-1);
  }
  lua_pop(cfg->L, 1);
  /* reset stack to original state for subsequent calls */
  lua_pop(cfg->L, i);
  return color;
}

void config_reload(Config *cfg, Env *env)
{
  assert(cfg && cfg->filename);
  config_load_file(cfg, cfg->filename, env);
}

bool config_cstr_in_list(Config *cfg,
                    const char* section,
                    const char* key,
                    const char* checkstr,
                    Env* env)
{
  assert(cfg && key && section && checkstr);
  int i = 0, had_err = 0;
  bool ret = false;
  env_error_check(env);
  /* get section */
  i = config_find_section_for_getting(cfg, section, env);
  if (i < 0)
  {
    lua_pop(cfg->L, i);
    return false;
  }
  lua_getfield(cfg->L, -1, key);
  i++;
  if (lua_isnil(cfg->L, -1) || !lua_istable(cfg->L, -1))
  {
/*    if (cfg->verbose) warning("key '%s' is not set or not a table",
                               key);
    lua_pop(cfg->L, 1);*/
    had_err = -1;
  }
  if (!had_err)
  {
    /* table is at the top position in the stack */
    lua_pushnil(cfg->L);  /* first key */
    while (lua_next(cfg->L, -2) != 0) {
      /* uses 'key' (at index -2) and 'value' (at index -1) */
      if (lua_isnil(cfg->L, -1) || !lua_isstring(cfg->L, -1))
      {
        if (cfg->verbose) warning("non-string value in section %s, key %s",
                                   section, key);
        lua_pop(cfg->L, 1);
      } else {
        if (strcmp(checkstr, lua_tostring(cfg->L, -1)) == 0)
        {
          /* found checkstr, report success */
          ret = true;
        }
        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop(cfg->L, 1);
      }
    }
  }
  /* remove temporary stack items */
  lua_pop(cfg->L, i);
  return ret;
}

bool config_get_verbose(Config *cfg)
{
  return cfg->verbose;
}

int config_dominates(Config* cfg, GenomeFeatureType ft1,
                      GenomeFeatureType ft2, Env* env)
{
  assert(cfg && env);
  char *fts1, *fts2;

  if (ft1 == ft2)
    return DOMINATES_EQUAL;
  fts1 = (char*) genome_feature_type_get_cstr(ft1);
  fts2 = (char*) genome_feature_type_get_cstr(ft2);
  if (fts1 == NULL || fts2 == NULL)
    return DOMINATES_UNKNOWN_TYPE;
  else
  {
    if (config_cstr_in_list(cfg, "dominate",fts1, fts2, env))
      return DOMINATES_FIRST;
    else if (config_cstr_in_list(cfg, "dominate",fts2, fts1, env))
      return DOMINATES_SECOND;
    else
      return DOMINATES_NOT_SPECIFIED;
  }
}

/*!
Unit tests for the Config class.
\param env Pointer to Environment object.
\return Error status.
*/
int config_unit_test(Env* env)
{
  int had_err = 0;
  Config *cfg;
  const char* test1 = "mRNA";
  const char* str = NULL;
  Str *luafile = str_new_cstr("config.lua",env);
  Color col1, col2, col, defcol, tmpcol;
  double num;

  /* example colors */
  col1.red=.1;col1.green=.2;col1.blue=.3;
  col2.red=.4;col2.green=.5;col2.blue=.6;
  col.red=1;col.green=1;col.blue=1;
  defcol.red=.8;defcol.green=.8;defcol.blue=.8;

  /* instantiate new config object */
  cfg = config_new(env, false);

  /* at the beginning, all values are defaults, since nothing is defined */
  tmpcol = config_get_color(cfg, "exon", env);
  ensure(had_err, color_equals(tmpcol,defcol));
  tmpcol = config_get_color(cfg, "cds", env);
  ensure(had_err, color_equals(tmpcol,defcol));
  tmpcol = config_get_color(cfg, "foo", env);
  ensure(had_err, color_equals(tmpcol,defcol));
  num = config_get_num(cfg,"format", "margins", 10, env);
  ensure(had_err, num == 10);
  str = config_get_cstr(cfg, "collapse", "exon", "", env);
  ensure(had_err, (str == ""));

  /* change some values... */
  config_set_color(cfg, "exon", col, env);
  config_set_num(cfg,"format", "margins", 11, env);
  config_set_num(cfg,"format", "foo", 2, env);

  /* is it saved correctly? */
  tmpcol = config_get_color(cfg, "exon", env);
  ensure(had_err, !color_equals(tmpcol,defcol));
  tmpcol = config_get_color(cfg, "exon", env);
  ensure(had_err, color_equals(tmpcol,col));
  num = config_get_num(cfg,"format", "margins", 10,  env);
  ensure(had_err, num == 11);
  num = config_get_num(cfg,"format", "foo", 10, env);
  ensure(had_err, num == 2);

  /* create a new color definition */
  config_set_color(cfg, "foo", col, env);
  config_set_cstr(cfg, "bar", "baz", test1, env);

  /* is it saved correctly? */
  tmpcol = config_get_color(cfg, "foo", env);
  ensure(had_err, !color_equals(tmpcol,defcol));
  tmpcol = config_get_color(cfg, "foo", env);
  ensure(had_err, color_equals(tmpcol,col));
  str = config_get_cstr(cfg, "bar", "baz", "", env);
  ensure(had_err, (str != ""));
  ensure(had_err, (strcmp(str,test1)==0));
  str = config_get_cstr(cfg, "bar", "test", "", env);
  ensure(had_err, (str == ""));

  /* mem cleanup */
  str_delete(luafile, env);
  config_delete(cfg, env);

  return had_err;
}
