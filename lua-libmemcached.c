/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <xxleite@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include <libmemcached/memcached.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define STRCMP(a,b) (strcmp(a,b) == 0)

#ifndef LUA_API
#define LUA_API __declspec(dllexport)
#endif

#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM==501
#define luaL_setfuncs(L,l,s) luaL_register(L,NULL,l)
#endif

#define lua_set_const(L, con, name) {lua_pushnumber(L, con); lua_setfield(L, -2, name);}
#define lua_set_sconst(L, con, name) {lua_pushstring(L, con); lua_setfield(L, -2, name);}
#define LUA_LIBMEMCACHED "libmemcached"

#if !defined mempcpy
void *mempcpy(void *dst, const void *src, size_t len) {
  return (char *) memcpy(dst, src, len) + len;
}
#endif

#if !defined LIBMEMCACHED_VERSION_HEX || LIBMEMCACHED_VERSION_HEX<0x1000015
#define MEMCACHED_HASH_MURMUR3 -3
#endif

#if !defined LIBMEMCACHED_VERSION_HEX || LIBMEMCACHED_VERSION_HEX>0x1000002
#define MEMCACHED_CONNECTION_MAX -6
#define MEMCACHED_CONNECTION_UNKNOWN -9
#endif

#ifdef DEBUG
static void log_me(const char *name) {
  printf("\n >> %s\n", name);
}

static void stack_dump(lua_State *L) {
  int i, t, top;
  top = lua_gettop(L);
  for (i = 1; i <= top; i++) {
    t = lua_type(L, i);
    if(i>1)
      printf(", ");
    switch (t) {
      case LUA_TSTRING:  printf("`%s'", lua_tostring(L, i));              break;
      case LUA_TBOOLEAN: printf(lua_toboolean(L, i) ? "true" : "false");  break;
      case LUA_TNUMBER:  printf("%g", lua_tonumber(L, i));                break;
      case LUA_TNIL:     printf("nil");                                   break;
      default: printf("%s: %p", lua_typename(L, t), lua_topointer(L, i)); break;
    }
  }
  printf("\n");
}
#else
static void log_me(const char *name) {}
static void stack_dump(lua_State *L) {}
#endif

//

enum storage_type{ ADD, SET, REPLACE, CAS, APPEND, PREPEND } s_type;
enum operation_type{ INCREMENT, DECREMENT } o_type;

static void should_buffer(memcached_st *memcache, int *bitwise_flag) {
  if(*bitwise_flag > 0) {
    if((*bitwise_flag & 2))
      memcached_behavior_set(memcache, MEMCACHED_BEHAVIOR_NOREPLY, 0);
    if((*bitwise_flag & 4))
      memcached_behavior_set(memcache, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 0);
  } else {
    *bitwise_flag  = 1;
    *bitwise_flag += ((memcached_behavior_get(memcache, MEMCACHED_BEHAVIOR_NOREPLY)==0) ? 2 : 0);
    *bitwise_flag += ((memcached_behavior_get(memcache, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS)==0) ? 4 : 0);
    if((*bitwise_flag & 2))
      memcached_behavior_set(memcache, MEMCACHED_BEHAVIOR_NOREPLY, 1);
    if((*bitwise_flag & 4))
      memcached_behavior_set(memcache, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 1);
  }
}

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
      memcached_server_list_free(servers);
      return; 
  }

  memcached_server_list_free(servers);
}

static void get_behavior(lua_State *L, const char *const_name, memcached_st *memcache) {
  int behavior = -2;
  uint64_t value;

  if(STRCMP(const_name, "use_binary"))  behavior = MEMCACHED_BEHAVIOR_BINARY_PROTOCOL;
  if(STRCMP(const_name, "use_udp"))     behavior = MEMCACHED_BEHAVIOR_USE_UDP;
  if(STRCMP(const_name, "no_block"))    behavior = MEMCACHED_BEHAVIOR_NO_BLOCK;
  if(STRCMP(const_name, "keepalive"))   behavior = MEMCACHED_BEHAVIOR_TCP_KEEPALIVE;
  if(STRCMP(const_name, "enable_cas"))  behavior = MEMCACHED_BEHAVIOR_SUPPORT_CAS;
  if(STRCMP(const_name, "tcp_nodelay")) behavior = MEMCACHED_BEHAVIOR_TCP_NODELAY;
  if(STRCMP(const_name, "no_reply"))    behavior = MEMCACHED_BEHAVIOR_NOREPLY;
  if(behavior > -2) {
    value = memcached_behavior_get(memcache, behavior);
    lua_pushboolean(L, value);
    return;
  }

  if(STRCMP(const_name, "send_timeout"))    behavior = MEMCACHED_BEHAVIOR_SND_TIMEOUT;
  if(STRCMP(const_name, "receive_timeout")) behavior = MEMCACHED_BEHAVIOR_RCV_TIMEOUT;
  if(STRCMP(const_name, "connect_timeout")) behavior = MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT;
  if(STRCMP(const_name, "poll_timeout"))    behavior = MEMCACHED_BEHAVIOR_POLL_TIMEOUT;
  if(STRCMP(const_name, "keepalive_idle"))  behavior = MEMCACHED_BEHAVIOR_TCP_KEEPIDLE;
  if(STRCMP(const_name, "retry_timeout"))   behavior = MEMCACHED_BEHAVIOR_RETRY_TIMEOUT;
  if(behavior > -2) {
    value = memcached_behavior_get(memcache, behavior);
    lua_pushnumber(L, value);
    return;
  }

  if(STRCMP(const_name, "hash"))        behavior = MEMCACHED_BEHAVIOR_HASH;
  if(STRCMP(const_name, "ketama_hash")) behavior = MEMCACHED_BEHAVIOR_KETAMA_HASH;
  if(behavior > -2) {
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
      case MEMCACHED_HASH_MURMUR3:  lua_pushstring(L, "murmur3");
      case MEMCACHED_HASH_DEFAULT:  
      default: 
        lua_pushstring(L, "default");
        break;
    }
    return;
  }

  if(STRCMP(const_name, "distribution")) behavior = MEMCACHED_BEHAVIOR_DISTRIBUTION;
  if(behavior > -2) {
    value = memcached_behavior_get(memcache, behavior);
    switch(value) {
      case MEMCACHED_DISTRIBUTION_MODULA:                lua_pushstring(L, "modula");     break;
      case MEMCACHED_DISTRIBUTION_CONSISTENT:            lua_pushstring(L, "consistent"); break;
      case MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA:     lua_pushstring(L, "ketama");     break;
      case MEMCACHED_DISTRIBUTION_RANDOM:                lua_pushstring(L, "random");     break;
      case MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY: lua_pushstring(L, "spy");        break;
      case MEMCACHED_DISTRIBUTION_CONSISTENT_MAX:        lua_pushstring(L, "max");        break;
      default:
        value = memcached_behavior_get(memcache, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED);
        if(value>-1)
          lua_pushstring(L, "weighted");
        else         
          lua_pushstring(L, "none");
        break;
    }
  } else {
    lua_pushnil(L);
  }
}

static void add_behaviors(lua_State *L, memcached_st *memcache, memcached_return *status) {
  // wank to add behavior(s) (new/set_behavior)
  int x           = 0;
  int behavior    = -1;
  uint64_t val    = 0;
  const char *key = NULL;
  const char *vx  = NULL;

  lua_pushnil(L);
  while(lua_next(L, 1) != 0) {
    if(x>0 && *status!=MEMCACHED_SUCCESS && lua_isstring(L, -1)) {
      lua_pop(L, 1);
      continue;
    }

    ++x;
    behavior = -1;
    key      = lua_tostring(L, -2);

    if(STRCMP(key, "use_binary"))      behavior = MEMCACHED_BEHAVIOR_BINARY_PROTOCOL;
    if(STRCMP(key, "use_udp"))         behavior = MEMCACHED_BEHAVIOR_USE_UDP;
    if(STRCMP(key, "no_block"))        behavior = MEMCACHED_BEHAVIOR_NO_BLOCK;
    if(STRCMP(key, "keepalive"))       behavior = MEMCACHED_BEHAVIOR_TCP_KEEPALIVE;
    if(STRCMP(key, "enable_cas"))      behavior = MEMCACHED_BEHAVIOR_SUPPORT_CAS;
    if(STRCMP(key, "tcp_nodelay"))     behavior = MEMCACHED_BEHAVIOR_TCP_NODELAY;
    if(STRCMP(key, "no_reply"))        behavior = MEMCACHED_BEHAVIOR_NOREPLY;
    if(STRCMP(key, "send_timeout"))    behavior = MEMCACHED_BEHAVIOR_SND_TIMEOUT;
    if(STRCMP(key, "receive_timeout")) behavior = MEMCACHED_BEHAVIOR_RCV_TIMEOUT;
    if(STRCMP(key, "connect_timeout")) behavior = MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT;
    if(STRCMP(key, "poll_timeout"))    behavior = MEMCACHED_BEHAVIOR_POLL_TIMEOUT;
    if(STRCMP(key, "keepalive_idle"))  behavior = MEMCACHED_BEHAVIOR_TCP_KEEPIDLE;
    if(STRCMP(key, "retry_timeout"))   behavior = MEMCACHED_BEHAVIOR_RETRY_TIMEOUT;

    // value should be number or boolean
    if(behavior != -1 && (lua_isnumber(L, -1) || lua_isboolean(L, -1))) {
      val     = (uint64_t)(lua_type(L, -1)==LUA_TBOOLEAN ? lua_toboolean(L, -1) : lua_tointeger(L, -1));
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
      if(STRCMP(vx, "fnv1_64"))  val = MEMCACHED_HASH_FNV1_64;
      if(STRCMP(vx, "fnv1a_64")) val = MEMCACHED_HASH_FNV1A_64;
      if(STRCMP(vx, "fnv1_32"))  val = MEMCACHED_HASH_FNV1_32;
      if(STRCMP(vx, "fnv1a_32")) val = MEMCACHED_HASH_FNV1A_32;
      if(STRCMP(vx, "jenkins"))  val = MEMCACHED_HASH_JENKINS;
      if(STRCMP(vx, "hsieh"))    val = MEMCACHED_HASH_HSIEH;
      if(STRCMP(vx, "murmur"))   val = MEMCACHED_HASH_MURMUR;
      if(STRCMP(vx, "murmur3"))  val = MEMCACHED_HASH_MURMUR3;

      if(
        STRCMP(key, "ketama_hash") && 
        (STRCMP(vx, "jenkins") || STRCMP(vx, "hsieh") || STRCMP(vx, "murmur") || STRCMP(vx, "murmur3"))
      )
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
      if(STRCMP(vx, "ketama"))     val = MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA;
      if(STRCMP(vx, "random"))     val = MEMCACHED_DISTRIBUTION_RANDOM;
      if(STRCMP(vx, "spy"))        val = MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY;
      if(STRCMP(vx, "max"))        val = MEMCACHED_DISTRIBUTION_CONSISTENT_MAX;
      if(STRCMP(vx, "weighted"))   val = 1; behavior = MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED;
      if(behavior != -1)
        *status = memcached_behavior_set(memcache, behavior, val);
    }
    
    //
    lua_pop(L, 1);
  }
}

static int save(lua_State *L, enum storage_type type) {
  memcached_st     *memcache = luaL_checkudata(L, 1, LUA_LIBMEMCACHED);
  time_t           ttl       = 0;
  memcached_return status;
  uint64_t cas;
  size_t k_len, v_len;
  const char *k, *v;
  LIBMEMCACHED_API
  memcached_return_t (*strategy)(memcached_st *, const char *, size_t, const char *, size_t, time_t, uint32_t);

  if(type == CAS) {
    cas = luaL_checklong(L, 1);
    k   = luaL_checklstring(L, 2, &k_len);
    v   = luaL_checklstring(L, 3, &v_len);
    if(lua_gettop(L) > 3)
      ttl = luaL_checkint(L, 4);
    status = memcached_cas(memcache, k, k_len, v, v_len, ttl, (uint32_t)0, cas);

    if (status != MEMCACHED_SUCCESS) {
      lua_pushboolean(L, 0);
      lua_pushstring(L, memcached_strerror(memcache, status));
      return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
  }

  switch(type) {
    case ADD:     strategy = &memcached_add;     break;
    case SET:     strategy = &memcached_set;     break;
    case REPLACE: strategy = &memcached_replace; break;
    case APPEND:  strategy = &memcached_append;  break;
    case PREPEND: strategy = &memcached_prepend; break;
    default:
      lua_pushboolean(L, 0);
      return 1;
  }

  if(lua_type(L, 2) == LUA_TTABLE) {
    int bt_flag = 0;
    if(lua_gettop(L) > 2)
      ttl = luaL_checkint(L, 3);

    should_buffer(memcache, &bt_flag);

    lua_pushnil(L);
    while(lua_next(L, 2) != 0) {
      k = lua_tolstring(L, -2, &k_len);
      v = lua_tolstring(L, -1, &v_len);
      (*strategy)(memcache, k, k_len, v, v_len, ttl, (uint32_t)0);

      lua_pop(L, 1);
    }
    status = memcached_flush_buffers(memcache);
    should_buffer(memcache, &bt_flag);

  } else {
    k = luaL_checklstring(L, 2, &k_len);
    v = luaL_checklstring(L, 3, &v_len);
    if(lua_gettop(L) > 3)
      ttl = luaL_checkint(L, 4);

    status = (*strategy)(memcache, k, k_len, v, v_len, ttl, (uint32_t)0);
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
  int              v = 1;
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
  memcached_st     *memcache = luaL_checkudata(L, 1, LUA_LIBMEMCACHED);
  memcached_return status;

  // change stack
  lua_pushvalue(L, -1);
  lua_replace(L, 1);
  lua_pop(L, 1);
  if(lua_gettop(L)==2 && lua_isnumber(L, 1)) {
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_replace(L, 1);
    lua_replace(L, 2);
  }

  add_servers(L, memcache, &status);

  if (status != MEMCACHED_SUCCESS) {
    lua_pushboolean(L, 0);
    lua_pushstring(L, memcached_strerror(memcache, status));
    return 2;
  }

  lua_pushboolean(L, 1);
  return 1;
};

static int memc_set_behavior(lua_State *L) {
  memcached_st     *memcache = luaL_checkudata(L, 1, LUA_LIBMEMCACHED);
  memcached_return status;

  // change stack
  lua_pushvalue(L, -1);
  lua_replace(L, 1);
  lua_pop(L, 1);

  add_behaviors(L, memcache, &status);

  if (status != MEMCACHED_SUCCESS) {
    lua_pushboolean(L, 0);
    lua_pushstring(L, memcached_strerror(memcache, status));
    return 2;
  }
  
  return 0;
}

static int memc_get_behavior(lua_State *L) {
  memcached_st *memcache = luaL_checkudata(L, 1, LUA_LIBMEMCACHED);
  const char   *const_name;

  switch(lua_type(L, 2)) {
    case LUA_TTABLE: 
      lua_newtable(L);
      lua_pushnil(L);
      // 
      while(lua_next(L, 2) != 0) {
      
        if(lua_type(L, -1) == LUA_TSTRING) {
          const_name = lua_tostring(L, -1);
          get_behavior(L, const_name, memcache);
          lua_setfield(L, -4, const_name);
        }
      
        lua_pop(L, 1);
      }
      break;
    case LUA_TSTRING:
      const_name = lua_tostring(L, 2);
      get_behavior(L, const_name, memcache);
      break;
    default:
      return luaL_error(L, "argument must be string or table");
  }
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

static int memc_get(lua_State *L) {
  memcached_st        *memcache = luaL_checkudata(L, 1, LUA_LIBMEMCACHED);
  memcached_return    status;
  memcached_result_st *results = NULL;
  uint64_t   cas;
  size_t     len, *keys_len, n = 1;
  const char *k, *key, *value;
  const char **keys;
  
  switch(lua_type(L, 2)) {
    case LUA_TTABLE:
      n        = luaL_getn(L, 2);
      keys     = malloc(n * sizeof(char *));
      keys_len = malloc(n * sizeof *keys_len);

      n = -1;
      lua_pushnil(L);
      while(lua_next(L, 2) != 0) {
        if(lua_type(L, -1) == LUA_TSTRING) {
          k           = lua_tolstring(L, -1, &len);
          keys[++n]   = malloc((len+1) * sizeof(char *));
          keys_len[n] = len;
          *((char *) mempcpy((char*)keys[n], k, len)) = '\0';
        }
        lua_pop(L, 1);
      }
      ++n;
      break;
    case LUA_TSTRING:
      keys     = malloc(n * sizeof(char *));
      keys_len = malloc(n * sizeof *keys_len);

      k           = lua_tolstring(L, -1, &len);
      keys[0]     = malloc((len+1) * sizeof(char *));
      keys_len[0] = len;
      *((char *) mempcpy((char*)keys[0], k, len)) = '\0';
      break;
    default:
      return luaL_error(L, "parameter should be string or table");
  }

  status = memcached_mget(memcache, keys, keys_len, n);
  if(status == MEMCACHED_SUCCESS) {
    if(lua_type(L, 2) == LUA_TTABLE) {
      n = 1;
      lua_newtable(L);
      while((results = memcached_fetch_result(memcache, NULL, &status)) != NULL) {
        key   = memcached_result_key_value(results);
        value = memcached_result_value(results);

        lua_pushstring(L, key);
        lua_pushstring(L, value);
        lua_settable(L, 3);
      }
    } else {
      n = 2;
      if((results = memcached_fetch_result(memcache, NULL, &status)) != NULL) {
        value = memcached_result_value(results);
        cas   = memcached_result_cas(results);

        lua_pushstring(L, value);
        lua_pushnumber(L, cas);
      } else {
        lua_pushboolean(L, 0);
        lua_pushstring(L, memcached_strerror(memcache, status));
      }
    }
  } else {
    n = 2;
    lua_pushboolean(L, 0);
    lua_pushstring(L, memcached_strerror(memcache, status));
  }

  memcached_result_free(results);
  free(keys);
  free(keys_len);
  return n;
}

static int memc_delete(lua_State *L) {
  memcached_st     *memcache = luaL_checkudata(L, 1, LUA_LIBMEMCACHED);
  memcached_return status;
  time_t           ttl = 0;
  size_t           k_len;
  const char       *k;
  int              bt_flag = 0;

  if(lua_gettop(L)>2)
    ttl = luaL_checkint(L, 3);

  switch(lua_type(L, 2)) {
    case LUA_TTABLE:
      should_buffer(memcache, &bt_flag);
      
      lua_pushnil(L);
      while(lua_next(L, 2) != 0) {
        if(lua_type(L, -1) == LUA_TSTRING) {
          k = lua_tolstring(L, -1, &k_len);
          memcached_delete(memcache, k, k_len, ttl);
        }
      }
      
      status = memcached_flush_buffers(memcache);
      should_buffer(memcache, &bt_flag);
      lua_pushboolean(L, (status==MEMCACHED_SUCCESS ? 1 : 0));

      break;
    case LUA_TSTRING:
      k      = lua_tolstring(L, 2, &k_len);
      status = memcached_delete(memcache, k, k_len, ttl);
      lua_pushboolean(L, (MEMCACHED_SUCCESS==status ? 1 : 0));
      break;
    default:
      return luaL_error(L, "argument must be string or table");
  }
  return 1;
}

static int memc_check_key(lua_State *L) {
  if(lua_gettop(L) > 1 && lua_isstring(L, 2)) {
    size_t len;
    lua_tolstring(L, 2, &len);
    if(MEMCACHED_MAX_KEY < len) {
      lua_pushboolean(L, 0);
      lua_pushstring(L, "key is greater than memcached limit");
      return 2;
    }

    lua_pushboolean(L, 1);
  } else {
    return luaL_error(L, "argument must be a string");
  }
  return 1;
}

//
static int memc_free(lua_State *L) {
  memcached_st *memcache = luaL_checkudata(L, 1, LUA_LIBMEMCACHED);
  memcached_free(memcache);  
  return 0;
}

// new instance
static int memc_new(lua_State *L) {
  memcached_st        *memcache;
  memcached_return    status;

  memcache = lua_newuserdata(L, sizeof(struct memcached_st));
  memcache = memcached_create(memcache);

  // preset
  status = memcached_behavior_set(memcache, MEMCACHED_BEHAVIOR_VERIFY_KEY, 1);

  if(MEMCACHED_SUCCESS != status)
    status = memcached_behavior_set(memcache, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);

  // use murmur3/murmur if multiple servers
  if(MEMCACHED_SUCCESS != status && lua_type(L, 1)==LUA_TTABLE) {
    if(-1 != MEMCACHED_HASH_MURMUR3)
      status = memcached_behavior_set(memcache, MEMCACHED_BEHAVIOR_HASH, MEMCACHED_HASH_MURMUR3);
    else
      status = memcached_behavior_set(memcache, MEMCACHED_BEHAVIOR_HASH, MEMCACHED_HASH_MURMUR);
  }

  if (status != MEMCACHED_SUCCESS) {
    lua_pushnil(L);
    lua_pushstring(L, memcached_strerror(memcache, status));
    return 2;
  }

  // check for behavior(s)
  if(lua_gettop(L)>1 && (lua_type(L, 2)==LUA_TTABLE || lua_type(L, 3)==LUA_TTABLE)) {
    lua_pushvalue(L, 1);
    if(lua_type(L, 2)==LUA_TTABLE)
      lua_pushvalue(L, 2);
    else
      lua_pushvalue(L, 3);

    lua_replace(L, 1);
    add_behaviors(L, memcache, &status);

    if (status != MEMCACHED_SUCCESS) {
      lua_pushnil(L);
      lua_pushstring(L, memcached_strerror(memcache, status));
      return 2;
    }

    lua_replace(L, 1);
  }
  
  add_servers(L, memcache, &status);

  if (status != MEMCACHED_SUCCESS) {
    lua_pushnil(L);
    lua_pushstring(L, memcached_strerror(memcache, status));
    return 2;
  }

  luaL_getmetatable(L, LUA_LIBMEMCACHED);
  lua_setmetatable(L, -2);

  return 1;
}

//
const struct luaL_Reg methods[] = {
  {"add_server",   &memc_add_server},
  {"set_behavior", &memc_set_behavior},
  {"get_behavior", &memc_get_behavior},
  {"add",          &memc_add},
  {"replace",      &memc_replace},
  {"cas",          &memc_cas},
  {"append",       &memc_append},
  {"prepend",      &memc_prepend},
  {"check_key",    &memc_check_key},
  {"set",          &memc_set},
  {"delete",       &memc_delete},
  {"get",          &memc_get},
  {"incr",         &memc_incr},
  {"decr",         &memc_decr},
  {NULL,           NULL}
};

//
const struct luaL_Reg metamethods[] = {
  {"__gc", &memc_free},
  {NULL,   NULL}
};

//
const struct luaL_Reg functions[] = {
  {"new", &memc_new},
  {NULL,  NULL}
};

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
  lua_set_sconst(L, "0.1b", "_VERSION");
  lua_set_sconst(L, LIBMEMCACHED_VERSION_STRING, "_LIBVERSION");

  return 1;
}