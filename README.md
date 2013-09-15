# summary

lua-libmemcached is a simple [Lua](http://www.lua.org/) binding for [libmemcached](http://bit.ly/libmemcached).

# help and support

please fill an issue or help it doing a clone and then a pull request.

# license

[BEER-WARE](http://bit.ly/b33rw4r3), see source

# prerequisites

+ [libmemcached](http://bit.ly/libmemcached) (of course) 
+ [Lua](http://www.lua.org/) ([Luajit](http://luajit.org/))
+ [gcc](http://gcc.gnu.org/) and [make](http://www.gnu.org/software/make/) ([min-gw32](http://sourceforge.net/projects/mingw/files/MinGW/) on windows)
+ [upx](http://upx.sourceforge.net/) (optional)

# installation

compatible with *NIX systens, supposed to works on windows. You only need to edit some vars on makefile, for basic instalation use:

    # make

if you have upx use this for better output.
  
    # make release
  
for cleanup

    # make clean
  
# basic usage

```lua

--[[
  
  behavior*  = further reading http://bit.ly/b3h4v1or
  ttl*/time* = (time to live) in seconds, optional, default 0 (indeterminate)
  ttl**      = (time to live) in seconds, optional, default 0 (no changes last ttl)
  offset*    = value offset, optional, default 1
  **         = for async callback pass a function as last argument

--]]

-- useful variables
local memc, inst, behavior, value, cas_token
local key, value, key2, value2, n_key =
      'foo', 'bar', 'fizz', 'buzz', 'rubber_duck'

memc = require 'lua-libmemcached'

-- new "libmemcached", arguments are optional, return false unless successful.
-- arguments: host/host:ip/unix socket/table of..., port/behavior(s)*
inst = memc.new(
    '127.0.0.1' or 'localhost:11211' or {'localhost:11211', {'10.1.1.66', 11211}},
    11211 or {"use_udp", no_block=true, distribution='consistent'}
  )


-- add server(s)/port, port are optional, return true if successful.
inst:add_server(
    '10.1.1.66' or 'localhost:11211' or {'10.1.1.69:11211', {'10.1.1.69', 11212}},
    11211
  )


-- set behavior(s)* flag, flag are optional, return true if successful.
inst:set_behavior(
    'tcp_nodelay' or {"enable_cas", use_binary=true}, false
  )


-- get behavior(s)* value, return table/value if successful.
behavior = inst:get_behavior('ketama_hash' or {"distribution", 'no_block'})


-- add a key(s) with value(s), ttl*, returns true if successful. **
inst:add(key or {[key]=value, [key2]=value2}, value, 3600 or 0)


-- stores a value(s) in given key(s) with ttl*, returns true if successful. **
inst:set(key or {[key2]=value2, [key]=value}, value, 7200 or 0)


-- get value(s), cas token by key, returns false if not successful. **
local value, cas_token = inst:get(key or {key, key2})


-- replace a value(s), ttl* in given key(s), returns true if successful. **
-- same as set, but fails if the key does not exist on the server
inst:replace(key or {[key2]=value2, [key]=value}, value, 7200 or 0)


-- compare and swap a key with value/ttl*, returns true if successful. **
inst:cas(cas_token, key, value, 86400 or 0)


-- appends data to value(s) in a given key(s), returns true if successful. **
inst:append(key or {[key]=value, [key2]=value2}, value)


-- prepends data to value(s) in a given key(s), returns true if successful. **
inst:prepend(key or {[key2]=value2, [key]=value}, value)


-- delete key(s) or delete with delayed time*, returns true if successful. **
inst:delete(key or {[key]=value, [key2]=value2}, 60 or 0)


-- increments value of a key with offset* and ttl** **
-- returns new value or false if not successful.
inst:incr(n_key, 10 or 1, 1800 or 0)


-- decrements value of a key with offset* and ttl** **
-- returns new value or false if not successful.
inst:decr(n_key, 10 or 1, 1800 or 0)

-- safe key checksum, returns false/true and string with message if error. **
local checksum, err_str = inst:check_key(key2)

``` 

# behaviors

check the links below to more understanding on each subject.

## optional

**[use_binary](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_BINARY_PROTOCOL)**: *boolean*, default true.

**[use_udp](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_USE_UDP)**: *boolean*.

**[no_block](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_NO_BLOCK)**: *boolean*.

**[keepalive](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_KEEPALIVE)**: *boolean*.

**[enable_cas](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_SUPPORT_CAS)**: *boolean*.

**[tcp_nodelay](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_TCP_NODELAY)**: *boolean*.

**[no_reply](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_NOREPLY)**: *boolean*.

**[send_timeout](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_SND_TIMEOUT)**: in microseconds.

**[receive_timeout](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_RCV_TIMEOUT)** in microseconds.

**[connect_timeout](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT)** in microseconds.

**[poll_timeout](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_POLL_TIMEOUT)** in seconds? - default -1.

**[keepalive_idle](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_KEEPALIVE_IDLE)** in seconds, linux only.

**[retry_timeout](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_RETRY_TIMEOUT)** in seconds.

**[hash](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_HASH)**: 
  *[md5](http://bit.ly/h4sh3z#MEMCACHED_HASH_MD5)*,
  *[crc](http://bit.ly/h4sh3z#MEMCACHED_HASH_CRC)*,
  *[fnv1_64](http://bit.ly/h4sh3z#MEMCACHED_HASH_FNV1_64)*,
  *[fnv1a_64](http://bit.ly/h4sh3z#MEMCACHED_HASH_FNV1A_64)*,
  *[fnv1_32](http://bit.ly/h4sh3z#MEMCACHED_HASH_FNV1_32)*,
  *[fnv1a_32](http://bit.ly/h4sh3z#MEMCACHED_HASH_FNV1A_32)*,
  *[jenkins](http://bit.ly/h4sh3z#MEMCACHED_HASH_JENKINS)*,
  *[hsieh](http://bit.ly/h4sh3z#MEMCACHED_HASH_HSIEH)*,
  *[murmur](http://bit.ly/h4sh3z#MEMCACHED_HASH_MURMUR)*,
  *[murmur3](http://bit.ly/h4sh3z#MEMCACHED_HASH_MURMUR3)* and
  *[default](http://bit.ly/h4sh3z#MEMCACHED_HASH_DEFAULT)*

**[ketama_hash](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_KETAMA_HASH)**: 
  *[md5](http://bit.ly/h4sh3z#MEMCACHED_HASH_MD5)*,
  *[crc](http://bit.ly/h4sh3z#MEMCACHED_HASH_CRC)*,
  *[fnv1_64](http://bit.ly/h4sh3z#MEMCACHED_HASH_FNV1_64)*,
  *[fnv1a_64](http://bit.ly/h4sh3z#MEMCACHED_HASH_FNV1A_64)*,
  *[fnv1_32](http://bit.ly/h4sh3z#MEMCACHED_HASH_FNV1_32)*,
  *[fnv1a_32](http://bit.ly/h4sh3z#MEMCACHED_HASH_FNV1A_32)* and
  *[default](http://bit.ly/h4sh3z#MEMCACHED_HASH_DEFAULT)*

**[distribution](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_DISTRIBUTION)**: 
  *[modula](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_DISTRIBUTION)*,
  *[consistent](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_DISTRIBUTION)*,
  *[weighted](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED)*,
  *[compat](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_KETAMA_COMPAT)* and
  *[compat_spy](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_KETAMA_COMPAT)*

## preset

**[MEMCACHED_BEHAVIOR_VERIFY_KEY](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_VERIFY_KEY)**

**[MEMCACHED_BEHAVIOR_HASH](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_HASH)**: 
  **[MEMCACHED_HASH_MURMUR](http://bit.ly/h4sh3z#MEMCACHED_HASH_MURMUR)**, (if murmur3 is unavailable) 
  **[MEMCACHED_HASH_MURMUR3](http://bit.ly/h4sh3z#MEMCACHED_HASH_MURMUR3)**

**[MEMCACHED_BEHAVIOR_BINARY_PROTOCOL](http://bit.ly/b3h4v1or#MEMCACHED_BEHAVIOR_BINARY_PROTOCOL)**

# tests

see test.lua ...

# TODO

+ support callbacks
+ support luvit module style
+ create a test suite
+ improve makefile

% August 04th, 2013 -03 GMT
