/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <xxleite@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <libmemcached/memcached.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <lua-libmemcached.h>

// meta methods
const struct luaL_reg memc_methods[] = {
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

const struct luaL_reg memc_reg[] = {
  {"new", memc_new},
  {NULL,  NULL}
};

static int memc_add_server(lua_State *L) { return 0; }

static int memc_set_behavior(lua_State *L) { return 0; }

static int memc_get_behavior(lua_State *L) { return 0; }

static int memc_add(lua_State *L) { return 0; }

static int memc_replace(lua_State *L) { return 0; }

static int memc_cas(lua_State *L) { return 0; }

static int memc_append(lua_State *L) { return 0; }

static int memc_prepend(lua_State *L) { return 0; }

static int memc_append_multi(lua_State *L) { return 0; }

static int memc_prepend_multi(lua_State *L) { return 0; }

static int memc_check_key(lua_State *L) { return 0; }

static int memc_set(lua_State *L) { return 0; }

static int memc_set_multi(lua_State *L) { return 0; }

static int memc_delete(lua_State *L) { return 0; }

static int memc_delete_multi(lua_State *L) { return 0; }

static int memc_get(lua_State *L) { return 0; }

static int memc_get_multi(lua_State *L) { return 0; }

static int memc_incr(lua_State *L) { return 0; }

static int memc_decr(lua_State *L) { return 0; }

// free garbage after gc call
static int memc_free(lua_State *L) { 
  printf("## __gc\n");
  return 0;
}

// new instance
static int memc_new(lua_State *L) {

  lua_createtable(L, 0, 0);

  luaL_newmetatable(L, "memc");

  lua_pushcfunction(L, memc_free);
  lua_setfield(L, -2, "__gc");

  luaL_newlib(L, memc_methods);
  lua_setfield(L, -2, "__index");

  lua_setmetatable(L, -1);

  luaL_getmetatable(L, "libmemcached");
  lua_setmetatable(L, -2);
  return 1;  
}

// register lib
LUALIB_API int luaopen_libmemcached(lua_State *L) {
  luaL_register(L, "libmemcached", memc_obj);

  lua_set_const(L, MEMCACHED_BEHAVIOR_NO_BLOCK,               "BEHAVIOR_NO_BLOCK");
  lua_set_const(L, MEMCACHED_BEHAVIOR_TCP_NODELAY,            "BEHAVIOR_TCP_NODELAY");
  lua_set_const(L, MEMCACHED_BEHAVIOR_HASH,                   "BEHAVIOR_HASH");
  lua_set_const(L, MEMCACHED_BEHAVIOR_KETAMA,                 "BEHAVIOR_KETAMA");
  lua_set_const(L, MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE,       "BEHAVIOR_SOCKET_SEND_SIZE");
  lua_set_const(L, MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE,       "BEHAVIOR_SOCKET_RECV_SIZE");
  lua_set_const(L, MEMCACHED_BEHAVIOR_CACHE_LOOKUPS,          "BEHAVIOR_CACHE_LOOKUPS");
  lua_set_const(L, MEMCACHED_BEHAVIOR_SUPPORT_CAS,            "BEHAVIOR_SUPPORT_CAS");
  lua_set_const(L, MEMCACHED_BEHAVIOR_POLL_TIMEOUT,           "BEHAVIOR_POLL_TIMEOUT");
  lua_set_const(L, MEMCACHED_BEHAVIOR_DISTRIBUTION,           "BEHAVIOR_DISTRIBUTION");
  lua_set_const(L, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS,        "BEHAVIOR_BUFFER_REQUESTS");
  lua_set_const(L, MEMCACHED_BEHAVIOR_USER_DATA,              "BEHAVIOR_USER_DATA");
  lua_set_const(L, MEMCACHED_BEHAVIOR_SORT_HOSTS,             "BEHAVIOR_SORT_HOSTS");
  lua_set_const(L, MEMCACHED_BEHAVIOR_VERIFY_KEY,             "BEHAVIOR_VERIFY_KEY");
  lua_set_const(L, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT,        "BEHAVIOR_CONNECT_TIMEOUT");
  lua_set_const(L, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT,          "BEHAVIOR_RETRY_TIMEOUT");
  lua_set_const(L, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED,        "BEHAVIOR_KETAMA_WEIGHTED");
  lua_set_const(L, MEMCACHED_BEHAVIOR_KETAMA_HASH,            "BEHAVIOR_KETAMA_HASH");
  lua_set_const(L, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL,        "BEHAVIOR_BINARY_PROTOCOL");
  lua_set_const(L, MEMCACHED_BEHAVIOR_SND_TIMEOUT,            "BEHAVIOR_SND_TIMEOUT");
  lua_set_const(L, MEMCACHED_BEHAVIOR_RCV_TIMEOUT,            "BEHAVIOR_RCV_TIMEOUT");
  lua_set_const(L, MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT,   "BEHAVIOR_SERVER_FAILURE_LIMIT");
  lua_set_const(L, MEMCACHED_BEHAVIOR_IO_MSG_WATERMARK,       "BEHAVIOR_IO_MSG_WATERMARK");
  lua_set_const(L, MEMCACHED_BEHAVIOR_IO_BYTES_WATERMARK,     "BEHAVIOR_IO_BYTES_WATERMARK");
  lua_set_const(L, MEMCACHED_BEHAVIOR_IO_KEY_PREFETCH,        "BEHAVIOR_IO_KEY_PREFETCH");
  lua_set_const(L, MEMCACHED_BEHAVIOR_HASH_WITH_PREFIX_KEY,   "BEHAVIOR_HASH_WITH_PREFIX_KEY");
  lua_set_const(L, MEMCACHED_BEHAVIOR_NOREPLY,                "BEHAVIOR_NOREPLY");
  lua_set_const(L, MEMCACHED_BEHAVIOR_USE_UDP,                "BEHAVIOR_USE_UDP");
  lua_set_const(L, MEMCACHED_BEHAVIOR_AUTO_EJECT_HOSTS,       "BEHAVIOR_AUTO_EJECT_HOSTS");
  lua_set_const(L, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS,     "BEHAVIOR_NUMBER_OF_REPLICAS");
  lua_set_const(L, MEMCACHED_BEHAVIOR_RANDOMIZE_REPLICA_READ, "BEHAVIOR_RANDOMIZE_REPLICA_READ");
  lua_set_const(L, MEMCACHED_BEHAVIOR_CORK,                   "BEHAVIOR_CORK");
  lua_set_const(L, MEMCACHED_BEHAVIOR_TCP_KEEPALIVE,          "BEHAVIOR_TCP_KEEPALIVE");
  lua_set_const(L, MEMCACHED_BEHAVIOR_TCP_KEEPIDLE,           "BEHAVIOR_TCP_KEEPIDLE");
  lua_set_const(L, MEMCACHED_BEHAVIOR_MAX,                    "BEHAVIOR_MAX");

  // memcached_server_distribution:
  lua_set_const(L, MEMCACHED_DISTRIBUTION_MODULA,                "DISTRIBUTION_MODULA");
  lua_set_const(L, MEMCACHED_DISTRIBUTION_CONSISTENT,            "DISTRIBUTION_CONSISTENT");
  lua_set_const(L, MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA,     "DISTRIBUTION_CONSISTENT_KETAMA");
  lua_set_const(L, MEMCACHED_DISTRIBUTION_RANDOM,                "DISTRIBUTION_RANDOM");
  lua_set_const(L, MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY, "DISTRIBUTION_CONSISTENT_KETAMA_SPY");
  lua_set_const(L, MEMCACHED_DISTRIBUTION_CONSISTENT_MAX,        "DISTRIBUTION_CONSISTENT_MAX");

  // memcached_hash:
  lua_set_const(L, MEMCACHED_HASH_DEFAULT,  "HASH_DEFAULT"); //=0
  lua_set_const(L, MEMCACHED_HASH_MD5,      "HASH_MD5");
  lua_set_const(L, MEMCACHED_HASH_CRC,      "HASH_CRC");
  lua_set_const(L, MEMCACHED_HASH_FNV1_64,  "HASH_FNV1_64");
  lua_set_const(L, MEMCACHED_HASH_FNV1A_64, "HASH_FNV1A_64");
  lua_set_const(L, MEMCACHED_HASH_FNV1_32,  "HASH_FNV1_32");
  lua_set_const(L, MEMCACHED_HASH_FNV1A_32, "HASH_FNV1A_32");
  lua_set_const(L, MEMCACHED_HASH_HSIEH,    "HASH_HSIEH");
  lua_set_const(L, MEMCACHED_HASH_MURMUR,   "HASH_MURMUR");
  lua_set_const(L, MEMCACHED_HASH_JENKINS,  "HASH_JENKINS");
  lua_set_const(L, MEMCACHED_HASH_CUSTOM,   "HASH_CUSTOM");
  lua_set_const(L, MEMCACHED_HASH_MAX,      "HASH_MAX");

  // memcached_connection
  lua_set_const(L, MEMCACHED_CONNECTION_UNKNOWN,     "CONNECTION_UNKNOWN");
  lua_set_const(L, MEMCACHED_CONNECTION_TCP,         "CONNECTION_TCP");
  lua_set_const(L, MEMCACHED_CONNECTION_UDP,         "CONNECTION_UDP");
  lua_set_const(L, MEMCACHED_CONNECTION_UNIX_SOCKET, "CONNECTION_UNIX_SOCKET");
  lua_set_const(L, MEMCACHED_CONNECTION_MAX,         "CONNECTION_MAX");

  return 1;
}