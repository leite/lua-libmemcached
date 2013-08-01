/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <xxleite@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h> //
//#include <unistd.h> //
#include <libmemcached/memcached.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <lua-libmemcached.h>

// meta methods
const struct luaL_reg methods[] = {
  {"add_server",      memc_add_server},
  {"set_behavior",    memc_set_behavior},
  {"get_behavior",    memc_get_behavior},
  {"add",             memc_add},
  {"replace",         memc_replace},
  {"cas",             memc_cas},
  {"append",          memc_append},
  {"prepend",         memc_prepend},
  {"append_multi",    memc_append_multi},
  {"prepend_multi",   memc_prepend_multi},
  {"check_key",       memc_check_key},
  {"set",             memc_set},
  {"set_multi",       memc_set_multi},
  {"delete",          memc_delete},
  {"delete_multi",    memc_delete_multi},
  {"get",             memc_get},
  {"get_multi",       memc_get_multi},
  {"incr",            memc_incr},
  {"decr",            memc_decr},
  {NULL,              NULL}
};

const struct luaL_reg metamethods[] = {
  {"__gc", memc_free},
  {NULL,   NULL}
};

const struct luaL_reg functions[] = {
  {"new", memc_new},
  {NULL,  NULL}
};

enum storage_type{ ADD, SET, REPLACE, CAS, APPEND, PREPEND } s_type;
enum operation_type{ INCREMENT, DECREMENT } o_type;

static void add_servers(lua_State *L, memcached_st *memcache, memcached_return *status) {
  memcached_server_st *servers = NULL;
  const char          *host    = NULL;
  in_port_t           port     = 0;
  int                 x        = 0;

  switch(lua_type(L, 1)) {
    case LUA_TSTRING:
      host = lua_tostring(L, 1);
      if(lua_gettop(L)>1 && lua_isnumber(L, 2)) {
        port    = lua_tointeger(L, 2);
        *status = memcached_server_add(memcache, host, port);
      } else {
        servers = memcached_servers_parse(host);
        *status = memcached_server_push(memcache, servers);
      }
      break;
    case LUA_TTABLE:
      // 
      //status = NULL;
      lua_pushnil(L);
      while(lua_next(L, 1) != 0) {
        
        // 
        if(x>0 && *status!=MEMCACHED_SUCCESS) {
          lua_pop(L, 1);
          continue;
        }
        
        ++x;

        // 
        if(lua_istable(L, -1)) {
          lua_pushnil(L);
          while(lua_next(L, -2) != 0) {
            lua_pushvalue(L, -2);
            
            if(lua_type(L, -2)==LUA_TSTRING)
              host = lua_tostring(L, -2);
            if(lua_isnumber(L, -2))
              port = lua_tointeger(L, -2);

            lua_pop(L, 2);
          }

          if(host!=NULL && port!=0)
            *status = memcached_server_add(memcache, host, port);

          host = NULL;
          port = 0;
        }

        // 
        if(lua_type(L, -1)==LUA_TSTRING) {
          host    = lua_tostring(L, -1);        
          servers = memcached_servers_parse(host);
          *status = memcached_server_push(memcache, servers);
        }

        lua_pop(L, 1);
      }
      break;
    default:
      break;  
  }
  memcached_server_list_free(servers);
}

static void s2_behavior(lua_State *L, const char *const_name, memcached_st *memcache) {
  int behavior = 0;
  uint64_t value;

  if(STRCMP(const_name, "use_binary"))  behavior = MEMCACHED_BEHAVIOR_BINARY_PROTOCOL;
  if(STRCMP(const_name, "use_udp"))     behavior = MEMCACHED_BEHAVIOR_USE_UDP;
  if(STRCMP(const_name, "no_block"))    behavior = MEMCACHED_BEHAVIOR_NO_BLOCK;
  if(STRCMP(const_name, "keepalive"))   behavior = MEMCACHED_BEHAVIOR_KEEPALIVE;
  if(STRCMP(const_name, "enable_cas"))  behavior = MEMCACHED_BEHAVIOR_SUPPORT_CAS;
  if(STRCMP(const_name, "tcp_nodelay")) behavior = MEMCACHED_BEHAVIOR_TCP_NODELAY;
  if(STRCMP(const_name, "no_reply"))    behavior = MEMCACHED_BEHAVIOR_NOREPLY;

  //printf("%s ->> %d -->> %d\n", const_name, behavior, MEMCACHED_BEHAVIOR_USE_UDP);

  if(behavior != 0) {
    value = memcached_behavior_get(memcache, behavior);
    //printf("%s\n", const_name);
    printf("%s -> %d\n", const_name, value);
    lua_pushboolean(L, value);
    return;
  }

  if(STRCMP(const_name, "send_timeout"))    behavior = MEMCACHED_BEHAVIOR_SND_TIMEOUT;
  if(STRCMP(const_name, "receive_timeout")) behavior = MEMCACHED_BEHAVIOR_RCV_TIMEOUT;
  if(STRCMP(const_name, "connect_timeout")) behavior = MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT;
  if(STRCMP(const_name, "poll_timeout"))    behavior = MEMCACHED_BEHAVIOR_POLL_TIMEOUT;
  if(STRCMP(const_name, "keepalive_idle"))  behavior = MEMCACHED_BEHAVIOR_KEEPALIVE_IDLE;
  if(STRCMP(const_name, "retry_timeout"))   behavior = MEMCACHED_BEHAVIOR_RETRY_TIMEOUT;
  
  if(behavior != 0) {
    value = memcached_behavior_get(memcache, behavior);
    lua_pushnumber(L, value);
    return;
  }

  if(const_name=="hash")        behavior = MEMCACHED_BEHAVIOR_HASH;
  if(const_name=="ketama_hash") behavior = MEMCACHED_BEHAVIOR_KETAMA_HASH;

  if(behavior != 0) {
    value = memcached_behavior_get(memcache, behavior);
    switch(value) {
      case MEMCACHED_HASH_MD5:      lua_pushstring(L, "md5");
      case MEMCACHED_HASH_CRC:      lua_pushstring(L, "crc");
      case MEMCACHED_HASH_FNV1_64:  lua_pushstring(L, "fnv1_64");
      case MEMCACHED_HASH_FNV1A_64: lua_pushstring(L, "fnv1a_64");
      case MEMCACHED_HASH_FNV1_32:  lua_pushstring(L, "fnv1_32");
      case MEMCACHED_HASH_FNV1A_32: lua_pushstring(L, "fnv1a_32");
      case MEMCACHED_HASH_JENKINS:  lua_pushstring(L, "jenkins");
      case MEMCACHED_HASH_HSIEH:    lua_pushstring(L, "hsieh");
      case MEMCACHED_HASH_MURMUR:   lua_pushstring(L, "murmur");
      case MEMCACHED_HASH_DEFAULT:  
      default:
        lua_pushstring(L, "default");
    }
    return;
  }
  
  
  // also MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED or MEMCACHED_BEHAVIOR_KETAMA_COMPAT
  if(const_name=="distribution") behavior = MEMCACHED_BEHAVIOR_DISTRIBUTION;
  
  if(behavior != 0) {
    value = memcached_behavior_get(memcache, behavior);
    if(MEMCACHED_DISTRIBUTION_MODULA==value) {            lua_pushstring(L, "modula");
    } else if(MEMCACHED_DISTRIBUTION_CONSISTENT==value) { lua_pushstring(L, "consistent");
    } else {
      if(MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED != 0 && (value = memcached_behavior_get(memcache, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED))>0) {
        lua_pushstring(L, "weighted");
      } else if(MEMCACHED_BEHAVIOR_KETAMA_COMPAT != 0 && (value = memcached_behavior_get(memcache, MEMCACHED_BEHAVIOR_KETAMA_COMPAT))>0) {
        if(value==MEMCACHED_KETAMA_COMPAT_LIBMEMCACHED) lua_pushstring(L, "compat");
        if(value==MEMCACHED_KETAMA_COMPAT_SPY)          lua_pushstring(L, "compat_spy");
      } else {
        lua_pushstring(L, "none");
      }
    }
  }
}

static void add_behaviors(lua_State *L, memcached_st *memcache, memcached_return *status) {
  // wank to add behavior(s) (new/set_behavior)
  status          = NULL;
  int behavior    = 0;
  uint64_t val    = 0;
  const char *key = NULL;
  const char *vx  = NULL;

  // MEMCACHED_BEHAVIOR_CACHE_LOOKUPS

  lua_pushnil(L);
  while(lua_next(L, 1) != 0) {
    if(status!=NULL && status!=MEMCACHED_SUCCESS && lua_isstring(L, -1)) {
      lua_pop(L, 1);
      continue;
    }

    behavior = 0;
    key      = lua_tostring(L, -2);

    if(STRCMP(key, "use_binary"))      behavior = MEMCACHED_BEHAVIOR_BINARY_PROTOCOL;
    if(STRCMP(key, "use_udp"))         behavior = MEMCACHED_BEHAVIOR_USE_UDP;
    if(STRCMP(key, "no_block"))        behavior = MEMCACHED_BEHAVIOR_NO_BLOCK;
    if(STRCMP(key, "keepalive"))       behavior = MEMCACHED_BEHAVIOR_KEEPALIVE;
    if(STRCMP(key, "enable_cas"))      behavior = MEMCACHED_BEHAVIOR_SUPPORT_CAS;
    if(STRCMP(key, "tcp_nodelay"))     behavior = MEMCACHED_BEHAVIOR_TCP_NODELAY;
    if(STRCMP(key, "no_reply"))        behavior = MEMCACHED_BEHAVIOR_NOREPLY;
    if(STRCMP(key, "send_timeout"))    behavior = MEMCACHED_BEHAVIOR_SND_TIMEOUT;
    if(STRCMP(key, "receive_timeout")) behavior = MEMCACHED_BEHAVIOR_RCV_TIMEOUT;
    if(STRCMP(key, "connect_timeout")) behavior = MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT;
    if(STRCMP(key, "poll_timeout"))    behavior = MEMCACHED_BEHAVIOR_POLL_TIMEOUT;
    if(STRCMP(key, "keepalive_idle"))  behavior = MEMCACHED_BEHAVIOR_KEEPALIVE_IDLE;
    if(STRCMP(key, "retry_timeout"))   behavior = MEMCACHED_BEHAVIOR_RETRY_TIMEOUT;

    // value should be number or boolean
    if(behavior != 0 && (lua_isnumber(L, -1) || lua_isboolean(L, -1))) {
      val     = (uint64_t)lua_tointeger(L, -1);
      *status = memcached_behavior_set(memcache, behavior, val);

      lua_pop(L, 1);
      continue;
    }

    // value should be an string
    if(!lua_isstring(L, -1)) {
      lua_pop(L, 1);
      continue;
    }

    vx = lua_tostring(L, -1);

    if(STRCMP(key, "hash") || STRCMP(key, "ketama_hash")) {
      val = MEMCACHED_HASH_DEFAULT;
      if(STRCMP(vx, "md5"))      val = MEMCACHED_HASH_MD5;
      if(STRCMP(vx, "crc"))      val = MEMCACHED_HASH_CRC;
      if(STRCMP(vx,"fnv1_64"))   val = MEMCACHED_HASH_FNV1_64;
      if(STRCMP(vx, "fnv1a_64")) val = MEMCACHED_HASH_FNV1A_64;
      if(STRCMP(vx, "fnv1_32"))  val = MEMCACHED_HASH_FNV1_32;
      if(STRCMP(vx, "fnv1a_32")) val = MEMCACHED_HASH_FNV1A_32;
      if(STRCMP(vx, "jenkins"))  val = MEMCACHED_HASH_JENKINS;
      if(STRCMP(vx, "hsieh"))    val = MEMCACHED_HASH_HSIEH;
      if(STRCMP(vx, "murmur"))   val = MEMCACHED_HASH_MURMUR;

      if(STRCMP(key, "ketama_hash") && (STRCMP(vx, "jenkins") || STRCMP(vx, "hsieh") || STRCMP(vx, "murmur")))
        val = MEMCACHED_HASH_DEFAULT;

      *status = memcached_behavior_set(
                  memcache,
                  (STRCMP(key, "hash") ? MEMCACHED_BEHAVIOR_HASH : MEMCACHED_BEHAVIOR_KETAMA_HASH),
                  val
                );
    }

    //
    if(STRCMP(key, "distribution")) {
      behavior = MEMCACHED_BEHAVIOR_DISTRIBUTION;
      val      = MEMCACHED_DISTRIBUTION_MODULA;
      if(STRCMP(vx, "modula"))     val = MEMCACHED_DISTRIBUTION_MODULA;
      if(STRCMP(vx, "consistent")) val = MEMCACHED_DISTRIBUTION_CONSISTENT;
      if(STRCMP(vx, "weighted")) {
        val      = 1;
        behavior = MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED;
      }
      if(STRCMP(vx, "compat")) {
        val      = MEMCACHED_KETAMA_COMPAT_LIBMEMCACHED;
        behavior = MEMCACHED_BEHAVIOR_KETAMA_COMPAT;
      }
      if(STRCMP(vx, "compat_spy")) {
        val      = MEMCACHED_KETAMA_COMPAT_SPY;
        behavior = MEMCACHED_BEHAVIOR_KETAMA_COMPAT;
      }
      
      if(behavior != 0)
        *status = memcached_behavior_set(memcache, behavior, val);
    }
    
    //
    lua_pop(L, 1);
  }
}

static int save(lua_State *L, enum storage_type type) {
  memcached_st     *memcache = luaL_checkudata(L, 1, LUA_LIBMEMCACHED);
  time_t           ttl       = 0;
  int              stack     = 1;
  memcached_return status;
  uint64_t cas;
  size_t k_len, v_len;
  const char *k, *v;

  if(type == CAS)
    cas = luaL_checklong(L, ++stack);
  
  k = luaL_checklstring(L, ++stack, &k_len);
  v = luaL_checklstring(L, ++stack, &v_len);

  if(lua_gettop(L) > stack)
    ttl = luaL_checkint(L, ++stack);

  switch(type) {
    case ADD:     status = memcached_add(memcache, k, k_len, v, v_len, ttl, (uint32_t)0);      break;
    case SET:     status = memcached_set(memcache, k, k_len, v, v_len, ttl, (uint32_t)0);      break;
    case REPLACE: status = memcached_replace(memcache, k, k_len, v, v_len, ttl, (uint32_t)0);  break;
    case CAS:     status = memcached_cas(memcache, k, k_len, v, v_len, ttl, (uint32_t)0, cas); break;
    case APPEND:  status = memcached_append(memcache, k, k_len, v, v_len, ttl, (uint32_t)0);   break;
    case PREPEND: status = memcached_prepend(memcache, k, k_len, v, v_len, ttl, (uint32_t)0);  break;
    default:
      lua_pushboolean(L, 0);
      return 1;
      break;
  }

  if (status != MEMCACHED_SUCCESS) {
    lua_pushboolean(L, 0);
    lua_pushstring(L, memcached_strerror(memcache, status));
    return 2;
  }

  lua_pushboolean(L, 1);
  return 1;
}

static int operation(lua_State *L, enum operation_type type) {
  memcached_st     *memcache = luaL_checkudata(L, 1, LUA_LIBMEMCACHED);
  memcached_return status;
  const char       *k;
  int              v=1;
  size_t           k_len;
  uint64_t         n_value;

  k = luaL_checklstring(L, 2, &k_len);
  if(lua_gettop(L)>3)
    v = luaL_checklong(L, 3);

  switch(type) {
    case INCREMENT: status = memcached_increment(memcache, k, k_len, v, &n_value); break;
    case DECREMENT: status = memcached_decrement(memcache, k, k_len, v, &n_value); break;
    default:
      lua_pushnil(L);
      return 1;
      break;
  }

  if (status != MEMCACHED_SUCCESS) {
    lua_pushnil(L);
    lua_pushstring(L, memcached_strerror(memcache, status));
    return 2;
  }

  lua_pushnumber(L, n_value);
  return 1;
}

static int memc_add_server(lua_State *L) {
  return 0; 
};

static int memc_set_behavior(lua_State *L) { return 0; }

static int memc_get_behavior(lua_State *L) {
  memcached_st *memcache = luaL_checkudata(L, 1, LUA_LIBMEMCACHED);
  const char *const_name;

  if(lua_istable(L, 2)) {
    lua_newtable(L);
    
    lua_pushnil(L);
    while(lua_next(L, 2) != 0) {

      if(lua_type(L, -1) == LUA_TSTRING) {
        const_name = lua_tostring(L, -1);

        //printf("%s\n", const_name);

        //lua_pushstring(L, const_name);
        s2_behavior(L, const_name, memcache);

        //printf("%s\n", lua_typename(L, lua_type(L, -4)));

        lua_setfield(L, -4, const_name);
      }

      //printf(" %s - %s\n",
      //lua_typename(L, lua_type(L, -2)),
      //lua_typename(L, lua_type(L, -1)) );

      lua_pop(L, 1);
      printf("%s\n", lua_typename(L, lua_type(L, 1)));
      printf("%s\n", lua_typename(L, lua_type(L, 2)));
    }
  }

  //memcached_behavior_get(memcache, luaL_checklong(L, 2));
  return 1;
}

static int memc_add(lua_State *L) {
  return save(L, (s_type = ADD));
}

static int memc_set(lua_State *L) {
  return save(L, (s_type = SET));
}

static int memc_replace(lua_State *L) { 
  return save(L, (s_type = REPLACE));
}

static int memc_cas(lua_State *L) {
  return save(L, (s_type = CAS));
}

static int memc_append(lua_State *L) {
  return save(L, (s_type = APPEND));
}

static int memc_prepend(lua_State *L) {
  return save(L, (s_type = PREPEND));
}

static int memc_incr(lua_State *L) {
  return operation(L, (o_type = INCREMENT));
}

static int memc_decr(lua_State *L) {
  return operation(L, (o_type = DECREMENT));
}

static int memc_append_multi(lua_State *L) { return 0; }

static int memc_prepend_multi(lua_State *L) { return 0; }

static int memc_check_key(lua_State *L) { return 0; }

static int memc_set_multi(lua_State *L) { return 0; }

static int memc_delete(lua_State *L) { return 0; }

static int memc_delete_multi(lua_State *L) { return 0; }

static int memc_get(lua_State *L) {
  return 0;
}

static int memc_get_multi(lua_State *L) { return 0; }


//
static int memc_free(lua_State *L) {
  memcached_st *memcache = luaL_checkudata(L, 1, LUA_LIBMEMCACHED);
  memcached_free(memcache);  
  return 0;
}

// new instance
static int memc_new(lua_State *L) {
  memcached_st        *memcache;
  memcached_server_st *servers = NULL;
  memcached_return    status;

  memcache = lua_newuserdata(L, sizeof(struct memcached_st));
  memcache = memcached_create(memcache);
  
  add_servers(L, memcache, &status);
  //servers  = memcached_server_list_append(servers, "localhost", 11211, &status);
  //status   = memcached_server_push(memcache, servers);

  if (status != MEMCACHED_SUCCESS) {
    //fprintf(stderr, "Couldn't add server: %s\n", memcached_strerror(memcache, status));
    lua_pushnil(L);
    lua_pushstring(L, memcached_strerror(memcache, status));
    return 2;
  }

  luaL_getmetatable(L, LUA_LIBMEMCACHED);
  lua_setmetatable(L, -2);

  return 1;
}

// register lib
LUALIB_API int luaopen_libmemcached(lua_State *L) {

  // create metatable.
  luaL_newmetatable(L, LUA_LIBMEMCACHED);
  luaL_setfuncs(L, metamethods, 0);

  // create methods table.
  lua_newtable(L);
  luaL_setfuncs(L, methods, 0);

  // set __index field of metatable to point to methods table.
  lua_setfield(L, -2, "__index");

  // pop metatable, it is no longer needed.
  lua_pop(L, 1);

  // create module functions table.
  lua_newtable(L);

  // set functions.
  luaL_setfuncs(L, functions, 0);

  lua_set_sconst(L, "BEER-WARE", "_COPYRIGHT");
  lua_set_sconst(L, "Lua binding to libmemcached", "_DESCRIPTION");
  lua_set_sconst(L, "0.1a", "_VERSION");
  lua_set_sconst(L, LIBMEMCACHED_VERSION_STRING, "_LIBVERSION");

  return 1;
}