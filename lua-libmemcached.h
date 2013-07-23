/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <xxleite@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return
 * ----------------------------------------------------------------------------
 */

#ifndef LUA_API
#define LUA_API __declspec(dllexport)
#endif


#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM==501
#define lua_rawlen lua_objlen
#define lua_getuservalue lua_getfenv
#define lua_setuservalue lua_setfenv
#define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
#define luaL_setfuncs(L,l,s) luaL_register(L,NULL,l)
#endif

#define lua_set_const(L, con, name) {lua_pushnumber(L, con); lua_setfield(L, -2, name);}
#define LUA_LIBMEMCACHED "libmemcached"

static int memc_new(lua_State *L);
static int memc_add_server(lua_State *L);
static int memc_set_behavior(lua_State *L);
static int memc_get_behavior(lua_State *L);
static int memc_add(lua_State *L);
static int memc_replace(lua_State *L);
static int memc_cas(lua_State *L);
static int memc_append(lua_State *L);
static int memc_prepend(lua_State *L);
static int memc_append_multi(lua_State *L);
static int memc_prepend_multi(lua_State *L);
static int memc_check_key(lua_State *L);
static int memc_set(lua_State *L);
static int memc_set_multi(lua_State *L);
static int memc_delete(lua_State *L);
static int memc_delete_multi(lua_State *L);
static int memc_get(lua_State *L);
static int memc_get_multi(lua_State *L);
static int memc_incr(lua_State *L);
static int memc_decr(lua_State *L);
static int memc_free(lua_State *L);